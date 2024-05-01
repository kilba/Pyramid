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

// - batches -
bs_BatchPart bs_batchRange(bs_U32 offset, bs_U32 num) {
    return (bs_BatchPart){ offset, num };
}

int bs_batchSize(bs_Batch* batch) {
    return (batch->use_indices) ? batch->index_buf.num_units : batch->vertex_buf.num_units;
}

void bs_batchResizeCheck(bs_Batch* batch, int index_count, int vertex_count) {
    bs_bufferResizeCheck(&batch->vertex_buf, vertex_count);
    bs_bufferResizeCheck(&batch->index_buf, index_count);
}

inline bs_U8* bs_batchData(bs_Batch* batch, int index, int offset) {
    return batch->vertex_buf.data + (batch->vertex_buf.unit_size * index) + offset;
}

void bs_pushIndex(bs_Batch* batch, int idx) {
    idx += batch->vertex_buf.num_units;
    memcpy(
	    (int *)(batch->index_buf.data) + batch->index_buf.num_units++, 
	    &idx, 
	    sizeof(bs_U32)
    );
}

void bs_pushIndices(bs_Batch* batch, int *idxs, int num_elems) {
    for (int i = 0; i < num_elems; i++) {
        bs_pushIndex(batch, idxs[i]);
    }
}

void bs_pushIndexVa(bs_Batch* batch, int num_elems, ...) {
    va_list ptr;
    va_start(ptr, num_elems);

    for (int i = 0; i < num_elems; i++) {
        bs_pushIndex(batch, va_arg(ptr, int));
    }

    va_end(ptr);
}

inline void bs_pushAttrib(uint8_t **data_ptr, void *data, uint8_t size) {
    memcpy(*data_ptr, data, size);
    *data_ptr += size;
}

void bs_pushVertex(
    bs_Batch* batch,
    bs_vec3  position,
    bs_vec2  texture,
    bs_vec3  normal,
    bs_RGBA  color,
    bs_ivec4 bone_id,
    bs_vec4  weight,
    bs_U32   entity,
    bs_U32   image
) {
    uint8_t* data_ptr = bs_bufferData(&batch->vertex_buf, batch->vertex_buf.num_units);
    bs_Attribute* attributes = batch->pipeline.vs->attributes;

    bs_pushAttrib(&data_ptr, &position, attributes[BS_POSITION].size);
    bs_pushAttrib(&data_ptr, &texture, attributes[BS_TEXTURE].size);
    bs_pushAttrib(&data_ptr, &color, attributes[BS_COLOR].size);
    bs_pushAttrib(&data_ptr, &normal, attributes[BS_NORMAL].size);
    bs_pushAttrib(&data_ptr, &bone_id, attributes[BS_BONE_ID].size);
    bs_pushAttrib(&data_ptr, &weight, attributes[BS_WEIGHT].size);
    bs_pushAttrib(&data_ptr, &entity, attributes[BS_ENTITY].size);
    bs_pushAttrib(&data_ptr, &image, attributes[BS_IMAGE].size);

    batch->vertex_buf.num_units++;
}

bs_BatchPart bs_pushQuad(bs_Batch* batch, bs_Quad quad, bs_RGBA color, bs_Image* image, bs_ShaderEntity* shader_entity) {
    bs_U32 ent = shader_entity == NULL ? 0 : shader_entity->buffer_location;
    bs_U32 img = image         == NULL ? 0 : image->buffer_location;

    bs_batchResizeCheck(batch, 6, 4);
    bs_pushIndexVa(batch, 6, 0, 1, 2, 2, 1, 3);

    bs_pushVertex(batch, quad.a, quad.ca, bs_v3s(0.0), color, bs_iv4s(0), bs_v4s(0), ent, img);
    bs_pushVertex(batch, quad.b, quad.cb, bs_v3s(0.0), color, bs_iv4s(0), bs_v4s(0), ent, img);
    bs_pushVertex(batch, quad.c, quad.cc, bs_v3s(0.0), color, bs_iv4s(0), bs_v4s(0), ent, img);
    bs_pushVertex(batch, quad.d, quad.cd, bs_v3s(0.0), color, bs_iv4s(0), bs_v4s(0), ent, img);

    return (bs_BatchPart) { batch->index_buf.num_units, batch->index_buf.num_units };
}

bs_BatchPart bs_pushTriangle(bs_Batch* batch, bs_vec3 a, bs_vec3 b, bs_vec3 c, bs_RGBA color, bs_ShaderEntity* shader_entity) {
    bs_U32 entity = shader_entity == NULL ? 0 : shader_entity->buffer_location;

    bs_batchResizeCheck(batch, 3, 3);
    bs_pushIndexVa(batch, 3, 0, 1, 2),

    bs_pushVertex(batch, a, bs_v2(0.0, 0.0), bs_v3s(0.0), color, bs_iv4s(0), bs_v4s(0.0), entity, 0);
    bs_pushVertex(batch, b, bs_v2(1.0, 0.0), bs_v3s(0.0), color, bs_iv4s(0), bs_v4s(0.0), entity, 0);
    bs_pushVertex(batch, c, bs_v2(0.0, 1.0), bs_v3s(0.0), color, bs_iv4s(0), bs_v4s(0.0), entity, 0);

    return (bs_BatchPart){ batch->index_buf.num_units, batch->index_buf.num_units };
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

void bs_prepareBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer* buffer, VkDeviceMemory* buffer_mem) {
    VkBufferCreateInfo buffer_i = { 0 };
    buffer_i.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_i.size = size;
    buffer_i.usage = usage;
    buffer_i.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    BS_VK_ERR(vkCreateBuffer(bs_vkDevice(), &buffer_i, NULL, buffer), "Failed to create buffer");

    VkMemoryRequirements mem_req;
    vkGetBufferMemoryRequirements(bs_vkDevice(), *buffer, &mem_req);

    VkMemoryAllocateInfo alloc_i = { 0 };
    alloc_i.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_i.allocationSize = mem_req.size;
    alloc_i.memoryTypeIndex = bs_queryMemoryType(mem_req.memoryTypeBits, properties);

    BS_VK_ERR(vkAllocateMemory(bs_vkDevice(), &alloc_i, NULL, buffer_mem), "Failed to allocate buffer memory");

    vkBindBufferMemory(bs_vkDevice(), *buffer, *buffer_mem, 0);
}

void bs_copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    VkCommandBufferAllocateInfo alloc_i = { 0 };
    alloc_i.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_i.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_i.commandPool = bs_vkCmdPool();
    alloc_i.commandBufferCount = 1;

    VkCommandBuffer command_buffer;
    vkAllocateCommandBuffers(bs_vkDevice(), &alloc_i, &command_buffer);

    VkCommandBufferBeginInfo begin_i = { 0 };
    begin_i.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_i.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(command_buffer, &begin_i);

    VkBufferCopy region = { 0 };
    region.size = size;
    vkCmdCopyBuffer(command_buffer, srcBuffer, dstBuffer, 1, &region);
    vkEndCommandBuffer(command_buffer);

    VkSubmitInfo submit_i = { 0 };
    submit_i.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_i.commandBufferCount = 1;
    submit_i.pCommandBuffers = &command_buffer;

    vkQueueSubmit(bs_vkGraphicsQueue(), 1, &submit_i, VK_NULL_HANDLE);
    vkQueueWaitIdle(bs_vkGraphicsQueue());
    vkFreeCommandBuffers(bs_vkDevice(), bs_vkCmdPool(), 1, &command_buffer);
}

void bs_pushBatch(bs_Batch* batch) {
    bs_U32 vertex_size = batch->vertex_buf.num_units * batch->vertex_buf.unit_size;
    bs_U32 index_size = batch->index_buf.num_units * batch->index_buf.unit_size;

    // staging buffer
    VkBuffer staging_buffer = VK_NULL_HANDLE;
    VkDeviceMemory staging_memory = VK_NULL_HANDLE;

    bs_prepareBuffer(
        vertex_size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
        &staging_buffer, &staging_memory
    );

    void* data;
    vkMapMemory(bs_vkDevice(), staging_memory, 0, vertex_size, 0, &data);
    memcpy(data, batch->vertex_buf.data, vertex_size);
    vkUnmapMemory(bs_vkDevice(), staging_memory);

    // vertex buffer
    VkBuffer vertex_buffer = VK_NULL_HANDLE;
    VkDeviceMemory vertex_memory = VK_NULL_HANDLE;

    bs_prepareBuffer(
        vertex_size, 
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        &vertex_buffer, &vertex_memory
    );

    bs_copyBuffer(staging_buffer, vertex_buffer, vertex_size);

    // index buffer
    VkBuffer index_buffer = VK_NULL_HANDLE;
    VkDeviceMemory index_memory = VK_NULL_HANDLE;

    vkMapMemory(bs_vkDevice(), staging_memory, 0, index_size, 0, &data);
    memcpy(data, batch->index_buf.data, (size_t) index_size);
    vkUnmapMemory(bs_vkDevice(), staging_memory);

    bs_prepareBuffer(
        index_size, 
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
        &index_buffer, &index_memory
    );

    bs_copyBuffer(staging_buffer, index_buffer, index_size);
    vkDestroyBuffer(bs_vkDevice(), staging_buffer, NULL);
    vkFreeMemory(bs_vkDevice(), staging_memory, NULL);

    batch->vbuffer = vertex_buffer;
    batch->ibuffer = index_buffer;
}

void bs_selectBatch(bs_Batch* batch) {
    VkDeviceSize offsets[] = { 0 };

    VkCommandBuffer command_buffer = bs_vkHandle(bs_handleOffsets()->command_buffers + bs_frameData()->swapchain_frame);

    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, batch->pipeline.state);
    vkCmdBindVertexBuffers(command_buffer, 0, 1, &batch->vbuffer, offsets);
    vkCmdBindIndexBuffer(command_buffer, batch->ibuffer, 0, VK_INDEX_TYPE_UINT32);
}

void bs_renderBatch(bs_Batch* batch, bs_BatchPart range, bs_RenderType render_type) {
    bs_U32 offset = bs_handleOffsets()->command_buffers + bs_frameData()->swapchain_frame;
    vkCmdDrawIndexed(bs_vkHandle(offset), range.num, 1, range.offset, 0, 0);
}

// - renderer -
bs_Renderer bs_renderer(bs_U32 width, bs_U32 height) {
    bs_Renderer renderer = { 0 };
    renderer.dim = bs_v2(width, height);
    renderer.handle = bs_handleOffset();

    return renderer;
}

void bs_attach(bs_Renderer* renderer, bs_AttachmentType type) {
    bs_Attachment* attachment = renderer->attachments + renderer->num_attachments++;
    attachment->type = type;
}

void bs_pushRenderer(bs_Renderer* renderer) {
    // attachments
    VkAttachmentDescription attachments[BS_MAX_ATTACHMENTS] = { 0 };
    VkAttachmentReference color_refs[BS_MAX_ATTACHMENTS] = { 0 };
    bs_U32 num_color_refs = 0;
    VkAttachmentReference depth_ref = { 0 };

    for(int i = 0; i < renderer->num_attachments; i++) {
        VkAttachmentDescription* attachment = attachments + i;
        attachment->samples = VK_SAMPLE_COUNT_1_BIT;
        attachment->loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachment->storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment->stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment->stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment->initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachment->finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        switch(renderer->attachments[i].type) {
            // todo check if formats are supported
            case BS_NO_ATTACHMENTS: break;
            case BS_COLOR_ATTACHMENT: {
                color_refs[num_color_refs].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                color_refs[num_color_refs++].attachment = i;
                attachment->format = VK_FORMAT_B8G8R8A8_SRGB; 
            } break;
            case BS_DEPTH_ATTACHMENT: {
                depth_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                depth_ref.attachment = i;
                attachment->format = VK_FORMAT_D32_SFLOAT; 
            } break;
            default: bs_throw("Invalid attachment type");
        }
    }
    
    // render pass
    VkSubpassDescription subpass = { 0 };
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = num_color_refs;
    subpass.pColorAttachments = color_refs;
    subpass.pDepthStencilAttachment = depth_ref.layout == 0 ? NULL : &depth_ref;

    VkRenderPassCreateInfo render_pass_ci = { 0 };
    render_pass_ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_ci.attachmentCount = renderer->num_attachments;
    render_pass_ci.pAttachments = attachments;
    render_pass_ci.subpassCount = 1;
    render_pass_ci.pSubpasses = &subpass;

    BS_VK_ERR(vkCreateRenderPass(bs_vkDevice(), &render_pass_ci, NULL, &renderer->render_pass), "Failed to create render pass");

    // framebuffer
    for(int i = 0; i < bs_numSwapchainImgs(); i++) {
        VkFramebufferCreateInfo framebuf_ci = { 0 };
        framebuf_ci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuf_ci.renderPass = renderer->render_pass;
        framebuf_ci.attachmentCount = 1;
        framebuf_ci.pAttachments = (VkImageView*)bs_swapchainImgViews() + i;
        framebuf_ci.width = renderer->dim.x;
        framebuf_ci.height = renderer->dim.y;
        framebuf_ci.layers = 1;

        VkFramebuffer vk = VK_NULL_HANDLE;
        BS_VK_ERR(vkCreateFramebuffer(bs_vkDevice(), &framebuf_ci, NULL, &vk), "Failed to create framebuffer");

        bs_addVkHandle(vk);
    }
}

void bs_selectRenderer(bs_Renderer* renderer) {
    VkCommandBuffer command_buffer = bs_vkHandle(bs_handleOffsets()->command_buffers + bs_frameData()->swapchain_frame);

    VkRenderPassBeginInfo render_pass_i = { 0 };
    render_pass_i.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_i.renderPass = renderer->render_pass;
    render_pass_i.framebuffer = bs_vkHandle(renderer->handle + bs_swapchainImage());
    render_pass_i.renderArea.extent.width = renderer->dim.x;
    render_pass_i.renderArea.extent.height = renderer->dim.y;

    VkClearValue clear_color = {{{ 0.0f, 0.0f, 0.0f, 1.0f }}};
    render_pass_i.clearValueCount = 1;
    render_pass_i.pClearValues = &clear_color;
    vkCmdBeginRenderPass(command_buffer, &render_pass_i, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport = { 0 };
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = renderer->dim.x;
    viewport.height = renderer->dim.y;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(command_buffer, 0, 1, &viewport);

    VkRect2D scissor = { 0 };
    scissor.extent.width = renderer->dim.x;
    scissor.extent.height = renderer->dim.y;
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);
}

void bs_commitRenderer(bs_Renderer* renderer) {
    VkCommandBuffer command_buffer = bs_vkHandle(bs_handleOffsets()->command_buffers + bs_frameData()->swapchain_frame);
    vkCmdEndRenderPass(command_buffer);
}