// Basilisk
#include <bs_mem.h>
#include <bs_core.h>
#include <bs_shaders.h>
#include <bs_textures.h>
#include <bs_ini.h>
#include <bs_types.h>

// STD
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

#include <vulkan.h>

void bs_setVertexAttributes(bs_VertexShader* vs, const char* code, int code_len) {
    struct {
        char *name;
        bs_U32 name_len;
        bs_U8 format;
        bs_U8 value;
        uint8_t size;
    } attribs[] = {
        { "bs_Position", sizeof("bs_Position"), VK_FORMAT_R32G32B32_SFLOAT   , 1, sizeof(bs_vec3) },
        { "bs_Texture" , sizeof("bs_Texture" ), VK_FORMAT_R32G32_SFLOAT      , 2, sizeof(bs_vec2) },
        { "bs_Color"   , sizeof("bs_Color"   ), VK_FORMAT_R8G8B8A8_UNORM     , 4, sizeof(bs_RGBA) },
        { "bs_Normal"  , sizeof("bs_Normal"  ), VK_FORMAT_R32G32B32_SFLOAT   , 8, sizeof(bs_vec3) },
        { "bs_BoneId"  , sizeof("bs_BoneId"  ), VK_FORMAT_R32G32B32A32_SINT  , 16, sizeof(bs_ivec4) },
        { "bs_Weight"  , sizeof("bs_Weight"  ), VK_FORMAT_R32G32B32A32_SFLOAT, 32, sizeof(bs_vec4) },
        { "bs_Entity"  , sizeof("bs_Entity"  ), VK_FORMAT_R32_UINT           , 64, sizeof(bs_U32) },
        { "bs_Image"   , sizeof("bs_Image"   ), VK_FORMAT_R32_UINT           , 128, sizeof(bs_U32) }
    };   

    for(int i = 0; i < BS_NUM_ATTRIBUTES; i++) {
        uint8_t *attrib_sizes = (uint8_t *)&vs->attribs;
        uint8_t *attrib_size = attrib_sizes + i;
        *attrib_size = 0;

        if(bs_memmem(code, code_len, attribs[i].name, attribs[i].name_len)) {
            *attrib_size = attribs[i].size;

            vs->attributes[i].size = attribs[i].size;
            vs->attributes[i].format = attribs[i].format;
            vs->attrib_size_bytes += attribs[i].size;
            vs->attribs |= attribs[i].value;
            vs->attrib_count++;
        }
    }
}

static VkShaderModule bs_shaderModule(const char* spirv, int len) {
    VkShaderModuleCreateInfo shader_ci = { 0 };
    shader_ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_ci.codeSize = len - 1;
    shader_ci.pCode = spirv;

    VkShaderModule module;
    BS_VK_ERR(vkCreateShaderModule(bs_vkDevice(), &shader_ci, NULL, &module), "Failed to create shader module");
    return module;
}

bs_VertexShader bs_vertexShader(const char *path) {
    bs_VertexShader vs = { 0 };

    int len = 0;
    const char* spirv = bs_loadFile(path, &len);
    bs_setVertexAttributes(&vs, spirv, len);

    vs.module = bs_shaderModule(spirv, len);

    free(spirv);
    return vs;
}

bs_FragmentShader bs_fragmentShader(const char *path) {
    bs_FragmentShader fs = { 0 };

    int len = 0;
    const char* spirv = bs_loadFile(path, &len);

    fs.module = bs_shaderModule(spirv, len);

    free(spirv);
    return fs;
}

inline VkPipelineShaderStageCreateInfo bs_shaderStage(VkShaderModule module, VkShaderStageFlags flags) {
    VkPipelineShaderStageCreateInfo ci = { 0 };
    ci.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    ci.stage = flags;
    ci.module = module;
    ci.pName = "main";
    return ci;
}
VkVertexInputAttributeDescription attributes[BS_NUM_ATTRIBUTES] = { 0 };

bs_Pipeline bs_pipeline(bs_VertexShader* vs, bs_FragmentShader* fs) {
    bs_Pipeline pipeline = { 0 };
    pipeline.vs = vs;

    // pipeline
    VkDynamicState states[] = { 
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineLayout layout = VK_NULL_HANDLE;
    VkPipelineLayoutCreateInfo pipeline_layout_i = { 0 };
    pipeline_layout_i.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    BS_VK_ERR(vkCreatePipelineLayout(bs_vkDevice(), &pipeline_layout_i, NULL, &layout), "Failed to create pipeline layout");

    VkPipelineDynamicStateCreateInfo dynamic_state_i = { 0 };
    dynamic_state_i.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state_i.dynamicStateCount = sizeof(states) / sizeof(VkDynamicState);
    dynamic_state_i.pDynamicStates = states;

    // add attributes
    VkVertexInputBindingDescription input_binding = { 0 };
    input_binding.binding = 0;
    input_binding.stride = pipeline.vs->attrib_size_bytes;
    input_binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    bs_U32 offset = 0, location = 0;
    for(int i = 0, j = 1; i < BS_NUM_ATTRIBUTES; i++, j *= 2) {
        if ((pipeline.vs->attribs & j) != j) continue;

        attributes[location].binding = 0;
        attributes[location].location = location;
        attributes[location].format = pipeline.vs->attributes[i].format;
        attributes[location].offset = offset;
        offset += pipeline.vs->attributes[i].size;
        location++;
    }

    VkPipelineVertexInputStateCreateInfo vertex_ci = { 0 };
    vertex_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_ci.vertexBindingDescriptionCount = 1;
    vertex_ci.vertexAttributeDescriptionCount = pipeline.vs->attrib_count; 
    vertex_ci.pVertexBindingDescriptions = &input_binding; 
    vertex_ci.pVertexAttributeDescriptions = attributes; 

    VkPipelineInputAssemblyStateCreateInfo assembly_ci = { 0 };
    assembly_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    assembly_ci.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkViewport viewport = { 0 };
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)bs_swapchainExtents().x;
    viewport.height = (float)bs_swapchainExtents().y;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = { 0 };
    scissor.extent.width = bs_swapchainExtents().x;
    scissor.extent.height = bs_swapchainExtents().y;

    VkPipelineViewportStateCreateInfo viewport_state_ci = { 0 };
    viewport_state_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state_ci.viewportCount = 1;
    viewport_state_ci.pViewports = &viewport;
    viewport_state_ci.scissorCount = 1;
    viewport_state_ci.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer_ci = { 0 };
    rasterizer_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer_ci.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer_ci.lineWidth = 1.0f;
    rasterizer_ci.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer_ci.frontFace = VK_FRONT_FACE_CLOCKWISE;

    VkPipelineMultisampleStateCreateInfo multisampling_ci = { 0 };
    multisampling_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling_ci.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling_ci.minSampleShading = 1.0f;

    VkPipelineColorBlendAttachmentState blend_state = { 0 };
    blend_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    blend_state.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    blend_state.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    blend_state.colorBlendOp = VK_BLEND_OP_ADD;
    blend_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    blend_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    blend_state.alphaBlendOp = VK_BLEND_OP_ADD;
    blend_state.blendEnable = VK_TRUE;
    blend_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blend_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blend_state.colorBlendOp = VK_BLEND_OP_ADD;
    blend_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    blend_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    blend_state.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo color_blending_ci = { 0 };
    color_blending_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blending_ci.logicOp = VK_LOGIC_OP_COPY;
    color_blending_ci.attachmentCount = 1;
    color_blending_ci.pAttachments = &blend_state;

    VkPipelineShaderStageCreateInfo shader_stages[] = { 
        bs_shaderStage(vs->module, VK_SHADER_STAGE_VERTEX_BIT), 
        bs_shaderStage(fs->module, VK_SHADER_STAGE_FRAGMENT_BIT)
    };

    VkGraphicsPipelineCreateInfo pipeline_ci = { 0 };
    pipeline_ci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_ci.stageCount = 2;
    pipeline_ci.pStages = shader_stages;
    pipeline_ci.pVertexInputState = &vertex_ci;
    pipeline_ci.pInputAssemblyState = &assembly_ci;
    pipeline_ci.pViewportState = &viewport_state_ci;
    pipeline_ci.pRasterizationState = &rasterizer_ci;
    pipeline_ci.pMultisampleState = &multisampling_ci;
    pipeline_ci.pColorBlendState = &color_blending_ci;
    pipeline_ci.pDynamicState = &dynamic_state_i;
    pipeline_ci.layout = layout;
    pipeline_ci.renderPass = bs_vkRenderPass();
    pipeline_ci.subpass = 0;
    pipeline_ci.basePipelineIndex = -1;

    BS_VK_ERR(vkCreateGraphicsPipelines(bs_vkDevice(), VK_NULL_HANDLE, 1, &pipeline_ci, NULL, (VkPipeline*)&pipeline.state), "Failed to create graphics pipeline");

    vkDestroyShaderModule(bs_vkDevice(), vs->module, NULL);
    vkDestroyShaderModule(bs_vkDevice(), fs->module, NULL);

    return pipeline;
}