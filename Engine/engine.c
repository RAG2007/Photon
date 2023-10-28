#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <stdlib.h>
#include <stdio.h>
#include <vulkan/vulkan_core.h>
#include <GLFW/glfw3native.h>
#include <GLFW/glfw3.h>

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;



GLFWwindow* init_window() {
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", NULL, NULL);
	return window;
}

VkInstance create_instance() {
	//Creating Application Info
	VkApplicationInfo application_info;
	application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	application_info.pApplicationName = "Photon";
	application_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	application_info.pEngineName = "Itself";
	application_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	application_info.apiVersion = VK_API_VERSION_1_0;

	//adding required extensions
	uint32_t extension_count = 0;
	glfwGetRequiredInstanceExtensions(&extension_count);
	const char **extension_list = glfwGetRequiredInstanceExtensions(&extension_count);


	//Creating Instance Create info
	VkInstanceCreateInfo create_info;
	create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	create_info.pApplicationInfo = &application_info;
	create_info.enabledExtensionCount = extension_count;
	create_info.ppEnabledExtensionNames = extension_list;
	// TOFIX Add extension count and names
	
	//Creating instance
	VkInstance instance;
	if(vkCreateInstance(&create_info, NULL, &instance) != VK_SUCCESS)
		return NULL;
	return instance;
}

VkPhysicalDevice pick_physical_device(VkInstance instance) {
	//Checking count of physical devices
	uint32_t p_device_count = 0;
	if(vkEnumeratePhysicalDevices(instance, &p_device_count, NULL) != VK_SUCCESS)
		return NULL;

	//Loading list of physical devices
	VkPhysicalDevice p_device_list[p_device_count];
	if(vkEnumeratePhysicalDevices(instance, &p_device_count, p_device_list) != VK_SUCCESS)
		return NULL;

	//Picking right physical device
	int best_index = -1;
	for(int i = 0; i < p_device_count; i++) {
		VkPhysicalDeviceProperties p_device_properties;
		vkGetPhysicalDeviceProperties(p_device_list[i], &p_device_properties);
		// here TOFIX - add logic to picking p_device
		if(p_device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			best_index = i;
		if(p_device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU && best_index == -1)
			best_index = i;
	}
	if(best_index != -1)
		return p_device_list[best_index];
	return NULL;
}

VkDeviceQueueCreateInfo find_queue_families(VkPhysicalDevice p_device) {
	//getting queue families
	uint32_t queue_family_properties_count;
	vkGetPhysicalDeviceQueueFamilyProperties(p_device, &queue_family_properties_count, NULL);
	VkQueueFamilyProperties queue_family_properties[queue_family_properties_count];
	vkGetPhysicalDeviceQueueFamilyProperties(p_device, &queue_family_properties_count, queue_family_properties);
	int queue_family_index = -1;
	uint32_t queue_count = 0;

	for(int i = 0; i < queue_family_properties_count; i++) {
		if (queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			queue_family_index = i;
			queue_count = queue_family_properties[i].queueCount;
		}
	}
	float queue_priorities[queue_count];
	for(int i = 0; i < queue_count; i++) {
		queue_priorities[i] = (float)1 / queue_count;
	}
	VkDeviceQueueCreateInfo queue_create_info;
	queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queue_create_info.queueFamilyIndex = queue_family_index;
	queue_create_info.queueCount = queue_count;
	queue_create_info.pQueuePriorities = queue_priorities;
	return queue_create_info;
}

VkDevice create_logical_device(VkPhysicalDevice p_device, VkInstance instance, VkDeviceQueueCreateInfo queue_create_info, VkPhysicalDeviceFeatures p_device_features) {
	VkDeviceCreateInfo l_device_creation_info;
	l_device_creation_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	l_device_creation_info.queueCreateInfoCount = 1;
	l_device_creation_info.pQueueCreateInfos = &queue_create_info;
	l_device_creation_info.pEnabledFeatures = &p_device_features;
	VkDevice l_device;
	if(vkCreateDevice(p_device, &l_device_creation_info, NULL, &l_device) != VK_SUCCESS)
		return NULL;
	return l_device;
}


int main() {
	//Window Initialization
	GLFWwindow* window = init_window();

	//Instance Creation
	VkInstance instance = create_instance();
	if(instance == NULL) {
		printf("Unable to create instance");
		return -1;
	}

	//Picking Physical Device
	VkPhysicalDevice p_device = pick_physical_device(instance);
	VkPhysicalDeviceFeatures p_device_features;
	vkGetPhysicalDeviceFeatures(p_device, &p_device_features);
	VkPhysicalDeviceProperties p_device_properties;
	vkGetPhysicalDeviceProperties(p_device, &p_device_properties);
	printf("%s\n", p_device_properties.deviceName);
	if(p_device == NULL) {
		printf("Unable to pick device!");
		return -1;
	}
	VkDeviceQueueCreateInfo queue_create_info = find_queue_families(p_device);

	//device creations
	VkDevice l_device = create_logical_device(p_device, instance, queue_create_info, p_device_features);
	if(l_device == NULL) {
		printf("Failed to create logical device!");
		return -1;
	}

	//queues creation
	VkQueue queues[queue_create_info.queueCount];
	for(int i = 0; i < queue_create_info.queueCount; i++) {
		vkGetDeviceQueue(l_device, queue_create_info.queueFamilyIndex, i, &queues[i]);
	}
	
	//window surface cration
	VkSurfaceKHR surface;
	if(glfwCreateWindowSurface(instance, window, NULL, &surface) != VK_SUCCESS) {
		printf("Failed to create window surface!");
		return -1;
	}




	//main loop
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
	}

	//cleanup
	vkDestroySurfaceKHR(instance, surface, 0);
	vkDestroyDevice(l_device, NULL);
	//vkDestroyInstance(instance, NULL);
	glfwDestroyWindow(window);
	glfwTerminate();
}