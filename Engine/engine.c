#include <string.h>
#include <sys/types.h>
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

const uint32_t extension_count = 0;
const char *extension_list[] = {

};

GLFWwindow* init_window() {
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", 0, 0);
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

	//Creating Instance Create info
	VkInstanceCreateInfo create_info;
	create_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	create_info.pApplicationInfo = &application_info;
	create_info.enabledExtensionCount = extension_count;
	create_info.ppEnabledExtensionNames = extension_list;

	// TOFIX Add extension count and names


	//Creating instance
	VkInstance instance;
	if(vkCreateInstance(&create_info, 0, &instance) != VK_SUCCESS)
		return NULL;
	return instance;
}

VkPhysicalDevice pick_physical_device(VkInstance instance) {
	//Checking count of physical devices
	uint32_t p_device_count = 0;
	if(vkEnumeratePhysicalDevices(instance, &p_device_count, 0) != VK_SUCCESS)
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
		if(p_device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU && best_index == 0)
			best_index = i;
	}
	if(best_index != -1)
		return p_device_list[best_index];
	return NULL;
}

int main() {
	//Window Initialization
	init_window();

	//Instance Creation
	VkInstance instance = create_instance();
	if(instance == NULL) {
		printf("Unable to create instance");
		return -1;
	}
	
	//Picking Physical Device
	VkPhysicalDevice p_device = pick_physical_device(instance);
	if(p_device == NULL) {
		printf("Unable to pick device");
		return -1;
	}
}