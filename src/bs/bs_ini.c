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

VkInstance instance;
VkSurfaceKHR surface;

VkPhysicalDeviceFeatures device_features = {0};
VkPhysicalDevice physical_device = VK_NULL_HANDLE;
VkDevice device;

VkQueue present_queue;

VkFence render_fence;
VkSwapchainKHR swapchain;

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

    free(queue_families);

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

    free(layers);

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

    free(devices);

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

    free(queue_cis);

    vkGetDeviceQueue(device, queue_family_indices.present_family, 0, &present_queue);
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
    VkSwapchainCreateInfoKHR swapchain_ci = { 0 };
    swapchain_ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_ci.surface = surface;
	swapchain_ci.imageArrayLayers = 1;
	swapchain_ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchain_ci.minImageCount = bs_clamp(2, capabilities.minImageCount, capabilities.maxImageCount);
	swapchain_ci.imageExtent.width = bs_clamp(w, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
	swapchain_ci.imageExtent.height = bs_clamp(h, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
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

	free(formats);
}

void bs_prepareCommands() {
    VkCommandPoolCreateInfo pool_ci = { 0 };
	pool_ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_ci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	pool_ci.queueFamilyIndex = queue_family_indices.graphics_family;

    VkCommandPool pool;
    BS_VK_ERR(vkCreateCommandPool(device, &pool_ci, NULL, &pool), "Failed to create a command pool");

    VkCommandBufferAllocateInfo buffer_ci = { 0 };
    buffer_ci.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    buffer_ci.commandPool = pool;
    buffer_ci.commandBufferCount = 1;
    buffer_ci.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    VkCommandBuffer buffer = { 0 };
    BS_VK_ERR(vkAllocateCommandBuffers(device, &buffer_ci, &buffer), "Failed to allocate command buffers");
}

void bs_prepareSynchronization() {
    VkFenceCreateInfo fence_ci = { 0 };
    fence_ci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_ci.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    
    VkSemaphoreCreateInfo semaphore_ci = { 0 };
    semaphore_ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphore_ci.flags = 0;

    BS_VK_ERR(vkCreateFence(device, &fence_ci, NULL, &render_fence), "Failed to create fence");

    VkSemaphore swapchain_semaphore, render_semaphore;
    BS_VK_ERR(vkCreateSemaphore(device, &semaphore_ci, NULL, &swapchain_semaphore), "Failed to create semaphore");
    BS_VK_ERR(vkCreateSemaphore(device, &semaphore_ci, NULL, &render_semaphore), "Failed to create semaphore");
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
    bs_prepareCommands();
    bs_prepareSynchronization();
}

void bs_run(void (*tick)()) {
	while (!glfwWindowShouldClose(window.glfw)) {

		frame.elapsed = glfwGetTime();
		frame.delta_time = frame.elapsed - last_frame.elapsed;

		glfwPollEvents();

		//vkWaitForFences(device, 1, &render_fence, true, 1000000000);
    	//vkResetFences(device, 1, &render_fence);

    	//vkAcquireNextImageKHR(device, )

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