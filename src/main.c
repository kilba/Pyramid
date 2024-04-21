#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <wnd.h>

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan.h>

VkInstance instance;
VkSurfaceKHR surface;

VkPhysicalDeviceFeatures device_features = {0};
VkPhysicalDevice physical_device = VK_NULL_HANDLE;
VkDevice device;

VkQueue present_queue;

typedef struct {
    uint32_t family_count;

    uint32_t graphics_family;
    bool graphics_family_is_valid;

    uint32_t present_family;
    bool present_family_is_valid;
} QueueFamilyIndices;

WindowData wdata;

bool enable_validation_layers = true;
const char *validation_layers[] = {
    "VK_LAYER_KHRONOS_validation"
};

void tick() {
}

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
    QueueFamilyIndices indices;
    indices.graphics_family_is_valid = false;
    indices.present_family_is_valid = false;
    indices.family_count = 2;

    uint32_t num_families;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &num_families, NULL);

    VkQueueFamilyProperties* queue_families = malloc(num_families * sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(device, &num_families, queue_families);

    for(int i = 0; i < num_families; i++) {
        if(queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphics_family = i;
            indices.present_family_is_valid = true;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
        if(presentSupport) {
            indices.present_family = i;
            indices.present_family_is_valid = true;
        }
    }

    free(queue_families);

    return indices;
}

bool isDeviceSuitable(VkPhysicalDevice device) {
    QueueFamilyIndices indices = findQueueFamilies(device);
    return indices.present_family_is_valid && indices.present_family_is_valid;
}

bool checkValidationLayerSupport() {
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

/* --- Initialization --- */
void createInstance() {
    if(enable_validation_layers && !checkValidationLayerSupport()) {
        printf("The validation layers requested are not available!\n");
        return;
    }

    VkApplicationInfo appInfo = {0};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo ci = {0};
    const char *extensions[] = {
        "VK_KHR_surface",
        "VK_KHR_win32_surface"
    };

    ci.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    ci.pApplicationInfo = &appInfo;
    ci.enabledExtensionCount = 2;
    ci.ppEnabledExtensionNames = extensions;

    if(enable_validation_layers) {
        ci.enabledLayerCount = sizeof(validation_layers) / sizeof(const char *);
        ci.ppEnabledLayerNames = validation_layers;
    } else {
	    ci.enabledLayerCount = 0;
    }

    VkResult result = vkCreateInstance(&ci, NULL, &instance);
    if(result != VK_SUCCESS) {
	    printf("Instance creation failed\n");
    }
}

void createWindowSurface() {
    VkWin32SurfaceCreateInfoKHR ci = {0};
    ci.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    ci.hinstance = wdata.hinstance;
    ci.hwnd = wdata.hwnd;

    VkResult err = vkCreateWin32SurfaceKHR(instance, &ci, NULL, &surface);
    if(err != VK_SUCCESS) {
	    printf("Failed to create a window surface!\n");
    }
}

void debugMessenger() {
}

void selectPhysicalDevice() {
    uint32_t num_devices = 0;
    vkEnumeratePhysicalDevices(instance, &num_devices, NULL);
    if(num_devices == 0) {
	    printf("No GPU with Vulkan support was found!\n");
        exit(1);
    }

    VkPhysicalDevice* devices = malloc(num_devices * sizeof(VkPhysicalDevice));
    vkEnumeratePhysicalDevices(instance, &num_devices, devices);

    for(int i = 0; i < num_devices; i++) {
        if(isDeviceSuitable(devices[i])) {
            physical_device = devices[i];
            break;
        }
    }

    if(physical_device == VK_NULL_HANDLE) {
        printf("Failed to find a suitable GPU!\n");
        return;
    }
}

void createLogicalDevice() {
    QueueFamilyIndices indices = findQueueFamilies(physical_device);

    VkDeviceQueueCreateInfo* queue_cis = malloc(indices.family_count * sizeof(VkDeviceQueueCreateInfo));
    uint32_t uq_families[] = { indices.graphics_family, indices.present_family };
    uint32_t num_uq_families = indices.family_count;

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

    float queuePriority = 1.0;
    for(int i = 0; i < num_uq_families; i++) {
        VkDeviceQueueCreateInfo queue_ci = {0};
        queue_ci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_ci.queueFamilyIndex = uq_families[i];
        queue_ci.queueCount = 1;
        queue_ci.pQueuePriorities = &queuePriority;

        queue_cis[i] = queue_ci;
    }

    VkDeviceCreateInfo ci = {0};
    ci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    ci.pQueueCreateInfos = queue_cis;
    ci.queueCreateInfoCount = num_uq_families;
    ci.pEnabledFeatures = &device_features;
    ci.enabledExtensionCount = 0;
    
    if(enable_validation_layers) {
        ci.enabledLayerCount = sizeof(validation_layers) / sizeof(const char *);
        ci.ppEnabledLayerNames = validation_layers;
    } else {
	    ci.enabledLayerCount = 0;
    }

    VkResult err = vkCreateDevice(physical_device, &ci, NULL, &device);
    if(err != VK_SUCCESS) {
        printf("Failed to create Logical Device!\n");
        return;
    }

    vkGetDeviceQueue(device, indices.present_family, 0, &present_queue);
}

int main() {
    wdata = bs_initWnd(800, 600, "wnd");

    createInstance();
    createWindowSurface();
    selectPhysicalDevice();
    createLogicalDevice();

    bs_wndTick(tick);

    return 0;
}
