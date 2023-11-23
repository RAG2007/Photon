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

int error_return = 1, success_return = 0;

void init_window(GLFWwindow **window)
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	*window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", NULL, NULL);
}

int create_instance(VkInstance *instance)
{
	VkApplicationInfo application_info = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pNext = NULL,
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
		.pNext = NULL,
		.flags = 0,
		.pApplicationInfo = &application_info,
		.enabledLayerCount = 0,
		.ppEnabledLayerNames = NULL,
		.enabledExtensionCount = extension_count,
		.ppEnabledExtensionNames = extension_list
	};
	
	if (vkCreateInstance(&create_info, NULL, instance) != VK_SUCCESS)
		return error_return;
	return success_return;
}

int pick_physical_device(VkInstance instance, VkPhysicalDevice *p_device)
{
	uint32_t p_device_count = 0;
	if (vkEnumeratePhysicalDevices(instance, &p_device_count,
				       NULL) != VK_SUCCESS)
		return error_return;

	VkPhysicalDevice p_device_list[p_device_count];
	if (vkEnumeratePhysicalDevices(instance, &p_device_count,
				       p_device_list) != VK_SUCCESS)
		return error_return;

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
	*p_device = p_device_list[best_index];
	if (best_index != -1)
		return success_return;
	return error_return;
}

int find_queue_families(VkPhysicalDevice p_device, VkDeviceQueueCreateInfo *create_info)
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

	*create_info = (VkDeviceQueueCreateInfo){
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.queueFamilyIndex = queue_family_index,
		.queueCount = queue_count,
		.pQueuePriorities = queue_priorities
	};
	return success_return;
}

int create_logical_device(VkPhysicalDevice p_device,
			       VkInstance instance, 
			       VkDeviceQueueCreateInfo queue_create_info[],
			       VkPhysicalDeviceFeatures p_device_features,
			       VkDevice *l_device)
{
	const char *const device_extension_list[] =  {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	VkDeviceCreateInfo create_info = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.queueCreateInfoCount = 1,
		.pQueueCreateInfos = queue_create_info,
		.enabledLayerCount = 0,
		.ppEnabledLayerNames = NULL,
		.enabledExtensionCount = 1,
		.ppEnabledExtensionNames = device_extension_list,
		.pEnabledFeatures = &p_device_features
	};
	if (vkCreateDevice(p_device, &create_info, NULL, l_device) != VK_SUCCESS)
		return error_return;
	return success_return;
}

int setting_surface_format(VkPhysicalDevice p_device,
			   VkSurfaceKHR surface,
			   VkSurfaceFormatKHR *surface_format)
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
		    surface_formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			*surface_format = surface_formats[i];
			return success_return;
		    }
	}
	*surface_format = surface_formats[0];
	return success_return;
}

int setting_present_mode(VkPhysicalDevice p_device,VkSurfaceKHR surface,
			 VkPresentModeKHR *present_mode)
{
	uint32_t present_mode_count;
	vkGetPhysicalDeviceSurfacePresentModesKHR(p_device, surface,
						  &present_mode_count, NULL);
	VkPresentModeKHR present_modes[present_mode_count];
	vkGetPhysicalDeviceSurfacePresentModesKHR(p_device, surface, 
						  &present_mode_count,
						  present_modes);
	for (int i = 0; i < present_mode_count; i++) {
		if (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
			*present_mode =  VK_PRESENT_MODE_MAILBOX_KHR;
			return success_return;
		}
	}
	*present_mode = VK_PRESENT_MODE_FIFO_KHR;
	return success_return;
}

int setting_swapchain_extent(VkExtent2D *extent)
{
	extent->height = HEIGHT;
	extent->width = WIDTH;
	return success_return;
}
	
int create_swapchain(VkPhysicalDevice p_device, VkSurfaceKHR surface,
				uint32_t queue_family_index, VkDevice l_device,
				VkSurfaceFormatKHR surface_format, 
				VkPresentModeKHR present_mode,
				VkExtent2D extent, VkSwapchainKHR *swap_chain)
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
		.pNext = NULL,
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
		.pQueueFamilyIndices = NULL,
		.preTransform = capabilities.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = present_mode,
		.clipped = VK_TRUE,
		.oldSwapchain = VK_NULL_HANDLE
	};

	if (vkCreateSwapchainKHR(l_device, &create_info, NULL,
				 swap_chain) != VK_SUCCESS) {
		vkDestroySwapchainKHR(l_device, *swap_chain, NULL);
		return error_return;
	}
	return success_return;
}

int create_image_views(VkDevice l_device, VkImage images[], 
		       uint32_t swapchain_image_count,
		       VkSurfaceFormatKHR surface_format,
		       VkImageView image_views[])
{
	for (size_t i = 0; i < swapchain_image_count; i++) {
		VkImageViewCreateInfo create_info = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.pNext = NULL,
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
			return error_return;
		}
	}
	return success_return;
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

int create_shader_module(VkDevice l_device, const char *code, int code_length,
			 VkShaderModule *shader_module)
{
	VkShaderModuleCreateInfo create_info = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.codeSize = code_length,
		.pCode = (uint *)code
	};

	if (vkCreateShaderModule(l_device, &create_info, NULL, 
				 shader_module) != VK_SUCCESS) {
		printf("Failed to create Shader Module");
		return error_return;
	}
	return success_return;
}

int create_render_pass(VkSurfaceFormatKHR surface_format, VkDevice l_device,
		       VkRenderPass *render_pass)
{
	VkAttachmentDescription color_attachment = {
		.flags = 0,
		.format = surface_format.format,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	};

	VkAttachmentReference color_attachment_ref = {
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	VkSubpassDescription subpass = {
		.flags = 0,
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.inputAttachmentCount = 0,
		.pInputAttachments = 0,
		.colorAttachmentCount = 1,
		.pColorAttachments = &color_attachment_ref,
		.pResolveAttachments = NULL,
		.pDepthStencilAttachment = NULL,
		.preserveAttachmentCount = 0,
		.pPreserveAttachments = NULL
	};

	VkRenderPassCreateInfo render_pass_info = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.attachmentCount = 1,
		.pAttachments = &color_attachment,
		.subpassCount = 1,
		.pSubpasses = &subpass,
		.dependencyCount = 0,
		.pDependencies = NULL
	};

	if (vkCreateRenderPass(l_device, &render_pass_info, NULL, render_pass) !=
			       VK_SUCCESS) {
		return error_return;
	}
	return success_return;
}

int create_graphics_pipeline(VkDevice l_device, VkExtent2D extent,
			     VkRenderPass render_pass,
			     VkPipelineLayout *pipeline_layout,
			     VkPipeline *graphics_pipeline)
{
	int length_vert;
	int length_frag;
	char *vert_shader_code = read_file("shaders/vert.spv", &length_vert);

	char *frag_shader_code = read_file("shaders/frag.spv", &length_frag);

	VkShaderModule vert_shader_module;
	create_shader_module(l_device, vert_shader_code, length_vert,
			     &vert_shader_module);

	VkShaderModule frag_shader_module;
	create_shader_module(l_device, frag_shader_code, length_frag,
			     &frag_shader_module);

	VkPipelineShaderStageCreateInfo vert_shader_stage_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.stage = VK_SHADER_STAGE_VERTEX_BIT,
		.module = vert_shader_module,
		.pName = "main",
		.pSpecializationInfo = NULL
	};


	VkPipelineShaderStageCreateInfo frag_shader_stage_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
		.module = frag_shader_module,
		.pName = "main",
		.pSpecializationInfo = NULL
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
		.pNext = NULL,
		.flags = 0,
		.dynamicStateCount = 2,
		.pDynamicStates = dynamic_states
	};

	VkPipelineVertexInputStateCreateInfo vertex_input_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.vertexBindingDescriptionCount = 0,
		.pVertexBindingDescriptions = NULL,
		.vertexAttributeDescriptionCount = 0,
		.pVertexAttributeDescriptions = NULL
	};

	VkPipelineInputAssemblyStateCreateInfo input_assembly = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.pNext = NULL,
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
		.pNext = NULL,
		.flags = 0,
		.viewportCount = 1,
		.pViewports = &viewport,
		.scissorCount = 1,
		.pScissors = &scissor
	};

	VkPipelineRasterizationStateCreateInfo rasterizer = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.pNext = NULL,
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
		.pNext = NULL,
		.flags = 0,
		.sampleShadingEnable = VK_FALSE,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
		.minSampleShading = 1.0f,
		.pSampleMask = NULL,
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
		.pNext = NULL,
		.flags = 0,
		.logicOpEnable = VK_FALSE,
		.logicOp = VK_LOGIC_OP_COPY,
		.attachmentCount = 1,
		.pAttachments = &color_blend_attachment,
		.blendConstants =  {0.0f, 0.0f, 0.0f, 0.0f}
	};


	VkPipelineLayoutCreateInfo pipeline_layout_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.setLayoutCount = 0,
		.pSetLayouts = NULL,
		.pushConstantRangeCount = 0,
		.pPushConstantRanges = NULL
	};

	if (vkCreatePipelineLayout(l_device, &pipeline_layout_info, NULL,
				   pipeline_layout) != VK_SUCCESS) {
		printf("Failed to create pipeline layout!");
		return error_return;
	};

	VkGraphicsPipelineCreateInfo pipeline_info = {
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.stageCount = 2,
		.pStages = shader_stages,
		.pVertexInputState = &vertex_input_info,
		.pInputAssemblyState = &input_assembly,
		.pTessellationState = NULL,
		.pViewportState = &viewport_state,
		.pRasterizationState = &rasterizer,
		.pMultisampleState = &multisampling,
		.pDepthStencilState = NULL,
		.pColorBlendState = &color_blending,
		.pDynamicState = &dynamic_state,
		.layout = *pipeline_layout,
		.renderPass = render_pass,
		.subpass = 0,
		.basePipelineHandle = VK_NULL_HANDLE,
		.basePipelineIndex = -1
	};

	if (vkCreateGraphicsPipelines(l_device, VK_NULL_HANDLE, 1,
				      &pipeline_info, NULL, graphics_pipeline)
	    != VK_SUCCESS) {
		printf("failed to create graphics pipeline!");
		return error_return;
	}

	free(frag_shader_code);
	free(vert_shader_code);
	vkDestroyShaderModule(l_device, vert_shader_module, NULL);
	vkDestroyShaderModule(l_device, frag_shader_module, NULL);

	return success_return;
}

int main()
{
	GLFWwindow* window;
	init_window(&window);

	VkInstance instance;
	
	if (create_instance(&instance) != success_return) {
		printf("Unable to create instance Aborting");
		return error_return;
	}

	VkPhysicalDevice p_device;
	if (pick_physical_device(instance, &p_device) != success_return) {
		printf("Unable to pick device! Aborting");
		return error_return;
	}
	VkPhysicalDeviceFeatures p_device_features;
	vkGetPhysicalDeviceFeatures(p_device, &p_device_features);
	VkPhysicalDeviceProperties p_device_properties;
	vkGetPhysicalDeviceProperties(p_device, &p_device_properties);

	

	VkDeviceQueueCreateInfo queue_create_info;
	if(find_queue_families(p_device, &queue_create_info) != success_return) {
		printf("Failed to find appropriate queue families");
		return error_return;
	}

	VkDevice l_device;
	if(create_logical_device(p_device, instance, &queue_create_info,
			      p_device_features, &l_device) != success_return) {
		printf("Failed to create logical device! Aborting");
		return error_return;
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
		return error_return;
	}

	VkSurfaceFormatKHR surface_format;
	setting_surface_format(p_device, surface, &surface_format);

	VkPresentModeKHR present_mode;
	setting_present_mode(p_device, surface, &present_mode);

	VkExtent2D extent;
	setting_swapchain_extent(&extent);

	VkSwapchainKHR swap_chain;
	if (create_swapchain(p_device, surface, queue_create_info.queueFamilyIndex,
			     l_device, surface_format, present_mode, extent,
			     &swap_chain) != success_return) {
		printf("Failed to create swap chain! Aborting");
		return error_return;
	}

	uint32_t swapchain_image_count;
	vkGetSwapchainImagesKHR(l_device, swap_chain, &swapchain_image_count,
				NULL);
	VkImage swapchain_images[swapchain_image_count];
	vkGetSwapchainImagesKHR(l_device, swap_chain, &swapchain_image_count,
				swapchain_images);

	VkImageView image_views[swapchain_image_count];
	if (create_image_views(l_device, swapchain_images,
			       swapchain_image_count, surface_format,
			       image_views) != success_return) {
		printf("Failed to create Image Views");
		return error_return;
	}

	VkRenderPass render_pass;
	if(create_render_pass(surface_format, l_device, &render_pass) !=
	   success_return) {
		printf("failed to create render pass!");
		return error_return;
	}

	VkPipelineLayout pipeline_layout;
	VkPipeline graphics_pipeline;
	if(create_graphics_pipeline(l_device, extent, render_pass,
				    &pipeline_layout, &graphics_pipeline)
	   != success_return) {
		return error_return;
	}

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
	}

	for (int i = 0; i < swapchain_image_count; i++) {
		vkDestroyImageView(l_device, image_views[i], NULL);
	}

	vkDestroyPipeline(l_device, graphics_pipeline, NULL);
	vkDestroyPipelineLayout(l_device, pipeline_layout, NULL);
	vkDestroyRenderPass(l_device, render_pass, NULL);
	vkDestroySwapchainKHR(l_device, swap_chain, NULL);
	vkDestroySurfaceKHR(instance, surface, NULL);
	vkDestroyDevice(l_device, NULL);
	vkDestroyInstance(instance, NULL);
	glfwDestroyWindow(window);
	glfwTerminate();
}
