#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdlib.h>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>
#include <GLFW/glfw3native.h>
#include <GLFW/glfw3.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 800;

GLFWwindow* init_window()
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", NULL,
					      NULL);
	return window;
}

VkInstance create_instance()
{
	VkApplicationInfo application_info = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pNext = 0,
		.pApplicationName = "Photon",
		.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
		.pEngineName = "Itself",
		.engineVersion = VK_MAKE_VERSION(1, 0, 0),
		.apiVersion = VK_API_VERSION_1_0
	};
	uint32_t extension_count = 0;
	glfwGetRequiredInstanceExtensions(&extension_count);
	const char **extension_list = glfwGetRequiredInstanceExtensions(&extension_count);

	VkInstanceCreateInfo create_info = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pNext = 0,
		.flags = 0,
		.pApplicationInfo = &application_info,
		.enabledLayerCount = 0,
		.ppEnabledLayerNames = 0,
		.enabledExtensionCount = extension_count,
		.ppEnabledExtensionNames = extension_list
	};
	
	VkInstance instance;
	if (vkCreateInstance(&create_info, NULL, &instance) != VK_SUCCESS)
		return NULL;
	return instance;
}

VkPhysicalDevice pick_physical_device(VkInstance instance)
{
	uint32_t p_device_count = 0;
	if (vkEnumeratePhysicalDevices(instance, &p_device_count,
				       NULL) != VK_SUCCESS)
		return NULL;

	VkPhysicalDevice p_device_list[p_device_count];
	if (vkEnumeratePhysicalDevices(instance, &p_device_count,
				       p_device_list) != VK_SUCCESS)
		return NULL;

	int best_index = -1;
	for (int i = 0; i < p_device_count; i++) {
		VkPhysicalDeviceProperties p_device_properties;
		vkGetPhysicalDeviceProperties(p_device_list[i], &p_device_properties);
		if (p_device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			best_index = i;
		if (p_device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU &
		    best_index == -1)
			best_index = i;
	}
	if (best_index != -1)
		return p_device_list[best_index];
	return NULL;
}

VkDeviceQueueCreateInfo find_queue_families(VkPhysicalDevice p_device)
{
	uint32_t queue_family_properties_count;
	vkGetPhysicalDeviceQueueFamilyProperties(p_device,
						 &queue_family_properties_count,
						 NULL);
	VkQueueFamilyProperties queue_family_properties[queue_family_properties_count];
	vkGetPhysicalDeviceQueueFamilyProperties(p_device, 
						 &queue_family_properties_count,
						 queue_family_properties);
	int queue_family_index = -1;
	uint32_t queue_count = 0;

	for (int i = 0; i < queue_family_properties_count; i++) {
		if (queue_family_properties[i].queueFlags &
		    VK_QUEUE_GRAPHICS_BIT) {
			queue_family_index = i;
			queue_count = queue_family_properties[i].queueCount;
		}
	}
	float queue_priorities[queue_count];
	for (int i = 0; i < queue_count; i++) {
		queue_priorities[i] = (float)1 / queue_count;
	}

	VkDeviceQueueCreateInfo create_info = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.pNext = 0,
		.flags = 0,
		.queueFamilyIndex = queue_family_index,
		.queueCount = queue_count,
		.pQueuePriorities = queue_priorities
	};
	return create_info;
}

VkDevice create_logical_device(VkPhysicalDevice p_device,
			       VkInstance instance, 
			       VkDeviceQueueCreateInfo queue_create_info[],
			       VkPhysicalDeviceFeatures p_device_features)
{
	const char *const device_extension_list[] =  {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	VkDeviceCreateInfo create_info = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = 0,
		.flags = 0,
		.queueCreateInfoCount = 1,
		.pQueueCreateInfos = queue_create_info,
		.enabledLayerCount = 0,
		.ppEnabledLayerNames = 0,
		.enabledExtensionCount = 1,
		.ppEnabledExtensionNames = device_extension_list,
		.pEnabledFeatures = &p_device_features
	};
	VkDevice l_device;
	if (vkCreateDevice(p_device, &create_info, NULL,
			   &l_device) != VK_SUCCESS)
		return NULL;
	return l_device;
}

VkSurfaceFormatKHR setting_surface_format(VkPhysicalDevice p_device,
					  VkSurfaceKHR surface)
{
	uint32_t surface_format_count;
	vkGetPhysicalDeviceSurfaceFormatsKHR(p_device, surface,
					     &surface_format_count, NULL);
	VkSurfaceFormatKHR surface_formats[surface_format_count];
	vkGetPhysicalDeviceSurfaceFormatsKHR(p_device, surface, 
					     &surface_format_count,
					     surface_formats);
	for (int i = 0; i < surface_format_count; i++) {
		if (surface_formats[i].format == VK_FORMAT_B8G8R8A8_SRGB &&
		    surface_formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			return surface_formats[i];
	}
	return surface_formats[0];
}

VkPresentModeKHR setting_present_mode(VkPhysicalDevice p_device,
				      VkSurfaceKHR surface)
{
	uint32_t present_mode_count;
	vkGetPhysicalDeviceSurfacePresentModesKHR(p_device, surface,
						  &present_mode_count, NULL);
	VkPresentModeKHR present_modes[present_mode_count];
	vkGetPhysicalDeviceSurfacePresentModesKHR(p_device, surface, 
						  &present_mode_count,
						  present_modes);
	for (int i = 0; i < present_mode_count; i++) {
		if (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
			return VK_PRESENT_MODE_MAILBOX_KHR;
	}
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D setting_swapchain_extent()
{
	VkExtent2D extent;
	extent.height = HEIGHT;
	extent.width = WIDTH;
	return extent;
}
	
VkSwapchainKHR create_swapchain(VkPhysicalDevice p_device, VkSurfaceKHR surface,
				uint32_t queue_family_index, VkDevice l_device,
				VkSurfaceFormatKHR surface_format, 
				VkPresentModeKHR present_mode,
				VkExtent2D extent)
{
	VkSurfaceCapabilitiesKHR capabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(p_device, surface,
						  &capabilities);

	uint32_t imageCount = capabilities.minImageCount + 1;
	if (imageCount > capabilities.maxImageCount &&
	    capabilities.maxImageCount != 0)
		imageCount = capabilities.maxImageCount;
	
	VkSwapchainCreateInfoKHR create_info = {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.pNext = 0,
		.flags = 0,
		.surface = surface,
		.minImageCount = imageCount,
		.imageFormat = surface_format.format,
		.imageColorSpace = surface_format.colorSpace,
		.imageExtent = extent,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = 0,
		.preTransform = capabilities.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = present_mode,
		.clipped = VK_TRUE,
		.oldSwapchain = VK_NULL_HANDLE
	};

	VkSwapchainKHR swap_chain;
	if (vkCreateSwapchainKHR(l_device, &create_info, NULL,
				 &swap_chain) != VK_SUCCESS) {
		vkDestroySwapchainKHR(l_device, swap_chain, NULL);
		return NULL;
	}
	return swap_chain;
}

int create_image_views(VkDevice l_device, VkImage images[], 
		       uint32_t swapchain_image_count,
		       VkSurfaceFormatKHR surface_format,
		       VkImageView image_views[])
{
	for (size_t i = 0; i < swapchain_image_count; i++) {
		VkImageViewCreateInfo create_info = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.pNext = 0,
			.flags = 0,
			.image = images[i],
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = surface_format.format,
			.components = {
				.r = VK_COMPONENT_SWIZZLE_IDENTITY,
				.g = VK_COMPONENT_SWIZZLE_IDENTITY,
				.b = VK_COMPONENT_SWIZZLE_IDENTITY,
				.a = VK_COMPONENT_SWIZZLE_IDENTITY
			},
			.subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1
			}
		};
		if (vkCreateImageView(l_device, &create_info, NULL,
		                      &image_views[i]) != VK_SUCCESS) {
			return -1;
		}
	}
	return 0;
}


char *read_file(char *name, int *length)
{
	int fd = open(name, O_RDONLY);

	if (fd == -1) {
		fprintf(stderr, "Failed to open file - %s\n", strerror(errno));
		return NULL;
	}

	struct stat st;

	if (fstat(fd, &st) == -1) {
		fprintf(stderr, "Failed get file size - %s\n", strerror(errno));
		return NULL;
	}

	char *buffer = malloc(st.st_size);

	if (!buffer) {
		fprintf(stderr, "Failed to allocate memory-%s\n", strerror(errno));
		return NULL;
	}

	if (read(fd, buffer, st.st_size) != st.st_size) {
		fprintf(stderr, "Failed to read file - %s\n", strerror(errno));
		return NULL;
	}
	*length = st.st_size;
	return buffer;
}

VkShaderModule create_shader_module(VkDevice l_device, const char *code, 
				    int code_length)
{
	VkShaderModuleCreateInfo create_info = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.pNext = 0,
		.flags = 0,
		.codeSize = code_length,
		.pCode = (uint *)code
	};

	VkShaderModule shader_module;
	if (vkCreateShaderModule(l_device, &create_info, NULL, 
				 &shader_module) != VK_SUCCESS) {
		printf("Failed to create Shader Module");
		return NULL;
	}
	return shader_module;
}

VkPipelineLayout create_graphics_pipeline(VkDevice l_device, VkExtent2D extent)
{
	int length_vert;
	int length_frag;
	char *vert_shader_code = read_file("shaders/vert.spv", &length_vert);
	char *frag_shader_code = read_file("shaders/frag.spv", &length_frag);

	VkShaderModule vert_shader_module = create_shader_module(l_device,
							       vert_shader_code,
							       length_vert);
	VkShaderModule frag_shader_module = create_shader_module(l_device,
	 						       frag_shader_code, 
							       length_frag);

	VkPipelineShaderStageCreateInfo vert_shader_stage_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.pNext = 0,
		.flags = 0,
		.stage = VK_SHADER_STAGE_VERTEX_BIT,
		.module = vert_shader_module,
		.pName = "main",
		.pSpecializationInfo = 0
	};


	VkPipelineShaderStageCreateInfo frag_shader_stage_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.pNext = 0,
		.flags = 0,
		.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
		.module = frag_shader_module,
		.pName = "main",
		.pSpecializationInfo = 0
	};

	VkPipelineShaderStageCreateInfo shader_stages[] = {
		vert_shader_stage_info,
		frag_shader_stage_info
	};

	VkDynamicState dynamic_states[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamic_state = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.pNext = 0,
		.flags = 0,
		.dynamicStateCount = 2,
		.pDynamicStates = dynamic_states
	};

	VkPipelineVertexInputStateCreateInfo vertex_input_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.pNext = 0,
		.flags = 0,
		.vertexBindingDescriptionCount = 0,
		.pVertexBindingDescriptions = 0,
		.vertexAttributeDescriptionCount = 0,
		.pVertexAttributeDescriptions = 0
	};

	VkPipelineInputAssemblyStateCreateInfo input_assembly = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.pNext = 0,
		.flags = 0,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		.primitiveRestartEnable = VK_TRUE
	};

	VkViewport viewport = {
		.x = 0.0f,
		.y = 0.0f,
		.width = (float) extent.width,
		.height = (float) extent.height,
		.minDepth = 0.0f,
		.maxDepth = 1.0f
	};

	VkRect2D scissor = {
		.offset = {0, 0},
		.extent = extent
	};

	VkPipelineViewportStateCreateInfo viewport_state = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.pNext = 0,
		.flags = 0,
		.viewportCount = 1,
		.pViewports = &viewport,
		.scissorCount = 1,
		.pScissors = &scissor
	};

	VkPipelineRasterizationStateCreateInfo rasterizer = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.pNext = 0,
		.flags = 0,
		.depthClampEnable = VK_FALSE,
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.cullMode = VK_CULL_MODE_BACK_BIT,
		.frontFace = VK_FRONT_FACE_CLOCKWISE,
		.lineWidth = 1.0f,
		.depthBiasEnable = VK_FALSE,
		.depthBiasConstantFactor = 0.0f,
		.depthBiasClamp = 0.0f,
		.depthBiasSlopeFactor = 0.0f
	};
	
	VkPipelineMultisampleStateCreateInfo multisampling = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.pNext = 0,
		.flags = 0,
		.sampleShadingEnable = VK_FALSE,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
		.minSampleShading = 1.0f,
		.pSampleMask = 0,
		.alphaToCoverageEnable = VK_FALSE,
		.alphaToOneEnable = VK_FALSE,
	};

	VkPipelineColorBlendAttachmentState color_blend_attachment = {
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | 
				  VK_COLOR_COMPONENT_G_BIT |
				  VK_COLOR_COMPONENT_B_BIT |
				  VK_COLOR_COMPONENT_A_BIT,
		.blendEnable = VK_FALSE,
		.srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
		.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
		.colorBlendOp = VK_BLEND_OP_ADD,
		.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
		.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
		.alphaBlendOp = VK_BLEND_OP_ADD
	};

	VkPipelineColorBlendStateCreateInfo color_blending = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.pNext = 0,
		.flags = 0,
		.logicOpEnable = VK_FALSE,
		.logicOp = VK_LOGIC_OP_COPY,
		.attachmentCount = 1,
		.pAttachments = &color_blend_attachment,
		.blendConstants[0] = 0.0f,
		.blendConstants[1] = 0.0f,
		.blendConstants[2] = 0.0f,
		.blendConstants[3] = 0.0f
	};

	VkPipelineLayout pipeline_layout;

	VkPipelineLayoutCreateInfo pipeline_layout_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pNext = 0,
		.flags = 0,
		.setLayoutCount = 0,
		.pSetLayouts = 0,
		.pushConstantRangeCount = 0,
		.pPushConstantRanges = 0,
	};
	//REST OF THE CODE
	if (vkCreatePipelineLayout(l_device, &pipeline_layout_info, 0, &pipeline_layout) != VK_SUCCESS) {
		return NULL;
	};
	
	vkDestroyShaderModule(l_device, vert_shader_module, NULL);
	vkDestroyShaderModule(l_device, frag_shader_module, NULL);
	return pipeline_layout;
}

int main()
{
	GLFWwindow* window = init_window();

	VkInstance instance = create_instance();
	if (instance == NULL) {
		printf("Unable to create instance Aborting");
		return -1;
	}

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

	VkDevice l_device = create_logical_device(p_device, instance,
						  &queue_create_info,
						  p_device_features);
	if (l_device == NULL) {
		printf("Failed to create logical device! Aborting");
		return -1;
	}

	VkQueue queues[queue_create_info.queueCount];
	for (int i = 0; i < queue_create_info.queueCount; i++) {
		vkGetDeviceQueue(l_device, queue_create_info.queueFamilyIndex,
				 i, &queues[i]);
	}
	
	VkSurfaceKHR surface;
	if (glfwCreateWindowSurface(instance, window, NULL,
				    &surface) != VK_SUCCESS) {
		printf("Failed to create window surface!");
		return -1;
	}

	VkSurfaceFormatKHR surface_format = setting_surface_format(p_device,
								   surface);
	VkPresentModeKHR present_mode = setting_present_mode(p_device, surface);
	VkExtent2D extent = setting_swapchain_extent();
	VkSwapchainKHR swapchain = create_swapchain(p_device, surface,
						    queue_create_info.queueFamilyIndex,
						    l_device, surface_format, 
						    present_mode, extent);
	if (swapchain == NULL) {
		printf("Failed to create swap chain! Aborting");
		return -1;
	}

	uint32_t swapchain_image_count;
	vkGetSwapchainImagesKHR(l_device, swapchain, &swapchain_image_count,
				NULL);
	VkImage swapchain_images[swapchain_image_count];
	vkGetSwapchainImagesKHR(l_device, swapchain, &swapchain_image_count,
				swapchain_images);

	VkImageView image_views[swapchain_image_count];
	if (create_image_views(l_device, swapchain_images,
			       swapchain_image_count, surface_format,
			       image_views) != 0) {
		printf("Failed to create Image Views");
		return -1;
	}

	VkPipelineLayout pipeline_layout;
	pipeline_layout = create_graphics_pipeline(l_device, extent);
	if(pipeline_layout == NULL) {
		printf("failed to create pipeline layout!");
		return -1;
	}
	

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
	}

	for (int i = 0; i < swapchain_image_count; i++) {
		vkDestroyImageView(l_device, image_views[i], NULL);
	}

	vkDestroyPipelineLayout(l_device, pipeline_layout, NULL);
	vkDestroySwapchainKHR(l_device, swapchain, NULL);
	vkDestroySurfaceKHR(instance, surface, NULL);
	vkDestroyDevice(l_device, NULL);
	vkDestroyInstance(instance, NULL);
	glfwDestroyWindow(window);
	glfwTerminate();
}
