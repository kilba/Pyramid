#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <wnd.h>

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan.h>

VkInstance instance;
VkSurfaceKHR surface;

VkPhysicalDeviceFeatures deviceFeatures = {0};
VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
VkDevice device;

VkQueue presentQueue;

typedef struct {
    uint32_t familyCount;

    uint32_t graphicsFamily;
    bool graphicsFamilyIsValid;

    uint32_t presentFamily;
    bool presentFamilyIsValid;
} QueueFamilyIndices;

WindowData wdata;

bool enableValidationLayers = true;
const char *validationLayers[] = {
    "VK_LAYER_KHRONOS_validation"
};

void tick() {
}

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
    QueueFamilyIndices indices;
    indices.graphicsFamilyIsValid = false;
    indices.presentFamilyIsValid = false;
    indices.familyCount = 2;

    uint32_t familyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &familyCount, NULL);

    VkQueueFamilyProperties queueFamilies[familyCount];
    vkGetPhysicalDeviceQueueFamilyProperties(device, &familyCount, queueFamilies);

    for(int i = 0; i < familyCount; i++) {
	if(queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
	    indices.graphicsFamily = i;
	    indices.graphicsFamilyIsValid = true;
	}

	VkBool32 presentSupport = false;
	vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
	if(presentSupport) {
	    indices.presentFamily = i;
	    indices.presentFamilyIsValid = true;
	}
    }

    return indices;
}

bool isDeviceSuitable(VkPhysicalDevice device) {
    QueueFamilyIndices indices = findQueueFamilies(device);
    return indices.graphicsFamilyIsValid && indices.presentFamilyIsValid;
}

bool checkValidationLayerSupport() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, NULL);

    VkLayerProperties availableLayers[layerCount];
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers);

    int validationLayerCount = sizeof(validationLayers) / sizeof(const char *);
    for(int i = 0; i < validationLayerCount; i++) {
	bool layerFound = false;

	for(int j = 0; j < layerCount; j++) {
	    if(strcmp(validationLayers[i], availableLayers[j].layerName) == 0) {
		layerFound = true;
		break;
	    }
	}

	if(!layerFound)
	    return false;
    }

    return true;
}

/* --- Initialization --- */
void createInstance() {
    if(enableValidationLayers && !checkValidationLayerSupport()) {
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

    if(enableValidationLayers) {
	ci.enabledLayerCount = sizeof(validationLayers) / sizeof(const char *);
	ci.ppEnabledLayerNames = validationLayers;
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
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, NULL);
    if(deviceCount == 0) {
	printf("No GPU with Vulkan support was found!\n");
    }

    VkPhysicalDevice devices[deviceCount];
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices);

    for(int i = 0; i < deviceCount; i++) {
	if(isDeviceSuitable(devices[i])) {
	    physicalDevice = devices[i];
	    break;
	}
    }

    if(physicalDevice == VK_NULL_HANDLE) {
	printf("Failed to find a suitable GPU!\n");
	return;
    }
}

void createLogicalDevice() {
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

    VkDeviceQueueCreateInfo queueCis[indices.familyCount];
    uint32_t uniqueQueueFamilies[] = { indices.graphicsFamily, indices.presentFamily };
    uint32_t uniqueFamilyCount = indices.familyCount;

    /* Remove Duplicates */
    for(int i = 0; i < uniqueFamilyCount; i++){
       for(int j = i + 1; j < uniqueFamilyCount; j++){
	  if(uniqueQueueFamilies[i] == uniqueQueueFamilies[j]){
	     for(int k = j; k < uniqueFamilyCount; k++) {
		uniqueQueueFamilies[k] = uniqueQueueFamilies[k+1];
	     }

	     j--;
	     uniqueFamilyCount--;
	  }
       }
    }

    float queuePriority = 1.0;
    for(int i = 0; i < uniqueFamilyCount; i++) {
	VkDeviceQueueCreateInfo queueCi = {0};
	queueCi.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCi.queueFamilyIndex = uniqueQueueFamilies[i];
	queueCi.queueCount = 1;
	queueCi.pQueuePriorities = &queuePriority;

	queueCis[i] = queueCi;
    }

    VkDeviceCreateInfo ci = {0};
    ci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    ci.pQueueCreateInfos = queueCis;
    ci.queueCreateInfoCount = uniqueFamilyCount;
    ci.pEnabledFeatures = &deviceFeatures;
    ci.enabledExtensionCount = 0;
    
    if(enableValidationLayers) {
	ci.enabledLayerCount = sizeof(validationLayers) / sizeof(const char *);
	ci.ppEnabledLayerNames = validationLayers;
    } else {
	ci.enabledLayerCount = 0;
    }

    VkResult err = vkCreateDevice(physicalDevice, &ci, NULL, &device);
    if(err != VK_SUCCESS) {
	printf("Failed to create Logical Device!\n");
	return;
    }

    vkGetDeviceQueue(device, indices.presentFamily, 0, &presentQueue);
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
