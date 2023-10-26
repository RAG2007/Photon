#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <stdlib.h>
#include <stdio.h>
#include <vulkan/vulkan_core.h>
#include <GLFW/glfw3native.h>
#include <GLFW/glfw3.h>
#define GLFW_INCLUDE_VULKAN
#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#define GLFW_EXPOSE_NATIVE_WIN32

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

int is_device_suitable(VkPhysicalDevice device) {
    	return 1;
}
struct QueueFamilyIndices {
    uint32_t graphicsFamily;
};

GLFWwindow* init_window() {
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", 0, 0);
	return window;
}

VkInstance create_instance() {
	VkApplicationInfo app_info;
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pApplicationName = "Engine";
	app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.pEngineName = "Photon";
	app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.apiVersion = VK_API_VERSION_1_0;
	VkInstanceCreateInfo create_info;
	create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	create_info.pApplicationInfo = &app_info;
	uint32_t glfwExtensionCount = 0; //GLFW extensions START TOFIX: MACOS support
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	create_info.enabledExtensionCount = glfwExtensionCount;
	create_info.ppEnabledExtensionNames = glfwExtensions; 
	create_info.enabledLayerCount = 0;
	VkInstance instance;
	VkResult result = vkCreateInstance(&create_info, 0, &instance);
	if (vkCreateInstance(&create_info, 0, &instance) != VK_SUCCESS) {
		printf("failed to create instance!");
	}
	return instance;
}

VkPhysicalDevice pick_physical_device(VkInstance instance) {
	VkPhysicalDevice physical_device = VK_NULL_HANDLE;
	uint32_t device_count = 1;
	vkEnumeratePhysicalDevices(instance, &device_count, &physical_device);
	if (physical_device == VK_NULL_HANDLE || !is_device_suitable(physical_device)) {
		printf("failed to find a suitable GPU!");
	}
	return physical_device;
}

struct QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
	struct QueueFamilyIndices indices;
	uint32_t queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, 0);
	struct VkQueueFamilyProperties queue_families[queue_family_count];
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families);
	for(int i = 0; i < queue_family_count; i++) {
		if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphicsFamily = i;
		}
	}
	return indices;
}

int create_logical_device(struct QueueFamilyIndices indices, VkPhysicalDevice physical_device, VkDevice* device) {
	VkDeviceQueueCreateInfo queue_create_info;
	queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queue_create_info.queueFamilyIndex = indices.graphicsFamily;
	queue_create_info.queueCount = 1;
	float queuePriority = 1.0f;
	queue_create_info.pQueuePriorities = &queuePriority;
	VkPhysicalDeviceFeatures deviceFeatures;
	VkDeviceCreateInfo create_info;
	create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	create_info.pQueueCreateInfos = &queue_create_info;
	create_info.queueCreateInfoCount = 1;

	create_info.pEnabledFeatures = &deviceFeatures;
	if(vkCreateDevice(physical_device, &create_info, 0, device) == VK_SUCCESS)
		return 1;
	else
		return -1;
}

int main() {
	GLFWwindow* window = init_window();
	VkInstance instance = create_instance();
	VkPhysicalDevice physical_device = pick_physical_device(instance);
	struct QueueFamilyIndices indices = findQueueFamilies(physical_device);
	VkDevice device;
	create_logical_device(indices, physical_device, &device);
	VkQueue graphicsQueue;
	vkGetDeviceQueue(device, indices.graphicsFamily, 0, &graphicsQueue);
	VkSurfaceKHR surface;
	glfwCreateWindowSurface(instance, window, 0, &surface);
	VkQueue presentQueue;
	
	while (!glfwWindowShouldClose(window)) {
 		glfwPollEvents();
	}

	vkDestroySurfaceKHR(instance, surface, 0);
	vkDestroyDevice(device, 0);
	vkDestroyInstance(instance, 0);
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}