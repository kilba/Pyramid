#include <windows.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

#include <vulkan.h>

#include <glfw/glfw3.h>
#include <glfw/glfw3native.h>

#include <bs_types.h>
#include <bs_math.h>
#include <bs_ini.h>
#include <bs_mem.h>

typedef struct {
	bs_vec2 resolution;

	double elapsed;
	double delta_time;

	bool key_states[BS_NUM_KEYS + 1];
} bs_WindowFrameData;

struct {
	GLFWwindow* glfw;
	bool second_input_poll;
} window = { 0 };

bs_WindowFrameData frame = { 0 };
bs_WindowFrameData last_frame = { 0 };

void bs_throw(const char* message) {
	printf("%s\n", message);
	exit(1);
}

void bs_throwVk(const char* message, VkResult result) {
	printf("%s (%d)\n", message, result);
	exit(1);
}

#define BS_VK_ERR(call, msg) \
    do { \
        if ((call) != VK_SUCCESS) { \
            bs_throwVk(msg, call); \
        } \
    } while(0)

VkInstance instance = VK_NULL_HANDLE;
VkSurfaceKHR surface = VK_NULL_HANDLE;

VkPhysicalDeviceFeatures device_features = {0};
VkPhysicalDevice physical_device = VK_NULL_HANDLE;
VkDevice device = VK_NULL_HANDLE;

VkQueue present_queue = VK_NULL_HANDLE;
VkQueue graphics_queue = VK_NULL_HANDLE;

VkFence* render_fences = NULL;
VkSwapchainKHR swapchain = VK_NULL_HANDLE;
VkSemaphore* swapchain_semaphores = NULL, *render_semaphores = NULL;

bs_ivec2 swapchain_extent = { 0, 0 };
VkFramebuffer* swapchain_framebufs = NULL;

VkRenderPass render_pass = VK_NULL_HANDLE;
VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
VkAttachmentDescription color_attachment = { 0 };

VkPipeline graphics_pipeline = VK_NULL_HANDLE;

VkImageView* swapchain_img_views = NULL;
VkImage* swapchain_imgs = NULL;
bs_U32 num_swapchain_imgs = 0;

VkFormat swapchain_img_format;

VkCommandBuffer* command_buffers = NULL;

#define BS_MAX_FRAMES_IN_FLIGHT 2

typedef struct {
    uint32_t family_count;

    uint32_t graphics_family;
    bool graphics_family_is_valid;

    uint32_t present_family;
    bool present_family_is_valid;
} QueueFamilyIndices;
QueueFamilyIndices queue_family_indices;

bool enable_validation_layers = true;
const char *validation_layers[] = {
    "VK_LAYER_KHRONOS_validation"
};

static void bs_glfwErrorCallback(int error, const char* description) {
	bs_Error error_log = { 0 };
	error_log.code = error;
	error_log.description = description;
}

static void bs_glfwInputCallback(GLFWwindow* glfw, int key, int scancode, int action, int mods) {
	if (key < 0 || key >= BS_NUM_KEYS) return;

	if ((action == GLFW_PRESS || action == GLFW_REPEAT)) {
		frame.key_states[key] = true;
	}
	else if (action == GLFW_RELEASE) {
		frame.key_states[key] = false;
	}
}

QueueFamilyIndices bs_findQueueFamilies() {
    QueueFamilyIndices indices;
    indices.graphics_family_is_valid = false;
    indices.present_family_is_valid = false;
    indices.family_count = 2;

    uint32_t num_families;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &num_families, NULL);

    VkQueueFamilyProperties* queue_families = malloc(num_families * sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &num_families, queue_families);

    for(int i = 0; i < num_families; i++) {
        if(queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphics_family = i;
            indices.present_family_is_valid = true;
        }

        VkBool32 present_support = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &present_support);
        if(present_support) {
            indices.present_family = i;
            indices.present_family_is_valid = true;
        }
    }

    //free(queue_families);

    return indices;
}

bool bs_isDeviceSuitable() {
    QueueFamilyIndices indices = bs_findQueueFamilies();
    return indices.present_family_is_valid && indices.present_family_is_valid;
}

bool bs_checkValidationLayerSupport() {
    uint32_t num_layers;
    vkEnumerateInstanceLayerProperties(&num_layers, NULL);

    VkLayerProperties* layers = malloc(num_layers * sizeof(VkLayerProperties));
    vkEnumerateInstanceLayerProperties(&num_layers, layers);

    int num_validation_layers = sizeof(validation_layers) / sizeof(const char *);
    for(int i = 0; i < num_validation_layers; i++) {
        bool layerFound = false;

        for(int j = 0; j < num_layers; j++) {
            if(strcmp(validation_layers[i], layers[j].layerName) == 0) {
                layerFound = true;
                break;
            }
	    }

        if(!layerFound) {
            return false;
        }
    }

    //free(layers);

    return true;
}

// initialization
void bs_prepareInstance() {
    if(enable_validation_layers && !bs_checkValidationLayerSupport()) {
        printf("The validation layers requested are not available!\n");
        return;
    }

	int num_extensions = 0;
	const char** extensions = glfwGetRequiredInstanceExtensions(&num_extensions);

    VkApplicationInfo app_i = {0};
    app_i.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_i.pApplicationName = "Hello Triangle";
    app_i.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_i.pEngineName = "No Engine";
    app_i.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_i.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo ci = { 0 };
    ci.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    ci.pApplicationInfo = &app_i;
    ci.enabledExtensionCount = num_extensions;
    ci.ppEnabledExtensionNames = extensions;

    if(enable_validation_layers) {
        ci.enabledLayerCount = sizeof(validation_layers) / sizeof(const char *);
        ci.ppEnabledLayerNames = validation_layers;
    } else {
	    ci.enabledLayerCount = 0;
    }

    BS_VK_ERR(vkCreateInstance(&ci, NULL, &instance), "Instance creation failed");
}

void bs_selectPhysicalDevice() {
    uint32_t num_devices = 0;
    vkEnumeratePhysicalDevices(instance, &num_devices, NULL);
    if(num_devices == 0) {
	    printf("No GPU with Vulkan support was found!\n");
        exit(1);
    }

    VkPhysicalDevice* devices = malloc(num_devices * sizeof(VkPhysicalDevice));
    vkEnumeratePhysicalDevices(instance, &num_devices, devices);

    for(int i = 0; i < num_devices; i++) {
		physical_device = devices[i];
        if(bs_isDeviceSuitable()) {
            break;
        }
    }

    //free(devices);

    if(physical_device == VK_NULL_HANDLE) {
        printf("Failed to find a suitable GPU!\n");
        return;
    }
}

void bs_prepareLogicalDevice() {
    queue_family_indices = bs_findQueueFamilies(physical_device);

    VkDeviceQueueCreateInfo* queue_cis = malloc(queue_family_indices.family_count * sizeof(VkDeviceQueueCreateInfo));
    uint32_t uq_families[] = { queue_family_indices.graphics_family, queue_family_indices.present_family };
    uint32_t num_uq_families = queue_family_indices.family_count;

    // remove dupes
    for(int i = 0; i < num_uq_families; i++){
       for(int j = i + 1; j < num_uq_families; j++){
            if(uq_families[i] == uq_families[j]){
                for(int k = j; k < num_uq_families; k++) {
                    uq_families[k] = uq_families[k + 1];
                }

                j--;
                num_uq_families--;
            }
       }
    }

    float queue_priority = 1.0;
    for(int i = 0; i < num_uq_families; i++) {
        VkDeviceQueueCreateInfo queue_ci = {0};
        queue_ci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_ci.queueFamilyIndex = uq_families[i];
        queue_ci.queueCount = 1;
        queue_ci.pQueuePriorities = &queue_priority;

        queue_cis[i] = queue_ci;
    }

	const char* extensions[] = {
        "VK_KHR_swapchain"
    };

    VkDeviceCreateInfo ci = {0};
    ci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    ci.pQueueCreateInfos = queue_cis;
    ci.queueCreateInfoCount = num_uq_families;
    ci.pEnabledFeatures = &device_features;
    ci.enabledExtensionCount = sizeof(extensions) / sizeof(const char *);
    ci.ppEnabledExtensionNames = extensions;
    if(enable_validation_layers) {
        ci.enabledLayerCount = sizeof(validation_layers) / sizeof(const char *);
        ci.ppEnabledLayerNames = validation_layers;
    } else {
	    ci.enabledLayerCount = 0;
    }

    BS_VK_ERR(vkCreateDevice(physical_device, &ci, NULL, &device), "Failed to create logical device");

    //free(queue_cis);

    vkGetDeviceQueue(device, queue_family_indices.present_family, 0, &present_queue);
    vkGetDeviceQueue(device, queue_family_indices.graphics_family, 0, &graphics_queue);
}

void bs_prepareSwapchain() {
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &capabilities);

	int w, h;
	glfwGetFramebufferSize(window.glfw, &w, &h);

	// format query
	bs_U32 num_formats;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &num_formats, NULL);	

	VkSurfaceFormatKHR* formats = malloc(num_formats * sizeof(VkSurfaceFormatKHR));
	vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &num_formats, formats);

	// present mode query
	bs_U32 num_modes;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &num_modes, NULL);

	VkPresentModeKHR* modes = malloc(num_modes * sizeof(VkPresentModeKHR));
	vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &num_modes, modes);

	bs_U32 indices[2] = { queue_family_indices.graphics_family, queue_family_indices.present_family };

	// creation
    swapchain_extent = bs_iv2(
        bs_clamp(w, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
        bs_clamp(h, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
    );

    VkSwapchainCreateInfoKHR swapchain_ci = { 0 };
    swapchain_ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_ci.surface = surface;
	swapchain_ci.imageArrayLayers = 1;
	swapchain_ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchain_ci.minImageCount = bs_clamp(2, capabilities.minImageCount, capabilities.maxImageCount);
	swapchain_ci.imageExtent.width = swapchain_extent.x;
	swapchain_ci.imageExtent.height = swapchain_extent.y;
	swapchain_ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchain_ci.imageSharingMode = (queue_family_indices.graphics_family == queue_family_indices.present_family) ?
		VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT;
	swapchain_ci.queueFamilyIndexCount = (queue_family_indices.graphics_family == queue_family_indices.present_family) ?
		0 : 2;
	swapchain_ci.pQueueFamilyIndices = (queue_family_indices.graphics_family == queue_family_indices.present_family) ?
		NULL : indices;
	swapchain_ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchain_ci.clipped = VK_TRUE;
	swapchain_ci.preTransform = capabilities.currentTransform;

	for(int i = 0; i < num_formats; i++) {
		VkSurfaceFormatKHR* format = formats + i;
		if(format->format == VK_FORMAT_B8G8R8A8_SRGB && format->colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			swapchain_ci.imageFormat = VK_FORMAT_B8G8R8A8_SRGB;
			swapchain_ci.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
			break;
		}
	}

    swapchain_img_format = swapchain_ci.imageFormat;
    swapchain_img_views = malloc(num_swapchain_imgs * sizeof(VkImageView));

	for(int i = 0; i < num_modes; i++) {
		if(modes[i] == VK_PRESENT_MODE_FIFO_KHR) {
			// VK_PRESENT_MODE_FIFO_KHR = double buffering
			// VK_PRESENT_MODE_MAILBOX_KHR = triple buffering
			swapchain_ci.presentMode = VK_PRESENT_MODE_FIFO_KHR;
			break;
		}
	}

	if(swapchain_ci.imageFormat == VK_FORMAT_UNDEFINED) {
		bs_throw("Unable to find an image format or color space");
	}

	BS_VK_ERR(vkCreateSwapchainKHR(device, &swapchain_ci, NULL, &swapchain), "Failed to create swapchain");

	// get images
	vkGetSwapchainImagesKHR(device, swapchain, &num_swapchain_imgs, NULL);
    swapchain_imgs = malloc(num_swapchain_imgs * sizeof(VkImage));
    vkGetSwapchainImagesKHR(device, swapchain, &num_swapchain_imgs, swapchain_imgs);

	//free(formats);
}

void bs_prepareImageViews() {
    for(int i = 0; i < num_swapchain_imgs; i++) {
        VkImageViewCreateInfo image_view_ci = { 0 };
        image_view_ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        image_view_ci.image = swapchain_imgs[i];
        image_view_ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
        image_view_ci.format = swapchain_img_format;
        image_view_ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_view_ci.subresourceRange.levelCount = 1;
        image_view_ci.subresourceRange.layerCount = 1;

        BS_VK_ERR(vkCreateImageView(device, &image_view_ci, NULL, swapchain_img_views + i), "Failed to create image view");
    }
}

VkShaderModule bs_spirv(const char* path) {
    VkShaderModule module;

    int len = 0;
    const char* spirv = bs_loadFile(path, &len);

    VkShaderModuleCreateInfo shader_ci = { 0 };
    shader_ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_ci.codeSize = len - 1;
    shader_ci.pCode = spirv;

    BS_VK_ERR(vkCreateShaderModule(device, &shader_ci, NULL, &module), "Failed to load spir-v");

    return module;
}

void bs_shaderv(const char* vs_path, const char* fs_path) {
    VkShaderModule vs = bs_spirv(vs_path);
    VkShaderModule fs = bs_spirv(fs_path);

    VkPipelineShaderStageCreateInfo vs_ci = { 0 };
    vs_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vs_ci.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vs_ci.module = vs;
    vs_ci.pName = "main";

    VkPipelineShaderStageCreateInfo fs_ci = { 0 };
    fs_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fs_ci.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fs_ci.module = fs;
    fs_ci.pName = "main";

    // pipeline
    VkDynamicState states[] = { 
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineLayoutCreateInfo pipeline_layout_i = { 0 };
    pipeline_layout_i.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    BS_VK_ERR(vkCreatePipelineLayout(device, &pipeline_layout_i, NULL, &pipeline_layout), "Failed to create pipeline layout");

    VkPipelineDynamicStateCreateInfo dynamic_state_i = { 0 };
    dynamic_state_i.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state_i.dynamicStateCount = sizeof(states) / sizeof(VkDynamicState);
    dynamic_state_i.pDynamicStates = states;

    VkPipelineVertexInputStateCreateInfo vertex_ci = { 0 };
    vertex_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    VkPipelineInputAssemblyStateCreateInfo assembly_ci = { 0 };
    assembly_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    assembly_ci.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkViewport viewport = { 0 };
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)swapchain_extent.x;
    viewport.height = (float)swapchain_extent.y;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = { 0 };
    scissor.extent.width = swapchain_extent.x;
    scissor.extent.height = swapchain_extent.y;

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

    VkPipelineShaderStageCreateInfo shader_stages[] = { vs_ci, fs_ci };
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
    pipeline_ci.layout = pipeline_layout;
    pipeline_ci.renderPass = render_pass;
    pipeline_ci.subpass = 0;
    pipeline_ci.basePipelineIndex = -1;

    BS_VK_ERR(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_ci, NULL, &graphics_pipeline), "Failed to create graphics pipeline");

    vkDestroyShaderModule(device, vs, NULL);
    vkDestroyShaderModule(device, fs, NULL);
}

void bs_prepareRenderPass() {
    color_attachment.format = swapchain_img_format;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachment_ref = { 0 };
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = { 0 };
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;

    VkRenderPassCreateInfo render_pass_ci = { 0 };
    render_pass_ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_ci.attachmentCount = 1;
    render_pass_ci.pAttachments = &color_attachment;
    render_pass_ci.subpassCount = 1;
    render_pass_ci.pSubpasses = &subpass;

    BS_VK_ERR(vkCreateRenderPass(device, &render_pass_ci, NULL, &render_pass), "Failed to create render pass");
}

void bs_prepareFramebuffer() {
    swapchain_framebufs = malloc(num_swapchain_imgs * sizeof(VkFramebuffer));

    for(int i = 0; i < num_swapchain_imgs; i++) {
        VkFramebufferCreateInfo framebuf_ci = { 0 };
        framebuf_ci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuf_ci.renderPass = render_pass;
        framebuf_ci.attachmentCount = 1;
        framebuf_ci.pAttachments = swapchain_img_views + i;
        framebuf_ci.width = swapchain_extent.x;
        framebuf_ci.height = swapchain_extent.y;
        framebuf_ci.layers = 1;

        BS_VK_ERR(vkCreateFramebuffer(device, &framebuf_ci, NULL, swapchain_framebufs + i), "Failed to create framebuffer");
    }
}

void bs_preparePipeline() {
    bs_shaderv("tri_vs.spv", "tri_fs.spv");
}

bs_U32 current_frame = 0;
void bs_recordCommands(VkCommandBuffer cmd_buf, int img_idx) {
    VkCommandBufferBeginInfo ci = { 0 };
    ci.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    BS_VK_ERR(vkBeginCommandBuffer(cmd_buf, &ci), "Failed to begin recording");

    VkRenderPassBeginInfo render_pass_i = { 0 };
    render_pass_i.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_i.renderPass = render_pass;
    render_pass_i.framebuffer = swapchain_framebufs[img_idx];
    render_pass_i.renderArea.extent.width = swapchain_extent.x;
    render_pass_i.renderArea.extent.height = swapchain_extent.y;

    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    render_pass_i.clearValueCount = 1;
    render_pass_i.pClearValues = &clearColor;
    vkCmdBeginRenderPass(cmd_buf, &render_pass_i, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);
    VkViewport viewport = { 0 };
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = swapchain_extent.x;
    viewport.height = swapchain_extent.y;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd_buf, 0, 1, &viewport);

    VkRect2D scissor = { 0 };
    scissor.extent.width = swapchain_extent.x;
    scissor.extent.height = swapchain_extent.y;
    vkCmdSetScissor(cmd_buf, 0, 1, &scissor);

    vkCmdDraw(cmd_buf, 3, 1, 0, 0);

    vkCmdEndRenderPass(cmd_buf);
    BS_VK_ERR(vkEndCommandBuffer(cmd_buf), "Failed to record commands");
}

void bs_prepareCommands() {
    VkCommandPoolCreateInfo pool_ci = { 0 };
	pool_ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_ci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	pool_ci.queueFamilyIndex = queue_family_indices.graphics_family;

    VkCommandPool pool;
    BS_VK_ERR(vkCreateCommandPool(device, &pool_ci, NULL, &pool), "Failed to create a command pool");

    command_buffers = malloc(BS_MAX_FRAMES_IN_FLIGHT * sizeof(VkCommandBuffer));

    VkCommandBufferAllocateInfo buffer_ci = { 0 };
    buffer_ci.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    buffer_ci.commandPool = pool;
    buffer_ci.commandBufferCount = BS_MAX_FRAMES_IN_FLIGHT;
    buffer_ci.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    BS_VK_ERR(vkAllocateCommandBuffers(device, &buffer_ci, command_buffers), "Failed to allocate command buffers");
}

void bs_prepareSynchronization() {
    VkFenceCreateInfo fence_ci = { 0 };
    fence_ci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_ci.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkSemaphoreCreateInfo semaphore_ci = { 0 };
    semaphore_ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    swapchain_semaphores = malloc(BS_MAX_FRAMES_IN_FLIGHT * sizeof(VkSemaphore));
    render_semaphores = malloc(BS_MAX_FRAMES_IN_FLIGHT * sizeof(VkSemaphore));
    render_fences = malloc(BS_MAX_FRAMES_IN_FLIGHT * sizeof(VkFence));

    for(int i = 0; i < BS_MAX_FRAMES_IN_FLIGHT; i++) {
        BS_VK_ERR(vkCreateFence(device, &fence_ci, NULL, render_fences + i), "Failed to create fence");
        BS_VK_ERR(vkCreateSemaphore(device, &semaphore_ci, NULL, swapchain_semaphores + i), "Failed to create semaphore");
        BS_VK_ERR(vkCreateSemaphore(device, &semaphore_ci, NULL, render_semaphores + i), "Failed to create semaphore");
    }
}

void bs_tmpDraw() {

    vkWaitForFences(device, 1, render_fences + current_frame, VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, render_fences + current_frame);

    uint32_t img_idx;
    vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, swapchain_semaphores[current_frame], VK_NULL_HANDLE, &img_idx);

    vkResetCommandBuffer(command_buffers[current_frame], 0);
    bs_recordCommands(command_buffers[current_frame], img_idx);

    VkSemaphore wait_semaphores[] = { swapchain_semaphores[current_frame] };
    VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSemaphore signal_semaphores[] = { render_semaphores[current_frame] };

    VkSubmitInfo submit_i = { 0 };
    submit_i.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_i.waitSemaphoreCount = 1;
    submit_i.pWaitSemaphores = wait_semaphores;
    submit_i.pSignalSemaphores = signal_semaphores;
    submit_i.signalSemaphoreCount = 1;
    submit_i.pWaitDstStageMask = wait_stages;
    submit_i.commandBufferCount = 1;
    submit_i.pCommandBuffers = command_buffers + current_frame;

    BS_VK_ERR(vkQueueSubmit(graphics_queue, 1, &submit_i, render_fences[current_frame]), "Failed to submit queue");
    VkSubpassDependency dependency = { 0 };
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo render_pass_info;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &dependency;

    VkSwapchainKHR swapchains[] = { swapchain };
    VkPresentInfoKHR present_i = { 0 };
    present_i.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_i.waitSemaphoreCount = 1;
    present_i.pWaitSemaphores = signal_semaphores;
    present_i.swapchainCount = 1;
    present_i.pSwapchains = swapchains;
    present_i.pImageIndices = &img_idx;

    vkQueuePresentKHR(present_queue, &present_i);

    current_frame = (current_frame + 1) % BS_MAX_FRAMES_IN_FLIGHT;
}

void bs_ini(bs_U32 width, bs_U32 height, const char* name) {
	assert(CHAR_BIT * sizeof(float) == 32);
	assert(CHAR_BIT * sizeof(char) == 8);

	frame.resolution = bs_v2(width, height);

	// glfw
	if (!glfwInit()) {
		bs_throw("Failed to initialize glfw");
	}

	glfwSetErrorCallback(bs_glfwErrorCallback);

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	window.glfw = glfwCreateWindow(frame.resolution.x, frame.resolution.y, name, NULL, NULL);
	if (window.glfw == NULL) {
		glfwTerminate();
		bs_throw("Failed to create window");
	}

	glfwSetKeyCallback(window.glfw, bs_glfwInputCallback);
	glfwSwapInterval(1);

	// vulkan
	bs_prepareInstance();

	BS_VK_ERR(glfwCreateWindowSurface(instance, window.glfw, NULL, &surface), "Failed to create surface");

    bs_selectPhysicalDevice();
    bs_prepareLogicalDevice();
    bs_prepareSwapchain();
    bs_prepareImageViews();
    bs_prepareRenderPass();
    bs_prepareFramebuffer();
    bs_preparePipeline();
    bs_prepareCommands();
    bs_prepareSynchronization();

    // vKDestroyPipelineLayout
    // vKDestroyRenderPass
    // vKDestroyFramebuffer
    // vkDestroyCommandPool
}

void bs_run(void (*tick)()) {
	while (!glfwWindowShouldClose(window.glfw)) {

		frame.elapsed = glfwGetTime();
		frame.delta_time = frame.elapsed - last_frame.elapsed;

		glfwPollEvents();

        bs_tmpDraw();		

		tick();

		glfwSwapBuffers(window.glfw);

		last_frame = frame;
	}

	glfwTerminate();
}

void bs_exit() {
	glfwSetWindowShouldClose(window.glfw, GLFW_TRUE);
}

double bs_deltaTime() {
	return frame.delta_time;
}

double bs_elapsedTime() {
	return frame.elapsed;
}

bs_vec2 bs_resolution() {
	return frame.resolution;
}

bool bs_keyDown(bs_U32 code) {
	if (code > BS_NUM_KEYS) return false;
	return frame.key_states[code];
}

bool bs_keyDownOnce(bs_U32 code) {
	if (code > BS_NUM_KEYS) return false;
	return frame.key_states[code] && !last_frame.key_states[code];
}

bool bs_keyUpOnce(bs_U32 code) {
	if (code > BS_NUM_KEYS) return false;
	return !frame.key_states[code] && last_frame.key_states[code];
}

void bs_wndTitle(const char* title) {
	glfwSetWindowTitle(window.glfw, title);
}

void bs_wndTitlef(const char* format, ...) {
	va_list argptr;
	va_start(argptr, format);

	char buf[512];
	vsprintf(buf, format, argptr);

	bs_wndTitle(buf);

	va_end(argptr);
}

HWND bs_hwnd() {
	return glfwGetWin32Window(window.glfw);
}

bs_PerformanceData bs_queryPerformance() {
	bs_PerformanceData data = { 0 };

	QueryPerformanceFrequency(&data.frequency);
	QueryPerformanceCounter(&data.start);

	return data;
}

double bs_queryPerformanceResult(bs_PerformanceData data) {
	QueryPerformanceCounter(&data.end);
	return (double)(data.end - data.start) / data.frequency;
}