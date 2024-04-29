#include <bs_types.h>
#include <bs_math.h>
#include <bs_ttf.h>
#include <bs_mem.h>
#include <bs_shaders.h>
#include <bs_ini.h>

#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <time.h>
#include <stdarg.h>
#include <assert.h>

#include <vulkan.h>

struct {
    bs_Batch* batch;
} selected = { 0 };

void bs_batchResizeCheck(int index_count, int vertex_count) {
    bs_bufferResizeCheck(&selected.batch->vertex_buf, vertex_count);
    bs_bufferResizeCheck(&selected.batch->index_buf, index_count);
}

inline bs_U8* bs_batchData(bs_Batch* batch, int index, int offset) {
    return batch->vertex_buf.data + (batch->vertex_buf.unit_size * index) + offset;
}

void bs_pushIndex(int idx) {
    idx += selected.batch->vertex_buf.num_units;
    memcpy(
	    (int *)(selected.batch->index_buf.data) + selected.batch->index_buf.num_units++, 
	    &idx, 
	    sizeof(bs_U32)
    );
}

void bs_pushIndices(int *idxs, int num_elems) {
    for (int i = 0; i < num_elems; i++) {
        bs_pushIndex(idxs[i]);
    }
}

void bs_pushIndexVa(int num_elems, ...) {
    va_list ptr;
    va_start(ptr, num_elems);

    for (int i = 0; i < num_elems; i++) {
        bs_pushIndex(va_arg(ptr, int));
    }

    va_end(ptr);
}

inline void bs_pushAttrib(uint8_t **data_ptr, void *data, uint8_t size) {
    memcpy(*data_ptr, data, size);
    *data_ptr += size;
}

void bs_pushVertex(
    bs_vec3  position,
    bs_vec2  texture,
    bs_vec3  normal,
    bs_RGBA  color,
    bs_ivec4 bone_id,
    bs_vec4  weight,
    bs_U32   entity,
    bs_U32   image
) {
    uint8_t* data_ptr = bs_bufferData(&selected.batch->vertex_buf, selected.batch->vertex_buf.num_units);
    bs_Attribute* attributes = selected.batch->pipeline.vs->attributes;

    bs_pushAttrib(&data_ptr, &position, attributes[BS_POSITION].size);
    bs_pushAttrib(&data_ptr, &texture, attributes[BS_TEXTURE].size);
    bs_pushAttrib(&data_ptr, &color, attributes[BS_COLOR].size);
    bs_pushAttrib(&data_ptr, &normal, attributes[BS_NORMAL].size);
    bs_pushAttrib(&data_ptr, &bone_id, attributes[BS_BONE_ID].size);
    bs_pushAttrib(&data_ptr, &weight, attributes[BS_WEIGHT].size);
    bs_pushAttrib(&data_ptr, &entity, attributes[BS_ENTITY].size);
    bs_pushAttrib(&data_ptr, &image, attributes[BS_IMAGE].size);

    selected.batch->vertex_buf.num_units++;
}

bs_BatchPart bs_pushQuad(bs_Quad quad, bs_RGBA color, bs_Image* image, bs_ShaderEntity* shader_entity) {
    bs_U32 ent = shader_entity == NULL ? 0 : shader_entity->buffer_location;
    bs_U32 img = image         == NULL ? 0 : image->buffer_location;

    bs_batchResizeCheck(6, 4);
    bs_pushIndexVa(6, 0, 1, 2, 2, 1, 3);

    bs_pushVertex(quad.a, quad.ca, bs_v3s(0.0), color, bs_iv4s(0), bs_v4s(0), ent, img);
    bs_pushVertex(quad.b, quad.cb, bs_v3s(0.0), color, bs_iv4s(0), bs_v4s(0), ent, img);
    bs_pushVertex(quad.c, quad.cc, bs_v3s(0.0), color, bs_iv4s(0), bs_v4s(0), ent, img);
    bs_pushVertex(quad.d, quad.cd, bs_v3s(0.0), color, bs_iv4s(0), bs_v4s(0), ent, img);

    return (bs_BatchPart) { selected.batch->index_buf.num_units, selected.batch->index_buf.num_units };
}

bs_BatchPart bs_pushTriangle(bs_vec3 a, bs_vec3 b, bs_vec3 c, bs_RGBA color, bs_ShaderEntity* shader_entity) {
    bs_U32 entity = shader_entity == NULL ? 0 : shader_entity->buffer_location;

    bs_batchResizeCheck(3, 3);
    bs_pushIndexVa(3, 0, 1, 2),

    bs_pushVertex(a, bs_v2(0.0, 0.0), bs_v3s(0.0), color, bs_iv4s(0), bs_v4s(0.0), entity, 0);
    bs_pushVertex(b, bs_v2(1.0, 0.0), bs_v3s(0.0), color, bs_iv4s(0), bs_v4s(0.0), entity, 0);
    bs_pushVertex(c, bs_v2(0.0, 1.0), bs_v3s(0.0), color, bs_iv4s(0), bs_v4s(0.0), entity, 0);

    return (bs_BatchPart){ selected.batch->index_buf.num_units, selected.batch->index_buf.num_units };
}

bs_Batch bs_batch(bs_Pipeline* pipeline) {
    bs_Batch batch = { 0 };
    batch.pipeline = *pipeline;
    batch.use_indices = true;

    // vao, vbo, ebo
    // attribute setup

    batch.vertex_buf = bs_buffer(batch.pipeline.vs->attrib_size_bytes, BS_BATCH_INCR_BY, 0, 0);
    batch.index_buf  = bs_buffer(sizeof(bs_U32), BS_BATCH_INCR_BY, 0, 0);

    return batch;
}

static bs_U32 bs_queryMemoryType(uint32_t filter, VkMemoryPropertyFlags props) {
    VkPhysicalDeviceMemoryProperties mem_props;
    vkGetPhysicalDeviceMemoryProperties(bs_vkPhysicalDevice(), &mem_props);

    for (uint32_t i = 0; i < mem_props.memoryTypeCount; i++) {
        if ((filter & (1 << i)) && (mem_props.memoryTypes[i].propertyFlags & props) == props) {
            return i;
        }
    }

    bs_throw("Failed to find memory type");
}

void bs_pushBatch() {
    assert(selected.batch != NULL);

    VkBufferCreateInfo buffer_i = { 0 };
    buffer_i.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_i.size = selected.batch->vertex_buf.num_units * selected.batch->vertex_buf.unit_size;
    buffer_i.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    buffer_i.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkBuffer vertex_buffer;
    BS_VK_ERR(vkCreateBuffer(bs_vkDevice(), &buffer_i, NULL, &vertex_buffer), "Failed to create vertex buffer");

    VkMemoryRequirements mem_req;
    vkGetBufferMemoryRequirements(bs_vkDevice(), vertex_buffer, &mem_req);

    VkMemoryAllocateInfo alloc_i = { 0 };
    alloc_i.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_i.allocationSize = mem_req.size;
    alloc_i.memoryTypeIndex = bs_queryMemoryType(mem_req.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    VkDeviceMemory memory;
    BS_VK_ERR(vkAllocateMemory(bs_vkDevice(), &alloc_i, NULL, &memory), "Failed to allocate memory");

    vkBindBufferMemory(bs_vkDevice(), vertex_buffer, memory, 0);

    void* data;
    vkMapMemory(bs_vkDevice(), memory, 0, buffer_i.size, 0, &data);
    memcpy(data, selected.batch->vertex_buf.data, buffer_i.size);
    vkUnmapMemory(bs_vkDevice(), memory);

    selected.batch->vbuffer = vertex_buffer;
}

void bs_selectBatch(bs_Batch* batch) {
    selected.batch = batch;
}