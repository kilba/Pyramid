// GL
#include <glad/glad.h>
#include <cglm/cglm.h>

// Basilisk
#include <bs_mem.h>
#include <bs_core.h>
#include <bs_shaders.h>
#include <bs_textures.h>
#include <bs_wnd.h>

// STD
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

bs_Buffer shader_entity_buf;
bs_Buffer shader_entity_data_buf;
bs_Space entity_shader_space;

const char* global_shader = NULL;
int global_shader_len = 0;

// Space Communication-
void bs_pushShaderBuffers() {
    shader_entity_buf = bs_buffer(0, sizeof(bs_ShaderEntity), 16, 0, 0);
    shader_entity_data_buf = bs_buffer(0, sizeof(bs_ShaderEntityData), 16, 0, 0);

    // Temporary until #include system is implemented
    global_shader = bs_loadFile("resources/basilisk.bsh", &global_shader_len);
}

void bs_pushShaderSpaces() {
    entity_shader_space = bs_shaderSpace(shader_entity_data_buf, BS_SPACE_ENTITIES, BS_STD430);
}
//-Space Communication

bs_ShaderEntity* bs_pushShaderEntity(bs_mat4 transformation) {
    bs_ShaderEntity* entity = bs_bufferAppend(&shader_entity_buf, NULL);
    bs_ShaderEntityData* entity_data = bs_bufferAppend(&shader_entity_data_buf, NULL);

    memset(entity, 0, sizeof(bs_ShaderEntity));
    memset(entity_data, 0, sizeof(bs_ShaderEntityData));

    entity_data->transform = transformation;
    entity->entity_data = entity_data;
    entity->buffer_location = shader_entity_buf.size - 1;

    return entity;
}

void bs_pushEntityShaderDataBuffer() {
    bs_U32 first = 0;
    bs_U32 last = 0xFFFFFFFF;

    for (int i = 0; i < shader_entity_buf.size; i++) {
        bs_ShaderEntity* entity = bs_bufferData(&shader_entity_buf, i);
        first = i;

        if (entity->changed) {
            first = i;
            break;
        }
    }

    for (int i = shader_entity_buf.size - 1; i >=0; i--) {
        bs_ShaderEntity* entity = bs_bufferData(&shader_entity_buf, i);
        if (entity->changed) {
            last = i + 1;
            entity->changed = false;
            break;
        }

        entity->changed = false;
    }

    bs_updateShaderSpace(&entity_shader_space, shader_entity_data_buf.data, 0, shader_entity_buf.size);
}
 
// Shader Spaces
bs_Space bs_shaderSpace(bs_Buffer buffer, bs_U32 binding, bs_SpaceType type) {
    bs_Space shader_space = { 0 };
    bs_U32 total_size = buffer.size * buffer.unit_size;

    if (type == BS_STD140 && total_size > 16384) {
        bs_callErrorf(BS_ERROR_SHADER_SPACE_INCOMPATIBLE_FORMAT, 1, "Cannot use std140 with a size > 16384");
        return shader_space;
    }

    shader_space.gl_type = (type == BS_STD430) ? GL_SHADER_STORAGE_BUFFER : GL_UNIFORM_BUFFER;
    shader_space.bind_point = binding;
    shader_space.buf = buffer;

    glGenBuffers(1, &shader_space.accessor);
    glBindBuffer(shader_space.gl_type, shader_space.accessor);
    glBufferData(shader_space.gl_type, total_size, buffer.data, GL_STREAM_DRAW);
    glBindBufferBase(shader_space.gl_type, shader_space.bind_point, shader_space.accessor);

    return shader_space;
}

void bs_updateShaderSpace(bs_Space* shader_space, void* data, bs_U32 index, bs_U32 num_units) {
    if (shader_space->accessor == 0) return;

    if ((index + num_units) > shader_space->buf.size) {
        bs_callErrorf(BS_ERROR_SHADER_SPACE_OUT_OF_RANGE, 1, "Attempted to access shader space out of range");
        return;
    }

    if (index == shader_space->buf.size) return;

    glBindBuffer(shader_space->gl_type, shader_space->accessor);
    glBufferSubData(shader_space->gl_type, shader_space->buf.unit_size * index, shader_space->buf.unit_size * num_units, data);
}

void bs_fetchShaderSpace(bs_Space* shader_space, bs_U32 offset, bs_U32 size, void* out) {
    glBindBuffer(shader_space->gl_type, shader_space->accessor);
    glGetBufferSubData(shader_space->gl_type, offset, size, out);
}

bs_Shader bs_shader(bs_VertexShader* vs, bs_FragmentShader* fs, bs_GeometryShader* gs, const char* name) {
    bs_Shader shader = { 0 };
    if (!vs || !fs) return shader;

    shader.vs = vs;
    shader.id = glCreateProgram();
    if (shader.id == 0) {
        bs_callErrorf(BS_ERROR_SHADER_PROGRAM_CREATION, 2, "Failed while creating shader");
        return shader;
    }

    glAttachShader(shader.id, vs->id);
    glAttachShader(shader.id, fs->id);
    if (gs != NULL) glAttachShader(shader.id, gs->id);

    glLinkProgram(shader.id);
    glUseProgram(shader.id);

    // set sampler uniforms
    GLint count;
    glGetProgramiv(shader.id, GL_ACTIVE_UNIFORMS, &count);
    for (int i = 0; i < count; i++) {
        GLchar name[16];
        GLsizei length;
        GLint size;
        GLenum type;

        glGetActiveUniform(shader.id, (GLuint)i, 16, &length, &size, &type, name);
        if (type == GL_SAMPLER_2D || type == GL_SAMPLER_2D_ARRAY) {
            glUniform1i(glGetUniformLocation(shader.id, name), name[length - 1] - '0');
        }
    }

    // cleanup
    glDetachShader(shader.id, vs->id);
    glDetachShader(shader.id, fs->id);
    if (gs != NULL) glDetachShader(shader.id, gs->id);

    // label
    if (name != NULL) {
        // glObjectLabel(GL_SHADER, shader.id, strlen(name), name);
    }

    return shader;
}

const char* bs_compileShader(GLuint* shader_id, int type, char* path) {
    int len = 0;
    const char* code = bs_loadFile(path, &len);
    const char* repl = bs_replaceFirstSubstring(code, "#define BASILISK", global_shader);

    if (repl != NULL) {
        free(code);
        code = repl;
    }

    // compile
    *shader_id = glCreateShader(type);
    glShaderSource(*shader_id, 1, &code, NULL);
    glCompileShader(*shader_id);

    GLint is_compiled = GL_FALSE;
    glGetShaderiv(*shader_id, GL_COMPILE_STATUS, &is_compiled);

    // handle error
    if (!is_compiled) {
        GLint max_length = 0;
        glGetShaderiv(*shader_id, GL_INFO_LOG_LENGTH, &max_length);

        char* log = bs_alloc(max_length);
        glGetShaderInfoLog(*shader_id, max_length, &max_length, log);
        glDeleteShader(*shader_id);

        bs_callErrorf(BS_ERROR_SHADER_COMPILATION, 2, "Shader \"%s\" failed during compilation\n\nLog:\n%s\n", path, log);
        free(log);
        return NULL;
    }

    return code;
}

void bs_setVertexShaderAttributes(bs_VertexShader* vs, const char* vs_code) {
    struct {
        char* name;
        int value;
        uint8_t size;
    } attribs[] = {
        { "in vec3 bs_Position" , BS_VAL_POSITION, sizeof(bs_vec3) },
        { "in vec2 bs_Texture" , BS_VAL_TEXTURE, sizeof(bs_vec2) },
        { "in vec4 bs_Color" , BS_VAL_COLOR, sizeof(bs_RGBA) },
        { "in vec3 bs_Normal" , BS_VAL_NORMAL, sizeof(bs_vec3) },
        { "in ivec4 bs_BoneId", BS_VAL_BONEID, sizeof(bs_ivec4) },
        { "in vec4 bs_Weight" , BS_VAL_WEIGHT, sizeof(bs_vec4) },
        { "in uint bs_Entity", BS_VAL_ENTITY, sizeof(bs_U32) },
        { "in uint bs_Image", BS_VAL_IMAGE, sizeof(bs_U32) }
    };

    int num = sizeof(bs_AttributeSizes); // Each element should be uint8_t so no need to divide
    for (int i = 0; i < num; i++) {
        uint8_t* attrib_sizes = (uint8_t*)&vs->attrib_sizes;
        uint8_t* attrib_size = attrib_sizes + i;
        *attrib_size = 0;

        if (strstr(vs_code, attribs[i].name)) {
            *attrib_size = attribs[i].size;

            vs->attrib_size_bytes += attribs[i].size;
            vs->attribs |= attribs[i].value;
            vs->attrib_count++;
        }
    }
}

bs_VertexShader bs_vertexShader(const char *path) {
    bs_VertexShader vs = { 0 };
    const char* code = bs_compileShader(&vs.id, GL_VERTEX_SHADER, path);
    bs_setVertexShaderAttributes(&vs, code);
    free(code);

    return vs;
}

bs_FragmentShader bs_fragmentShader(const char *path) {
    bs_FragmentShader fs = { 0 };
    free(bs_compileShader(&fs.id, GL_FRAGMENT_SHADER, path));

    return fs;
}

bs_GeometryShader bs_geometryShader(const char *path) {
    bs_GeometryShader gs = { 0 };
    free(bs_compileShader(&gs.id, GL_GEOMETRY_SHADER, path));

    return gs;
}

bs_ComputeShader bs_computeShader(const char* path) {
    bs_ComputeShader cs = { 0 };
    free(bs_compileShader(&cs.id, GL_COMPUTE_SHADER, path));

    cs.program = glCreateProgram();
    glAttachShader(cs.program, cs.id);
    glLinkProgram(cs.program);

    return cs;
}

void bs_computeSize(bs_ComputeShader* cs, bs_Texture* output, bs_U32 x, bs_U32 y, bs_U32 z) {
    glUseProgram(cs->program);
    if (output != NULL) {
        glBindImageTexture(0, output->id, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
    }
    glDispatchCompute(x, y, z);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

void bs_compute(bs_ComputeShader* cs, bs_Texture* output) {
    bs_computeSize(cs, output, output->w, output->h, 1);
}