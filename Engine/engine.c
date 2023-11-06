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
	application_info.pNext = NULL;
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
	create_info.pNext = NULL;
	create_info.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
	create_info.pApplicationInfo = &application_info;
	create_info.enabledLayerCount = 0;
	create_info.enabledExtensionCount = extension_count;
	create_info.ppEnabledExtensionNames = extension_list;
	// TOFIX Add extension count and names
	
	//Creating instance
	VkInstance instance;
	if (vkCreateInstance(&create_info, NULL, &instance) != VK_SUCCESS)
		return NULL;
	return instance;
}

VkPhysicalDevice pick_physical_device(VkInstance instance) {
	//Checking count of physical devices
	uint32_t p_device_count = 0;
	if (vkEnumeratePhysicalDevices(instance, &p_device_count, NULL) != VK_SUCCESS)
		return NULL;

	//Loading list of physical devices
	VkPhysicalDevice p_device_list[p_device_count];
	if (vkEnumeratePhysicalDevices(instance, &p_device_count, p_device_list) != VK_SUCCESS)
		return NULL;

	//Picking right physical device
	int best_index = -1;
	for (int i = 0; i < p_device_count; i++) {
		VkPhysicalDeviceProperties p_device_properties;
		vkGetPhysicalDeviceProperties(p_device_list[i], &p_device_properties);
		// here TOFIX - add logic to picking p_device
		if (p_device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			best_index = i;
		if (p_device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU && best_index == -1)
			best_index = i;
	}
	if (best_index != -1)
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

	for (int i = 0; i < queue_family_properties_count; i++) {
		if (queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			queue_family_index = i;
			queue_count = queue_family_properties[i].queueCount;
		}
	}
	float queue_priorities[queue_count];
	for (int i = 0; i < queue_count; i++) {
		queue_priorities[i] = (float)1 / queue_count;
	}
	VkDeviceQueueCreateInfo queue_create_info;
	queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queue_create_info.pNext = NULL;
	queue_create_info.flags = VK_DEVICE_QUEUE_CREATE_PROTECTED_BIT;
	queue_create_info.queueFamilyIndex = queue_family_index;
	queue_create_info.queueCount = queue_count;
	queue_create_info.pQueuePriorities = queue_priorities;
	return queue_create_info;
}

VkDevice create_logical_device(VkPhysicalDevice p_device, VkInstance instance, VkDeviceQueueCreateInfo queue_create_info[], VkPhysicalDeviceFeatures p_device_features) {
	//creating required device extensions list
	const char *const device_extension_list[] =  {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	//creating logical device creation info
	VkDeviceCreateInfo l_device_creation_info;
	l_device_creation_info.enabledExtensionCount = 1;
	l_device_creation_info.ppEnabledExtensionNames = device_extension_list;
	l_device_creation_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	l_device_creation_info.pNext = NULL;
	l_device_creation_info.flags = 0;
	l_device_creation_info.enabledLayerCount = 0;
	l_device_creation_info.queueCreateInfoCount = 1;
	l_device_creation_info.pQueueCreateInfos = queue_create_info;
	l_device_creation_info.pEnabledFeatures = &p_device_features;
	VkDevice l_device;
	if (vkCreateDevice(p_device, &l_device_creation_info, NULL, &l_device) != VK_SUCCESS)
		return NULL;
	return l_device;
}

VkSurfaceFormatKHR setting_surface_format(VkPhysicalDevice p_device, VkSurfaceKHR surface) {
	uint32_t surface_format_count;
	vkGetPhysicalDeviceSurfaceFormatsKHR(p_device, surface, &surface_format_count, NULL);
	VkSurfaceFormatKHR surface_formats[surface_format_count];
	vkGetPhysicalDeviceSurfaceFormatsKHR(p_device, surface, &surface_format_count, surface_formats);
	for (int i = 0; i < surface_format_count; i++) {
		if (surface_formats[i].format == VK_FORMAT_B8G8R8A8_SRGB && surface_formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			return surface_formats[i];
	}
	return surface_formats[0];
}

VkPresentModeKHR setting_present_mode(VkPhysicalDevice p_device, VkSurfaceKHR surface) {
	uint32_t present_mode_count;
	vkGetPhysicalDeviceSurfacePresentModesKHR(p_device, surface, &present_mode_count, NULL);
	VkPresentModeKHR present_modes[present_mode_count];
	vkGetPhysicalDeviceSurfacePresentModesKHR(p_device, surface, &present_mode_count, present_modes);
	for (int i = 0; i < present_mode_count; i++) {
		if (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
			return VK_PRESENT_MODE_MAILBOX_KHR;
	}
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D setting_swapchain_extent() {
	//TOFIX add better logic to if extent is valid
	VkExtent2D extent;
	extent.height = HEIGHT;
	extent.width = WIDTH;
	return extent;
}
	
VkSwapchainKHR create_swapchain(VkPhysicalDevice p_device, VkSurfaceKHR surface, uint32_t queue_family_index, VkDevice l_device, VkSurfaceFormatKHR surface_format, VkPresentModeKHR present_mode, VkExtent2D extent) {
	VkSurfaceCapabilitiesKHR capabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(p_device, surface, &capabilities);
	//TOFIX: ADD runtime error if required capabilities are not met

	//creating SwapChain creation info
	uint32_t imageCount = capabilities.minImageCount + 1;
	if (imageCount > capabilities.maxImageCount && capabilities.maxImageCount != 0)
		imageCount = capabilities.maxImageCount;
	
	VkSwapchainCreateInfoKHR create_info;
	create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	create_info.pNext = NULL;
	create_info.flags = 0;
	create_info.surface = surface;
	create_info.pQueueFamilyIndices = NULL;
	create_info.pQueueFamilyIndices = NULL;
	create_info.minImageCount = imageCount;
	create_info.imageFormat = surface_format.format;
	create_info.imageColorSpace = surface_format.colorSpace;
	create_info.imageExtent = extent;
	create_info.imageArrayLayers = 1;
	create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // TOFIX add post processing
	create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	create_info.preTransform = capabilities.currentTransform;
	create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	create_info.presentMode = present_mode;
	create_info.clipped = VK_TRUE;
	create_info.oldSwapchain = VK_NULL_HANDLE;

	VkSwapchainKHR swap_chain;
	if (vkCreateSwapchainKHR(l_device, &create_info, NULL, &swap_chain) != VK_SUCCESS) {
		vkDestroySwapchainKHR(l_device, swap_chain, NULL);
		return NULL;
	}
	return swap_chain;
}

int create_image_views(VkDevice l_device, VkImage images[], uint32_t swapchain_image_count, VkSurfaceFormatKHR surface_format, VkImageView image_views[]) {
	for (size_t i = 0; i < swapchain_image_count; i++) {
		VkImageViewCreateInfo createInfo;
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.pNext = NULL;
		createInfo.flags = 0;
		createInfo.image = images[i];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = surface_format.format;
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;
		if (vkCreateImageView(l_device, &createInfo, NULL, &image_views[i]) != VK_SUCCESS) {
			return -1;
		}
	}
	return 0;
}

int main() {
	//Window Initialization
	GLFWwindow* window = init_window();

	//Instance Creation
	VkInstance instance = create_instance();
	if (instance == NULL) {
		printf("Unable to create instance Aborting");
		return -1;
	}
	//Picking Physical Device
	VkPhysicalDevice p_device = pick_physical_device(instance);
	VkPhysicalDeviceFeatures p_device_features;
	vkGetPhysicalDeviceFeatures(p_device, &p_device_features);
	VkPhysicalDeviceProperties p_device_properties;

	vkGetPhysicalDeviceProperties(p_device, &p_device_properties);
	if (p_device == NULL) {
		printf("Unable to pick device! Aborting");
		return -1;
	}

	VkDeviceQueueCreateInfo queue_create_info = find_queue_families(p_device);

	//device creations
	VkDevice l_device = create_logical_device(p_device, instance, &queue_create_info, p_device_features);
	if (l_device == NULL) {
		printf("Failed to create logical device! Aborting");
		return -1;
	}

	//queues creation
	VkQueue queues[queue_create_info.queueCount];
	for (int i = 0; i < queue_create_info.queueCount; i++) {
		vkGetDeviceQueue(l_device, queue_create_info.queueFamilyIndex, i, &queues[i]);
	}
	
	//window surface cration
	VkSurfaceKHR surface;
	if (glfwCreateWindowSurface(instance, window, NULL, &surface) != VK_SUCCESS) {
		printf("Failed to create window surface!");
		return -1;
	}

	//swapchain creation
	VkSurfaceFormatKHR surface_format = setting_surface_format(p_device, surface);
	VkPresentModeKHR present_mode = setting_present_mode(p_device, surface);
	VkExtent2D extent = setting_swapchain_extent();
	VkSwapchainKHR swapchain = create_swapchain(p_device, surface, queue_create_info.queueFamilyIndex, l_device, surface_format, present_mode, extent);
	if (swapchain == NULL) {
		printf("Failed to create swap chain! Aborting");
		return -1;
	}

	uint32_t swapchain_image_count;
	vkGetSwapchainImagesKHR(l_device, swapchain, &swapchain_image_count, NULL);
	VkImage swapchain_images[swapchain_image_count];
	vkGetSwapchainImagesKHR(l_device, swapchain, &swapchain_image_count, swapchain_images);

	//Creating image views
	VkImageView image_views[swapchain_image_count];
	if(create_image_views(l_device, swapchain_images, swapchain_image_count, surface_format, image_views) != 0) {
		printf("Failed to create Image Views");
		return -1;
	}


	//main loop
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
	}

	//cleanup
	for (int i = 0; i < swapchain_image_count; i++) {
		vkDestroyImageView(l_device, image_views[i], NULL);
	}
	vkDestroySwapchainKHR(l_device, swapchain, NULL);
	vkDestroySurfaceKHR(instance, surface, NULL);
	vkDestroyDevice(l_device, NULL);
	vkDestroyInstance(instance, NULL);
	glfwDestroyWindow(window);
	glfwTerminate();
}