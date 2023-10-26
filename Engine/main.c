#include <vulkan/vulkan.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdlib.h>
#include <stdio.h>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

struct QueueFamilyIndices {
	uint32_t graphicsFamily;
};

int is_device_suitable(VkPhysicalDevice device) {
	return 1;
}

struct QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
	struct QueueFamilyIndices indices;
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, 0);
	return indices;
}

int main() {
	glfwInit(); //Init GLFW & window START
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", 0, 0); // //Init GLFW & window END

	VkApplicationInfo app_info; //Application Info START
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pApplicationName = "Engine";
	app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.pEngineName = "No Engine";
	app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.apiVersion = VK_API_VERSION_1_0; // Application Info END

	VkInstanceCreateInfo create_info; //Create Instance Info START
	create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	create_info.pApplicationInfo = &app_info; // Create Instance Info END

	uint32_t glfwExtensionCount = 0; //GLFW extensions START TOFIX: MACOS support
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	create_info.enabledExtensionCount = glfwExtensionCount;
	create_info.ppEnabledExtensionNames = glfwExtensions; 
	create_info.enabledLayerCount = 0;
	
	//GLFW extensions END
	// Vulkan Instance
	VkInstance instance;
	VkResult result = vkCreateInstance(&create_info, 0, &instance);
	if (vkCreateInstance(&create_info, 0, &instance) != VK_SUCCESS) {
		printf("failed to create instance!");
		return -1;
	} 
	// VULKAN INSTANCE END
	// Vulkan pick physical device
	VkPhysicalDevice physical_device = VK_NULL_HANDLE;
	uint32_t device_count = 1;
	vkEnumeratePhysicalDevices(instance, &device_count, &physical_device);
	if (physical_device == VK_NULL_HANDLE || !is_device_suitable(physical_device)) {
		printf("failed to find a suitable GPU!");
		return -1;
	}
	// pick physical device end TOFIX: add  picking best gpu

	// Finding queue families



	while (!glfwWindowShouldClose(window)) {
 		glfwPollEvents();
	}

	vkDestroyInstance(instance, 0); // CLEANUP
	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}