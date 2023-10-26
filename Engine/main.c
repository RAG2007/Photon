#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <stdlib.h>
#include <stdio.h>
#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN

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
	return physical_device; //TOFIX: add  picking best gpu
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

int main() {
	GLFWwindow* window = init_window();
	VkInstance instance = create_instance();
	VkPhysicalDevice physical_device = pick_physical_device(instance);
	struct QueueFamilyIndices indices = findQueueFamilies(physical_device);
	
	while (!glfwWindowShouldClose(window)) {
 		glfwPollEvents();
	}

	vkDestroyInstance(instance, 0); // CLEANUP
	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}