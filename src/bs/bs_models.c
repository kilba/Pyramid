#include <stdio.h>
#include <time.h>

#define CGLTF_IMPLEMENTATION
#include <cglm/cglm.h>

#include <stdint.h>

#include <bs_mem.h>
#include <bs_core.h>
#include <bs_math.h>
#include <bs_models.h>
#include <bs_shaders.h>
#include <bs_textures.h>
#include <bs_wnd.h>
#include <bs_json.h>

bs_Buffer animation_buf;
bs_Buffer armature_buf;

bs_Space armature_shader_space;

// Space Communication-
void bs_pushModelBuffers() {
    animation_buf = bs_buffer(0, sizeof(bs_Animation), 4, 32, 0);
    armature_buf = bs_buffer(0, sizeof(bs_mat4), 64, 0, 0);
}

void bs_calculateArmaturePose(bs_Armature* armature, bs_Animation* animation, float time);
void bs_updateArmature(bs_ArmatureStorage storage, bs_Animation* animation, float time) {
    if (animation == NULL) {
        return;
    }

    bs_calculateArmaturePose(storage.armature, animation, time);
    bs_updateShaderSpace(&armature_shader_space, storage.armature->joint_matrices, storage.buffer_location, storage.armature->num_joints);
}

void bs_pushArmatures() {
    armature_shader_space = bs_shaderSpace(armature_buf, BS_SPACE_ARMATURES, BS_STD430);
}
//-Space Communication

inline bs_mat4 bs_gltfMat4(bs_Gltf* gltf, int accessor, int i) {
    int buffer_view = bs_jsonField(gltf->accessors.as_objects + accessor, "bufferView").as_int;
    //int count = bs_jsonField(gltf->accessors.as_objects + accessor, "count").as_int;

    bs_Json* buffer_view_ptr = gltf->buffer_views.as_objects + buffer_view;
    int len = bs_jsonField(buffer_view_ptr, "byteLength").as_int;
    int offset = bs_jsonField(buffer_view_ptr, "byteOffset").as_int;

    bs_mat4 m = { 0 };
    memcpy(m.a, gltf->buffer + offset + i * sizeof(bs_mat4), sizeof(bs_mat4));
    return m;
}

inline int* bs_gltfIntArray(bs_Gltf* gltf, int accessor, int* out_len, int num_components, bs_U32 size) {
    int buffer_view = bs_jsonField(gltf->accessors.as_objects + accessor, "bufferView").as_int;

    bs_Json* buffer_view_ptr = gltf->buffer_views.as_objects + buffer_view;
    *out_len = bs_jsonField(buffer_view_ptr, "byteLength").as_int / size;
    int offset = bs_jsonField(buffer_view_ptr, "byteOffset").as_int;

    return (int*)(gltf->buffer + offset);
}

inline float* bs_gltfFloatArray(bs_Gltf* gltf, int accessor, int* out_len, int num_components) {
    int buffer_view = bs_jsonField(gltf->accessors.as_objects + accessor, "bufferView").as_int;
    int count = bs_jsonField(gltf->accessors.as_objects + accessor, "count").as_int;

    bs_Json* buffer_view_ptr = gltf->buffer_views.as_objects + buffer_view;
    *out_len = count * num_components;
    int offset = bs_jsonField(buffer_view_ptr, "byteOffset").as_int;

    return (float*)(gltf->buffer + offset);
}

bs_quat bs_interpolateRotation(bs_Armature* armature, bs_AnimationJoint* animation_joint, float time) {
    int index = 0;
    for (; index < animation_joint->num_rotations; index++) {
        if (animation_joint->rotations[index].time >= time) {
            break;
        }
    }

    if (index >= animation_joint->num_rotations) {
        return animation_joint->rotations[(animation_joint->num_rotations == 0) ? 0 : (animation_joint->num_rotations - 1)].value;
    }

    return animation_joint->rotations[index].value;
}

bs_vec3 bs_interpolateTranslation(bs_AnimationJoint* animation_joint, float time) {
    int time_index = 0;
    for (; time_index < animation_joint->num_translations; time_index++) {
        if (animation_joint->translations[time_index].time >= time) {
            break;
        }
    }

    if (time_index >= animation_joint->num_translations) {
        return animation_joint->translations[(animation_joint->num_translations == 0) ? 0 : (animation_joint->num_translations - 1)].value;
    }

    return animation_joint->translations[time_index].value;
}

bs_mat4 bs_jointTransformation(bs_Armature* armature, bs_Joint* joint) {
    if (joint == NULL) return (bs_mat4)BS_MAT4_IDENTITY;
    return armature->joint_matrices[joint->id];
}

bs_vec3 bs_jointPosition(bs_Armature* armature, bs_Joint* joint) {
    return bs_m4mulv4(bs_jointTransformation(armature, joint), bs_v4(0.0, 0.0, 0.0, 1.0)).xyz;
}

void bs_calculateJoint(bs_Armature* armature, bs_Joint* joint, bs_mat4 transformation, bs_mat4* destination) {
    const bs_mat4 parent = (joint->parent_idx == -1) ? (bs_mat4)BS_MAT4_IDENTITY : armature->joint_matrices[joint->parent_idx];

    glm_mat4_mul(joint->bind_matrix.a, joint->local_inv.a, destination);
    glm_mat4_mul(destination, &transformation, destination);
    glm_mat4_mul(destination, joint->bind_matrix_inv.a, destination);
    glm_mat4_mul(parent.a, destination, destination);
}

void bs_calculateArmaturePose(bs_Armature* armature, bs_Animation* animation, float time) {
    for (int i = 0; i < armature->num_joints; i++) {
        bs_Joint* joint = armature->joints + i;

        bs_vec3 translation = bs_interpolateTranslation(animation->joints + i, time);
        bs_quat rotation = bs_interpolateRotation(armature, animation->joints + i, time);
        bs_vec3 scale = animation->joints[i].scalings[0].value;

        bs_calculateJoint(armature, armature->joints + i, bs_transform(translation, rotation, scale), armature->joint_matrices + i);
    }
}

bs_ArmatureStorage bs_pushArmature(bs_Armature *armature, bs_Animation *resting_anim) {
    bs_ArmatureStorage armature_storage;

    armature_storage.buffer_location = armature_buf.size;
    armature_storage.armature = armature;

    if (resting_anim != NULL) {
        bs_calculateArmaturePose(armature, resting_anim, 0.0);
    }
    else {
        // set to identity matrix if theres no resting animation
        for (int i = 0; i < armature->num_joints; i++) {
            bs_mat4* joint = armature->joint_matrices + i;
            *joint = (bs_mat4)BS_MAT4_IDENTITY;
        }
    }

    bs_bufferAppendRange(&armature_buf, armature->joints, armature->num_joints);
    return armature_storage;
}

void bs_modelAttribData(bs_Gltf* gltf, bs_Primitive* primitive, int accessor, int num_components, int offset) {
    int num_floats = 0;
    float* a = bs_gltfFloatArray(gltf, accessor, &num_floats, num_components);

    for (int i = 0; i < num_floats / num_components; i++, offset += primitive->vertex_size) {
        for (int j = 0; j < num_components; j++) {
            primitive->vertices[offset + j] = a[i * num_components + j];
        }
    }
}

void bs_modelAttribDataI(
    bs_Gltf* gltf, 
    bs_Primitive* primitive, 
    int accessor, 
    int num_components, 
    int offset, 
    unsigned int* out) 
{
    int num_floats = 0;
    bs_U8* arr = bs_gltfFloatArray(gltf, accessor, &num_floats, num_components);

    for (int i = 0; i < num_floats / num_components; i++, offset += primitive->vertex_size) {
        for (int j = 0; j < num_components; j++) {
            out[offset + j] = arr[i * num_components];
        }
    }
}

void bs_readIndices(
    bs_Gltf* gltf, 
    bs_Model* model, 
    bs_Mesh* mesh, 
    bs_Primitive* primitive, 
    bs_Json* primitive_json) 
{
    int accessor = bs_jsonField(primitive_json, "indices").as_int;

    bs_U16* arr = bs_gltfFloatArray(gltf, accessor, &primitive->num_indices, 1);
    primitive->indices = bs_alloc(primitive->num_indices * sizeof(int));
    for (int i = 0; i < primitive->num_indices; i++) {
        primitive->indices[i] = arr[i];
    }

    mesh->num_vertices += primitive->num_vertices;
    mesh->num_indices += primitive->num_indices;
    model->num_vertices += primitive->num_vertices;
    model->num_indices += primitive->num_indices;
}

bs_Primitive* bs_loadPrim(
    bs_Gltf* gltf,
    bs_Model* model, 
    bs_Mesh* mesh, 
    bs_Json* mesh_json, 
    bs_Primitive* primitive, 
    bs_Json* primitive_json) 
{
    memset(primitive, 0, sizeof(bs_Primitive));

    bs_Json attributes = bs_jsonField(primitive_json, "attributes").as_object;
    bs_JsonValue position = bs_jsonField(&attributes, "POSITION");
    bs_JsonValue normal = bs_jsonField(&attributes, "NORMAL");
    bs_JsonValue tex_coord = bs_jsonField(&attributes, "TEXCOORD_0");
    bs_JsonValue joints = bs_jsonField(&attributes, "JOINTS_0");
    bs_JsonValue weights = bs_jsonField(&attributes, "WEIGHTS_0");

    primitive->aabb.max = bs_v3s(-FLT_MAX);
    primitive->aabb.min = bs_v3s(FLT_MAX);

    int num_floats = 0;
    int* indices_array = bs_gltfIntArray(gltf, position.as_int, &num_floats, 1, sizeof(bs_U32));

    int vertex_size = 0;

    if (position.found) {
        vertex_size += 3;
    }
    if (normal.found) {
        primitive->offset_nor = vertex_size;
        vertex_size += 3;
    }
    if (tex_coord.found) {
        primitive->offset_tex = vertex_size;
        vertex_size += 2;
    }
    if (joints.found) {
        primitive->offset_bid = vertex_size;
        vertex_size += 4;
    }
    if (weights.found) {
        primitive->offset_wei = vertex_size;
        vertex_size += 4;
    }

    primitive->parent = mesh;
    primitive->vertex_size = vertex_size;
    primitive->num_vertices = num_floats / 3;
    primitive->vertices = bs_alloc(primitive->num_vertices * vertex_size * sizeof(float));

    if (position.found) bs_modelAttribData(gltf, primitive, position.as_int, 3, 0);
    if (normal.found) bs_modelAttribData(gltf, primitive, normal.as_int, 3, primitive->offset_nor);
    if (tex_coord.found) bs_modelAttribData(gltf, primitive, tex_coord.as_int, 2, primitive->offset_tex);
    if (joints.found) bs_modelAttribDataI(gltf, primitive, joints.as_int, 4, primitive->offset_bid, primitive->vertices);
    if (weights.found) bs_modelAttribData(gltf, primitive, weights.as_int, 4, primitive->offset_wei);

    bs_readIndices(gltf, model, mesh, primitive, primitive_json);
    
    return primitive;
}

bs_Mesh* bs_loadMesh(
    bs_Gltf* gltf, 
    bs_Model* model, 
    bs_Mesh* mesh, 
    bs_Json* mesh_json, 
    bs_Json* node_json) 
{
    memset(mesh, 0, sizeof(bs_Mesh));

    bs_JsonValue name = bs_jsonField(node_json, "name");
    bs_JsonValue primitives = bs_jsonField(mesh_json, "primitives");

    mesh->parent = model;
    mesh->aabb.max = bs_v3s(-FLT_MAX);
    mesh->aabb.min = bs_v3s(FLT_MAX);
    mesh->name = bs_alloc(strlen(name.as_string.value) + 1);
    mesh->primitives = bs_alloc(primitives.as_array.size * sizeof(bs_Primitive));
    mesh->num_primitives = primitives.as_array.size;
    sprintf(mesh->name, "%s", name.as_string.value);

    for (int i = 0; i < mesh->num_primitives; i++) {
        bs_Json* primitive_json = primitives.as_array.as_objects + i;
        bs_loadPrim(gltf, model, mesh, mesh_json, mesh->primitives + i, primitive_json);
    }

    model->prim_count += mesh->num_primitives;
    return mesh;
}

void bs_loadAnimationChannel(
    bs_Gltf* gltf,
    bs_Animation* animation,
    bs_Channel* channel,
    bs_U32 num_floats, 
    bs_U32 offset, 
    bs_U32* transformation_offset, 
    void** transformations) 
{
    *transformations = animation->data + offset + *transformation_offset;
    const size_t size = num_floats * sizeof(float) + sizeof(float);

    for (int i = 0; i < channel->num_inputs; i++, *transformation_offset += size) {
        float time = channel->inputs[i];
        float transformation[4];

        int o = offset + *transformation_offset;

        memcpy(transformation, channel->outputs + i * num_floats, num_floats * sizeof(float));
        memcpy(animation->data + o, &time, sizeof(time));
        memcpy(animation->data + o + sizeof(float), transformation, num_floats * sizeof(float));

        animation->length = bs_max(animation->length, time);
    }
}

void bs_calculateAnimationSizes(
    bs_Channel* channels,
    int num_channels,
    bs_U32* translations, 
    bs_U32* rotations, 
    bs_U32* scalings, 
    int* joint_count) 
{
    bs_U32 num_translations = 0, num_rotations = 0, num_scalings = 0;

    // size calculation
    int last_node_i = -1;
    for (int i = 0; i < num_channels; i++) {
        bs_Channel* channel = channels + i;

        if (channel->type == BS_TRANSLATION) num_translations += channel->num_inputs;
        else if (channel->type == BS_ROTATION) num_rotations += channel->num_inputs;
        else num_scalings += channel->num_inputs;

        if (last_node_i != channel->node) {
            (*joint_count)++;
        }

        last_node_i = channel->node;
    }

    (*translations) += num_translations * (sizeof(bs_vec3) + sizeof(float));
    (*rotations) += num_rotations * (sizeof(bs_quat) + sizeof(float));
    (*scalings) += num_scalings * (sizeof(bs_vec3) + sizeof(float));
}

void bs_channelDeclaration(bs_Json* json, void* destination, void* param) {
    bs_Channel channel = { 0 };
    bs_Gltf* gltf = param;

    bs_Json target = bs_jsonField(json, "target").as_object;
    const char* path = bs_jsonField(&target, "path").as_string.value;

    bs_Json* sampler = gltf->samplers.as_objects + bs_jsonField(json, "sampler").as_int;
    int input = bs_jsonField(sampler, "input").as_int;
    int output = bs_jsonField(sampler, "output").as_int;

    int num_outputs = 0;
    channel.node = bs_jsonField(&target, "node").as_int;
    channel.inputs = bs_gltfFloatArray(gltf, input, &channel.num_inputs, 1);
    channel.outputs = bs_gltfFloatArray(gltf, output, &num_outputs, 1);

    if (strcmp(path, "translation") == 0) channel.type = BS_TRANSLATION;
    else if (strcmp(path, "rotation") == 0) channel.type = BS_ROTATION;
    else channel.type = BS_SCALE;

    memcpy(destination, &channel, sizeof(bs_Channel));
}

void bs_loadAnimation(bs_Gltf* gltf, bs_Model* model, bs_Json* animation_json) {
    bs_Animation animation = { 0 };

    gltf->samplers = bs_jsonField(animation_json, "samplers").as_array;
    int num_channels = 0;
    bs_Channel* channels = bs_jsonFieldObjectArray(
        animation_json, "channels", &num_channels,
        sizeof(bs_Channel), bs_channelDeclaration, gltf
    );

    const char* name = bs_jsonField(animation_json, "name").as_string.value;
    animation.name = bs_alloc(strlen(name) + 1);
    sprintf(animation.name, "%s", name);

    bs_U32 num_translations = 0;
    bs_U32 num_rotations = 0;
    bs_U32 num_scalings = 0;

    bs_calculateAnimationSizes(
        channels,
        num_channels,
        &num_translations, 
        &num_rotations, 
        &num_scalings, 
        &animation.joint_count
    );

    animation.model = model;
    animation.data = bs_alloc(num_translations + num_rotations + num_scalings);
    animation.joints = bs_alloc(animation.joint_count * sizeof(bs_AnimationJoint));
    memset(animation.joints, 0, animation.joint_count * sizeof(bs_AnimationJoint));

    for (int i = 0; i < animation.joint_count; i++) {
        animation.joints[i].id = i;
    }

    int last_node_i = -1;
    bs_AnimationJoint* joint = animation.joints;
    bs_U32 translation_offset = 0;
    bs_U32 rotation_offset = 0;
    bs_U32 scale_offset = 0;

    for (int i = 0; i < num_channels; i++) {
        bs_Channel* channel = channels + i;

        if (last_node_i != -1 && last_node_i != channel->node) {
            joint++;
        }

        if (channel->type == BS_TRANSLATION) {
            joint->num_translations = channel->num_inputs;
            bs_loadAnimationChannel(
                gltf, &animation, channel, 
                3, 0,
                &translation_offset, 
                &joint->translations
            );
        }
        else if (channel->type == BS_ROTATION) {
            joint->num_rotations = channel->num_inputs;
            bs_loadAnimationChannel(
                gltf, &animation, channel,
                4, num_translations,
                &rotation_offset,
                &joint->rotations
            );
        }
        else {
            joint->num_scalings = channel->num_inputs;
            bs_loadAnimationChannel(
                gltf, &animation, channel, 
                3, num_translations + num_rotations,
                &scale_offset,
                &joint->scalings
            );
        }

        last_node_i = channel->node;
    }

    bs_bufferAppend(&animation_buf, &animation);
    free(channels);
}

void bs_loadArmature(bs_Gltf* gltf, bs_Json* skin, bs_Armature* armature) {
    bs_JsonValue name = bs_jsonField(skin, "name");
    bs_JsonValue inverse_bind_matrices = bs_jsonField(skin, "inverseBindMatrices");
    bs_JsonArray joints = bs_jsonField(skin, "joints").as_array;

    int strlen_skin_name = strlen(name.as_string.value);
    armature->name = bs_alloc(strlen_skin_name + 1);
    strncpy(armature->name, name.as_string.value, strlen_skin_name);
    armature->name[strlen_skin_name] = '\0';

    armature->joint_matrices = bs_alloc(joints.size * sizeof(bs_Joint));
    armature->joints = bs_alloc(joints.size * sizeof(bs_Joint));
    armature->num_joints = joints.size;

    int stack_size = 0;
    bs_Joint* root = &armature->joints[stack_size++];
    for (int i = 0; i < armature->num_joints; i++) {
        armature->joints[i].parent_idx = -1;
    }
    for (int i = 0; i < armature->num_joints; i++) {
        bs_Json* node = gltf->nodes.as_objects + joints.as_ints[i];
        bs_Joint* joint = armature->joints + i;

        bs_JsonArray children = bs_jsonField(node, "children").as_array;
        for (int j = 0; j < children.size; j++) {
            // todo improve this
            bs_Json* child_node = gltf->nodes.as_objects + children.as_ints[j];
            for (int k = 0; k < armature->num_joints; k++) {
                if (joints.as_ints[k] == children.as_ints[j]) {
                    armature->joints[k].parent_idx = i;
                    break;
                }
            }
            children.as_ints[j] = i;
        }

        joint->bind_matrix_inv = bs_gltfMat4(gltf, inverse_bind_matrices.as_int, i);
        glm_mat4_inv(joint->bind_matrix_inv.a, joint->bind_matrix.a);
        glm_mat4_inv(
            bs_transform(
                bs_jsonFieldV3(node, "translation", bs_v3s(0.0)),
                bs_jsonFieldV4(node, "rotation", bs_v4(0.0, 0.0, 0.0, 1.0)),
                bs_jsonFieldV3(node, "scale", bs_v3s(1.0))
            ).a,
            joint->local_inv.a
        );

        // Set name
        const char* joint_name = bs_jsonField(node, "name").as_string.value;
        int joint_name_len = strlen(joint_name);
        joint->name = bs_alloc(joint_name_len + 1);
        memcpy(joint->name, joint_name, joint_name_len);
        joint->name[joint_name_len] = '\0';
        joint->id = i;
    }
}

bs_Model bs_model(const char* directory, const char* file_name) {
    const char model_path[MAX_PATH];
    sprintf(model_path, "%s/%s", directory, file_name);

    bs_Model model = { 0 };
    char* extension = strrchr(model_path, '.');

    if (extension == NULL) {
        bs_callErrorf(
            BS_ERROR_MODEL_INVALID_FILE_FORMAT, 2,
            "No extension found for path \"%s\"", model_path
        );
    }
    else if (strcmp(extension, ".gltf") != 0) {
        bs_callErrorf(
            BS_ERROR_MODEL_INVALID_FILE_FORMAT, 2,
            "Invalid model file format for path \"%s\", needs to be .glb or .gltf", model_path
        );
        return;
    }

    int len = 0;
    char* raw = bs_loadFile(model_path, &len);

    bs_Json json = bs_json(raw);
    bs_Gltf gltf = { 0 };
    gltf.meshes = bs_jsonField(&json, "meshes").as_array;
    gltf.skins = bs_jsonField(&json, "skins").as_array;
    gltf.nodes = bs_jsonField(&json, "nodes").as_array;
    gltf.accessors = bs_jsonField(&json, "accessors").as_array;
    gltf.buffer_views = bs_jsonField(&json, "bufferViews").as_array;
    gltf.buffers = bs_jsonField(&json, "buffers").as_array;
    gltf.animations = bs_jsonField(&json, "animations").as_array;

    if (gltf.buffers.size != 1) {
        bs_callErrorf(BS_ERROR_MODEL_GLTF_BUFFER_SIZE, 2, "Invalid buffer size (%d)", gltf.buffers.size);
    }

    int sz = 0;
    const char buffer_path[MAX_PATH];
    const char* uri = bs_jsonField(gltf.buffers.as_objects, "uri").as_string.value;
    gltf.buffer_size = bs_jsonField(gltf.buffers.as_objects, "byteLength").as_int;
    sprintf(buffer_path, "%s/%s", directory, uri);
    gltf.buffer = bs_loadFile(buffer_path, &sz);

    model.num_meshes = gltf.meshes.size;
    model.aabb.max = bs_v3s(-FLT_MAX);
    model.aabb.min = bs_v3s(FLT_MAX);
    model.meshes = bs_alloc(model.num_meshes * sizeof(bs_Mesh));
    model.name = bs_alloc(strlen(model_path) + 1);
    strcpy(model.name, model_path);

    model.armature_count = gltf.skins.size;
    model.armatures = bs_alloc(model.armature_count * sizeof(bs_Armature));

    for (int i = 0; i < model.armature_count; i++) {
        bs_loadArmature(&gltf, gltf.skins.as_objects + i, model.armatures + i);
    }

    for (int i = 0; i < gltf.nodes.size; i++) {
        bs_Json* node = gltf.nodes.as_objects + i;
        bs_JsonValue mesh_i = bs_jsonField(node, "mesh");
        if (!mesh_i.found) continue;

        bs_Json* mesh_json = gltf.meshes.as_objects + mesh_i.as_int;
        bs_loadMesh(&gltf, &model, model.meshes + mesh_i.as_int, mesh_json, node);
    }

    model.anim_count = gltf.animations.size;
    model.animation_offset = animation_buf.size;

    for (int i = 0; i < model.anim_count; i++) {
        bs_loadAnimation(&gltf, &model, gltf.animations.as_objects + i);
    }

    return model;
}

void bs_freeModel(bs_Model* model) {
    for (int i = 0; i < model->num_meshes; i++) {
        bs_Mesh* mesh = model->meshes + i;
        for (int j = 0; j < mesh->num_primitives; j++) {
            bs_Primitive* primitive = mesh->primitives + j;
            bs_free(primitive->indices);
            bs_free(primitive->vertices);
        }
        bs_free(mesh->name);
        bs_free(mesh->primitives);
    }
    bs_free(model->armatures);
    bs_free(model->name);
    bs_free(model->meshes);
}

bs_U32 bs_numModelTriangles(bs_Model* model) {
    return model->num_indices / 3;
}

void bs_calculateModelBounds(bs_Model* model) {
    if (model->aabb.min.x != FLT_MAX) return;

    for (int i = 0; i < model->num_meshes; i++) {
        bs_Mesh* mesh = model->meshes + i;

        for (int j = 0; j < mesh->num_primitives; j++) {
            bs_Primitive* prim = mesh->primitives + j;

            for (int k = 0; k < prim->num_vertices; k++) {
                bs_vec3 pos = *(bs_vec3*)(prim->vertices + k * prim->vertex_size);
                prim->aabb.min = bs_v3min(prim->aabb.min, bs_v3muls(pos, bs_defScale()));
                prim->aabb.max = bs_v3max(prim->aabb.max, bs_v3muls(pos, bs_defScale()));
            }

            mesh->aabb.min = bs_v3min(mesh->aabb.min, prim->aabb.min);
            mesh->aabb.max = bs_v3max(mesh->aabb.max, prim->aabb.max);
        }

        model->aabb.min = bs_v3min(model->aabb.min, mesh->aabb.min);
        model->aabb.max = bs_v3max(model->aabb.max, mesh->aabb.max);
    }
}

bs_Mesh* bs_meshFromName(bs_Model* model, const char* name) {
    for(int i = 0; i < model->num_meshes; i++) {
        bs_Mesh* mesh = model->meshes + i;
        if (strcmp(name, mesh->name) == 0) {
            return model->meshes + i;
        }
    }

    bs_callError(BS_ERROR_MODEL_MESH_NOT_FOUND, name);
    return NULL;
}

int bs_meshIdxFromName(bs_Model* model, const char* name) {
    bs_Mesh* mesh = bs_meshFromName(model, name);
    if (mesh == NULL) return -1;

    bs_U64 ptr0 = (bs_U64)model->meshes;
    bs_U64 ptr1 = (bs_U64)mesh;

    return (ptr1 - ptr0) / sizeof(bs_Mesh);
}

bs_Armature *bs_armature(bs_Model *model, const char* name) {
    for(int i = 0; i < model->armature_count; i++) {
	    bs_Armature *armature = model->armatures + i;
        if (strcmp(name, armature->name) == 0) return armature;
    }
    
    bs_callError(BS_ERROR_MODEL_ARMATURE_NOT_FOUND, name);
    return NULL;
}

bs_Joint* bs_boneFromName(bs_Armature* armature, const char* name) {
    for (int i = 0; i < armature->num_joints; i++) {
        if (strcmp(armature->joints[i].name, name) == 0) {
            return armature->joints + i;
        }
    }

    bs_callError(BS_ERROR_MODEL_BONE_NOT_FOUND, name);
    return armature->joints;
}

bs_Animation* bs_animation(bs_Model* model, const char* name) {
    for(int i = 0; i < model->anim_count; i++) {
	    bs_Animation *anim = bs_bufferData(&animation_buf, model->animation_offset + i);

        if (strcmp(name, anim->name) == 0) {
            return anim;
        }
    }

    bs_callError(BS_ERROR_MODEL_ANIMATION_NOT_FOUND, name);
    return NULL;
}

int bs_jointOffsetFromName(bs_Armature* armature, const char* name) {
    for (int i = 0; i < armature->num_joints; i++) {
        if (strcmp(armature->joints[i].name, name) == 0) {
            return i;
        }
    }

    return -1;
}