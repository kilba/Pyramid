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
#include <bs_shaders.h>

bs_HandleOffsets handle_offsets = { 0 };

struct {
	GLFWwindow* glfw;
	bool second_input_poll;
    bool resized;
} window = { 0 };

bs_WindowFrame frame = { 0 };
bs_WindowFrame last_frame = { 0 };


bs_WindowFrame* bs_frameData() {
    return &frame;
}

void bs_throw(const char* message) {
	printf("%s\n", message);
	exit(1);
}

void bs_throwVk(const char* message, bs_U32 result) {
	printf("%s (%d)\n", message, result);
	exit(1);
}

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

VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;

VkImageView* swapchain_img_views = NULL;
VkImage* swapchain_imgs = NULL;
bs_U32 num_swapchain_imgs = 0;

VkFormat swapchain_img_format;
VkCommandPool command_pool;

#define BS_MAX_FRAMES_IN_FLIGHT 2

bs_Buffer vk_handles = { 0 };
bs_U32 current_swapchain_img = 0;

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

bs_U32 bs_swapchainImage() {
    return current_swapchain_img;
}

void* bs_vkDevice() {
    return (void*)device;
}

void* bs_vkPhysicalDevice() {
    return (void*)physical_device;
}

void* bs_vkCmdPool() {
    return (void*)command_pool;
}

void* bs_vkGraphicsQueue() {
    return (void*)graphics_queue;
}

void* bs_swapchainImgViews() {
    return (void*)swapchain_img_views;
}

bs_U32 bs_numSwapchainImgs() {
    return num_swapchain_imgs;
}

bs_ivec2 bs_swapchainExtents() {
    return swapchain_extent;
}

bs_HandleOffsets* bs_handleOffsets() {
    return &handle_offsets;
}

void bs_addVkHandle(void* handle) {
    bs_bufferAppend(&vk_handles, &handle);
}

void* bs_vkHandle(bs_U32 index) {
    return *(void**)(vk_handles.data + index * sizeof(void*));
}

void** bs_vkHandleA(bs_U32 index) {
    return (void*)(vk_handles.data + index * sizeof(void*));
}

bs_U32 bs_handleOffset() {
    return vk_handles.num_units;
}

void bs_cleanupSwapChain() {
    for (size_t i = 0; i < num_swapchain_imgs; i++) {
      //  vkDestroyFramebuffer(device, swapchain_framebufs[i], NULL);
    }

    for (size_t i = 0; i < num_swapchain_imgs; i++) {
        vkDestroyImageView(device, swapchain_img_views[i], NULL);
    }

    vkDestroySwapchainKHR(device, swapchain, NULL);
}

void bs_cleanup() {
    bs_cleanupSwapChain();

    vkDestroyPipelineLayout(device, pipeline_layout, NULL);

    for (size_t i = 0; i < BS_MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(device, render_semaphores[i], NULL);
        vkDestroySemaphore(device, swapchain_semaphores[i], NULL);
        vkDestroyFence(device, render_fences[i], NULL);
    }

    // vkDestroyCommandPool(device, cmd_pool, NULL);

    vkDestroyDevice(device, NULL);

    if (enable_validation_layers) {
        // DestroyDebugUtilsMessengerEXT(instance, debug_messenger, NULL);
    }

    vkDestroySurfaceKHR(instance, surface, NULL);
    vkDestroyInstance(instance, NULL);

    //glfwDestroyWindow(window);
    //glfwTerminate();
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
    free(swapchain_img_views);
    free(swapchain_imgs);

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

    swapchain_img_views = malloc(num_swapchain_imgs * sizeof(VkImageView));
    swapchain_img_format = swapchain_ci.imageFormat;

	free(formats);
	free(modes);
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

void bs_prepareCommands() {
    VkCommandPoolCreateInfo pool_ci = { 0 };
	pool_ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_ci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	pool_ci.queueFamilyIndex = queue_family_indices.graphics_family;

    BS_VK_ERR(vkCreateCommandPool(device, &pool_ci, NULL, &command_pool), "Failed to create a command pool");

    bs_bufferResizeCheck(&vk_handles, BS_MAX_FRAMES_IN_FLIGHT);
    handle_offsets.command_buffers = bs_handleOffset();

    VkCommandBufferAllocateInfo buffer_ci = { 0 };
    buffer_ci.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    buffer_ci.commandPool = command_pool;
    buffer_ci.commandBufferCount = BS_MAX_FRAMES_IN_FLIGHT;
    buffer_ci.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    BS_VK_ERR(vkAllocateCommandBuffers(device, &buffer_ci, bs_vkHandleA(handle_offsets.command_buffers)), "Failed to allocate command buffers");
    vk_handles.num_units += BS_MAX_FRAMES_IN_FLIGHT;
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

void bs_recreateSwapchain() {
    int width = 0, height = 0;
    glfwGetFramebufferSize(window.glfw, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window.glfw, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(device);

    bs_cleanupSwapChain();

    bs_prepareSwapchain();
    bs_prepareImageViews();
}

void bs_render(void (*tick)()) {
    vkWaitForFences(device, 1, render_fences + frame.swapchain_frame, VK_TRUE, UINT64_MAX);

    VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, swapchain_semaphores[frame.swapchain_frame], VK_NULL_HANDLE, &current_swapchain_img);

    if(result == VK_ERROR_OUT_OF_DATE_KHR) {
        bs_recreateSwapchain();
        return;
    } else if(result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        bs_throw("Failed to acquire swapchain image");
    }
    
    VkCommandBuffer command_buffer = bs_vkHandle(handle_offsets.command_buffers + frame.swapchain_frame);
    vkResetFences(device, 1, render_fences + frame.swapchain_frame);
    vkResetCommandBuffer(command_buffer, 0);

    VkCommandBufferBeginInfo ci = { 0 };
    ci.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    BS_VK_ERR(vkBeginCommandBuffer(command_buffer, &ci), "Failed to begin recording");

    tick();

    //vkCmdEndRenderPass(command_buffer);
    BS_VK_ERR(vkEndCommandBuffer(command_buffer), "Failed to record commands");

    VkSemaphore wait_semaphores[] = { swapchain_semaphores[frame.swapchain_frame] };
    VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSemaphore signal_semaphores[] = { render_semaphores[frame.swapchain_frame] };

    VkSubmitInfo submit_i = { 0 };
    submit_i.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_i.waitSemaphoreCount = 1;
    submit_i.pWaitSemaphores = wait_semaphores;
    submit_i.pSignalSemaphores = signal_semaphores;
    submit_i.signalSemaphoreCount = 1;
    submit_i.pWaitDstStageMask = wait_stages;
    submit_i.commandBufferCount = 1;
    submit_i.pCommandBuffers = bs_vkHandleA(handle_offsets.command_buffers + frame.swapchain_frame);

    BS_VK_ERR(vkQueueSubmit(graphics_queue, 1, &submit_i, render_fences[frame.swapchain_frame]), "Failed to submit queue");
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
    present_i.pImageIndices = &current_swapchain_img;

    result = vkQueuePresentKHR(present_queue, &present_i);
    if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || window.resized) {
        window.resized = false;
        bs_recreateSwapchain();
    } else if(result != VK_SUCCESS) {
        bs_throw("Failed to acquire swapchain image");
    }

    frame.swapchain_frame = (frame.swapchain_frame + 1) % BS_MAX_FRAMES_IN_FLIGHT;
}

static void bs_glfwResizeCallback(GLFWwindow* window, int width, int height);
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
    glfwSetFramebufferSizeCallback(window.glfw, bs_glfwResizeCallback);
	glfwSwapInterval(1);

    vk_handles = bs_buffer(sizeof(void*), 16, 64, 0);

	// vulkan
	bs_prepareInstance();

	BS_VK_ERR(glfwCreateWindowSurface(instance, window.glfw, NULL, &surface), "Failed to create surface");

    bs_selectPhysicalDevice();
    bs_prepareLogicalDevice();
    bs_prepareSwapchain();
    bs_prepareImageViews();
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

        bs_render(tick);		

		glfwSwapBuffers(window.glfw);

		last_frame = frame;
	}

	glfwTerminate();
}

static void bs_glfwResizeCallback(GLFWwindow* glfw, int width, int height) {
    window.resized = true;
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