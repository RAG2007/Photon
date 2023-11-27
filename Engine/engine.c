#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
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

unsigned int WIDTH = 800;
unsigned int HEIGHT = 600;

const int MAX_FRAMES_IN_FLIGHT = 2;

int error_return = 1, success_return = 0;

int window_pointer;

struct queue_family_indices {
	int graphics_family;
   	int present_family;
};

struct engine_data {
	GLFWwindow *window;
	VkInstance instance;
	VkPhysicalDevice physical_device;
	VkPhysicalDeviceProperties physical_device_properties;
	VkPhysicalDeviceFeatures physical_device_features;
	VkSurfaceKHR surface;
	VkQueue graphics_queue;
	VkQueue present_queue;
	uint32_t queue_families_count;
	VkDeviceQueueCreateInfo queue_create_infos[2];
	VkDevice device;
	VkSurfaceFormatKHR surface_format;
	VkPresentModeKHR present_mode;
	VkExtent2D extent;
	VkSwapchainKHR swapchain;
	uint32_t swapchain_image_count;
	VkImage *swapchain_images;
	VkImageView *image_views;
	VkShaderModule shader_module;
	VkRenderPass render_pass;
	VkPipelineLayout pipeline_layout;
	VkPipeline graphics_pipeline;
	VkFramebuffer *swapchain_framebuffers;
	VkCommandPool command_pool;
	VkCommandBuffer command_buffers[2];
	uint32_t image_index;
	VkSemaphore image_available_semaphores[2];
	VkSemaphore render_finished_semaphores[2];
	VkFence in_flight_fences[2];
	uint32_t current_frame;
};


int framebuffer_resized = 0;

void *user_window_pointer;

void resize_callback(GLFWwindow* window, int width, int height) {
	framebuffer_resized = 1;
}

void engine_init_window(struct engine_data *data)
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	data->window = glfwCreateWindow(WIDTH, HEIGHT, "Photon", NULL, NULL);
	glfwSetWindowUserPointer(data->window, user_window_pointer);
	glfwSetFramebufferSizeCallback(data->window, resize_callback);
}


int engine_create_vulkan_instance(struct engine_data *data)
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

	if (vkCreateInstance(&create_info, NULL, &data->instance) != VK_SUCCESS)
		return error_return;
	return success_return;
}

int engine_pick_physical_device(struct engine_data *data)
{
	uint32_t p_device_count = 0;
	if (vkEnumeratePhysicalDevices(data->instance, &p_device_count,
				       NULL) != VK_SUCCESS)
		return error_return;

	VkPhysicalDevice p_device_list[p_device_count];
	if (vkEnumeratePhysicalDevices(data->instance, &p_device_count,
				       p_device_list) != VK_SUCCESS)
		return error_return;

	int best_index = -1;
	for (int i = 0; i < p_device_count; i++) {
		VkPhysicalDeviceProperties p_device_properties;
		vkGetPhysicalDeviceProperties(p_device_list[i], &p_device_properties);
		if (p_device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			best_index = i;
		if (p_device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU && best_index == -1)
			best_index = i;
	}

	if (best_index == -1)
		return error_return;
	data->physical_device = p_device_list[best_index];
	return success_return;
}

int engine_find_queue_families(struct engine_data *data)
{
	vkGetPhysicalDeviceQueueFamilyProperties(data->physical_device,
						 &data->queue_families_count,
						 NULL);

	VkQueueFamilyProperties queue_family_properties[data->queue_families_count];
	vkGetPhysicalDeviceQueueFamilyProperties(data->physical_device,
						 &data->queue_families_count,
						 queue_family_properties);
	struct queue_family_indices indices;
	memset(&indices, -1, sizeof(struct queue_family_indices));

	for (int i = 0; i < data->queue_families_count; i++) {
		if (queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphics_family = i;
		}

		VkBool32 present_support = 0;
		vkGetPhysicalDeviceSurfaceSupportKHR(data->physical_device, i, data->surface, &present_support);
		if (present_support) {
			indices.present_family = i;
		}
		if (indices.graphics_family != -1 && indices.present_family != -1) {
			break;
		}
	}
	float queue_priorities = 1;

	data->queue_create_infos[0] = (VkDeviceQueueCreateInfo) {
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.queueFamilyIndex = indices.graphics_family,
		.queueCount = 1,
		.pQueuePriorities = &queue_priorities
	};

	data->queue_create_infos[1] = (VkDeviceQueueCreateInfo) {
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.queueFamilyIndex = indices.present_family,
		.queueCount = 1,
		.pQueuePriorities = &queue_priorities
	};

	return success_return;
}

int engine_create_device(struct engine_data *data)
{
	const char *const device_extension_list[] =  {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	VkDeviceCreateInfo create_info = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.queueCreateInfoCount = data->queue_families_count,
		.pQueueCreateInfos = data->queue_create_infos,
		.enabledLayerCount = 0,
		.ppEnabledLayerNames = NULL,
		.enabledExtensionCount = 1,
		.ppEnabledExtensionNames = device_extension_list,
		.pEnabledFeatures = &data->physical_device_features
	};
	if (vkCreateDevice(data->physical_device, &create_info, NULL, &data->device) != VK_SUCCESS)
		return error_return;
	return success_return;
}

int engine_set_surface_format(struct engine_data *data)
{
	uint32_t surface_format_count;
	vkGetPhysicalDeviceSurfaceFormatsKHR(data->physical_device, data->surface,
					     &surface_format_count, NULL);
	VkSurfaceFormatKHR surface_formats[surface_format_count];
	vkGetPhysicalDeviceSurfaceFormatsKHR(data->physical_device, data->surface,
					     &surface_format_count,
					     surface_formats);
	for (int i = 0; i < surface_format_count; i++) {
		if (surface_formats[i].format == VK_FORMAT_B8G8R8A8_SRGB &&
		    surface_formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			data->surface_format = surface_formats[i];
			return success_return;
		}
	}
	data->surface_format = surface_formats[0];
	return success_return;
}

int engine_setting_present_mode(struct engine_data *data)
{
	uint32_t present_mode_count;
	vkGetPhysicalDeviceSurfacePresentModesKHR(data->physical_device, data->surface,
						  &present_mode_count, NULL);
	VkPresentModeKHR present_modes[present_mode_count];
	vkGetPhysicalDeviceSurfacePresentModesKHR(data->physical_device, data->surface,
						  &present_mode_count,
						  present_modes);
	for (int i = 0; i < present_mode_count; i++) {
		if (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
			data->present_mode =  VK_PRESENT_MODE_MAILBOX_KHR;
			return success_return;
		}
	}
	data->present_mode = VK_PRESENT_MODE_FIFO_KHR;
	return success_return;
}

int engine_setting_swapchain_extent(struct engine_data *data)
{
	int width, height;
	glfwGetFramebufferSize(data->window, &width, &height);
	data->extent.height = height;
	data->extent.width = width;
	return success_return;
}

int engine_create_swapchain(struct engine_data *data)
{
	VkSurfaceCapabilitiesKHR capabilities;
	if(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(data->physical_device, data->surface,
						  &capabilities) != VK_SUCCESS) {
		printf("Failed to get physical device surface capabilities!\n");
		return error_return;
	}

	data->swapchain_image_count = capabilities.minImageCount + 1;
	if (data->swapchain_image_count > capabilities.maxImageCount &&
	    capabilities.maxImageCount != 0)
		data->swapchain_image_count = capabilities.maxImageCount;

	VkSwapchainCreateInfoKHR create_info = {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.pNext = NULL,
		.flags = 0,
		.surface = data->surface,
		.minImageCount = data->swapchain_image_count,
		.imageFormat = data->surface_format.format,
		.imageColorSpace = data->surface_format.colorSpace,
		.imageExtent = data->extent,
		.imageArrayLayers = 1,
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.preTransform = capabilities.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = data->present_mode,
		.clipped = VK_TRUE,
		.oldSwapchain = VK_NULL_HANDLE
	};

	uint32_t queue_families_indices[2];
	queue_families_indices[0] = data->queue_create_infos[0].queueFamilyIndex;
	queue_families_indices[1] = data->queue_create_infos[1].queueFamilyIndex;

	if (data->queue_create_infos[0].queueFamilyIndex != data->queue_create_infos[1].queueFamilyIndex) {
        	create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        	create_info.queueFamilyIndexCount = 2;
        	create_info.pQueueFamilyIndices = queue_families_indices;
        } else {
		create_info.queueFamilyIndexCount = 1,
		create_info.pQueueFamilyIndices = &queue_families_indices[0];
	}

	if (vkCreateSwapchainKHR(data->device, &create_info, NULL, &data->swapchain) != VK_SUCCESS) {
		return error_return;
	}
	return success_return;
}

int engine_create_image_views(struct engine_data *data)
{
	for (int i = 0; i < data->swapchain_image_count; i++) {
		VkImageViewCreateInfo create_info = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.pNext = NULL,
			.flags = 0,
			.image = data->swapchain_images[i],
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = data->surface_format.format,
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
		if (vkCreateImageView(data->device, &create_info, NULL, &data->image_views[i]) != VK_SUCCESS) {
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

int engine_create_shader_module(struct engine_data *data, const char *code, int code_length,
			 VkShaderModule *shader_module)
{
	VkShaderModuleCreateInfo create_info = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.codeSize = code_length,
		.pCode = (uint *)code
	};

	if (vkCreateShaderModule(data->device, &create_info, NULL, shader_module) != VK_SUCCESS) {
		printf("Failed to create Shader Module");
		return error_return;
	}
	return success_return;
}

int engine_create_render_pass(struct engine_data *data)
{
	VkAttachmentDescription color_attachment = {
		.flags = 0,
		.format = data->surface_format.format,
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

	VkSubpassDependency dependency = {
		.srcSubpass = VK_SUBPASS_EXTERNAL,
		.dstSubpass = 0,
		.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.srcAccessMask = 0,
		.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		.dependencyFlags = 0
	};

	VkRenderPassCreateInfo render_pass_info = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.attachmentCount = 1,
		.pAttachments = &color_attachment,
		.subpassCount = 1,
		.pSubpasses = &subpass,
		.dependencyCount = 1,
		.pDependencies = &dependency
	};


	if (vkCreateRenderPass(data->device, &render_pass_info, NULL, &data->render_pass) != VK_SUCCESS) {
		return error_return;
	}
	return success_return;
}

int engine_create_graphics_pipeline(struct engine_data *data)
{
	int length_vert;
	int length_frag;
	char *vert_shader_code = read_file("shaders/vert.spv", &length_vert);

	char *frag_shader_code = read_file("shaders/frag.spv", &length_frag);

	VkShaderModule vert_shader_module;
	engine_create_shader_module(data, vert_shader_code, length_vert,
			     &vert_shader_module);

	VkShaderModule frag_shader_module;
	engine_create_shader_module(data, frag_shader_code, length_frag,
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
		.primitiveRestartEnable = VK_FALSE
	};

	VkViewport viewport = {
		.x = 0.0f,
		.y = 0.0f,
		.width = (float) data->extent.width,
		.height = (float) data->extent.height,
		.minDepth = 0.0f,
		.maxDepth = 1.0f
	};

	VkRect2D scissor = {
		.offset = {0, 0},
		.extent = data->extent
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

	if (vkCreatePipelineLayout(data->device, &pipeline_layout_info, NULL,
				   &data->pipeline_layout) != VK_SUCCESS) {
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
		.layout = data->pipeline_layout,
		.renderPass = data->render_pass,
		.subpass = 0,
		.basePipelineHandle = VK_NULL_HANDLE,
		.basePipelineIndex = -1
	};

	if (vkCreateGraphicsPipelines(data->device, VK_NULL_HANDLE, 1,
				      &pipeline_info, NULL, &data->graphics_pipeline)
	    != VK_SUCCESS) {
		printf("failed to create graphics pipeline!");
		return error_return;
	}

	free(frag_shader_code);
	free(vert_shader_code);
	vkDestroyShaderModule(data->device, vert_shader_module, NULL);
	vkDestroyShaderModule(data->device, frag_shader_module, NULL);

	return success_return;
}

int engine_create_framebuffers(struct engine_data *data)
{
	for(int i = 0; i < data->swapchain_image_count; i++) {
		VkImageView attachments[] = {
			data->image_views[i]
		};

		VkFramebufferCreateInfo framebuffer_info = {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.pNext = NULL,
			.flags = 0,
			.renderPass = data->render_pass,
			.attachmentCount = 1,
			.pAttachments = attachments,
			.width = data->extent.width,
			.height = data->extent.height,
			.layers = 1
		};
		if(vkCreateFramebuffer(data->device, &framebuffer_info, 0,
		  		       &data->swapchain_framebuffers[i]) != VK_SUCCESS) {
			printf("Failed to create framebuffer!\n");
			return error_return;
		}
	}
	return success_return;
}

int engine_create_command_pool(struct engine_data *data)
{
	VkCommandPoolCreateInfo command_pool_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.pNext = NULL,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = data->queue_create_infos[0].queueFamilyIndex
	};

	if (vkCreateCommandPool(data->device, &command_pool_info, NULL, &data->command_pool)
	    != VK_SUCCESS) {
		printf("Failed to create command pool\n");
		return error_return;
	}
	return success_return;
}

int engine_create_command_buffers(struct engine_data *data) {
	VkCommandBufferAllocateInfo alloc_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext = NULL,
		.commandPool = data->command_pool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = (uint32_t)MAX_FRAMES_IN_FLIGHT
	};
	if (vkAllocateCommandBuffers(data->device, &alloc_info, data->command_buffers) !=
	    VK_SUCCESS) {
		printf("Failed to create command buffers\n");
		return error_return;
	}
	return success_return;
}

int engine_record_command_buffer(struct engine_data *data) {
	VkCommandBufferBeginInfo begin_info = {
		.sType =  VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = NULL,
		.flags = 0,
		.pInheritanceInfo = NULL,
	};
	VkCommandBuffer *command_buffer = &data->command_buffers[data->current_frame];
	if(vkBeginCommandBuffer(*command_buffer, &begin_info) != VK_SUCCESS) {
		printf("Failed to create begin recording command buffer!\n");
		return error_return;
	}

	VkClearValue clear_color = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
	VkRenderPassBeginInfo render_pass_info = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.pNext = NULL,
		.renderPass = data->render_pass,
		.framebuffer = data->swapchain_framebuffers[data->image_index],
		.renderArea = {
			.offset = {0, 0},
			.extent = data->extent
		},
		.clearValueCount = 1,
		.pClearValues = &clear_color
	};

	vkCmdBeginRenderPass(*command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(*command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
	data->graphics_pipeline);

	VkViewport viewport = {
		.x = 0.0f,
		.y = 0.0f,
		.width = (float)data->extent.width,
		.height = (float)data->extent.height,
		.minDepth = 0.0f,
		.maxDepth = 1.0f
	};
	vkCmdSetViewport(*command_buffer, 0, 1, &viewport);

	VkRect2D scissor = {
		.offset = {0, 0},
		.extent = data->extent
	};

	vkCmdDraw(*command_buffer, 3, 1, 0, 0);

	vkCmdEndRenderPass(*command_buffer);

	if(vkEndCommandBuffer(*command_buffer) != VK_SUCCESS) {
		printf("failed to record command buffer");
		return error_return;
	}
	return success_return;
}

int engine_create_sync_objects(struct engine_data *data)
{
	VkSemaphoreCreateInfo semaphore_create_info = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0
	};


	VkFenceCreateInfo fence_info = {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.pNext = NULL,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT
	};

	for(int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		if (vkCreateSemaphore(data->device, &semaphore_create_info, NULL, &data->image_available_semaphores[i]) != VK_SUCCESS ||
		    vkCreateSemaphore(data->device, &semaphore_create_info, NULL, &data->render_finished_semaphores[i]) != VK_SUCCESS ||
		    vkCreateFence(data->device, &fence_info, NULL, &data->in_flight_fences[i]) != VK_SUCCESS) {
			printf("failed to create synchronization objects for a frame!");
			return error_return;
		}
	}
	return success_return;
}

void engine_cleanup_swapchain(struct engine_data *data)
{
	for(int i = 0; i < data->swapchain_image_count; i++) {
		vkDestroyFramebuffer(data->device, data->swapchain_framebuffers[i], NULL);
	}
	for (int i = 0; i < data->swapchain_image_count; i++) {
		vkDestroyImageView(data->device, data->image_views[i], NULL);
	}
	vkDestroySwapchainKHR(data->device, data->swapchain, NULL);
}

int engine_recreate_swapchain(struct engine_data *data) {
	int width = 0, height = 0;
        glfwGetFramebufferSize(data->window, &width, &height);

        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(data->window, &width, &height);
            glfwWaitEvents();
        }

	vkDeviceWaitIdle(data->device);

	engine_cleanup_swapchain(data);

	engine_set_surface_format(data);
	engine_setting_present_mode(data);
	engine_setting_swapchain_extent(data);

	engine_create_swapchain(data);

	vkGetSwapchainImagesKHR(data->device, data->swapchain, &data->swapchain_image_count, NULL);
	data->swapchain_images = malloc(sizeof(VkImage) * data->swapchain_image_count);
	vkGetSwapchainImagesKHR(data->device, data->swapchain, &data->swapchain_image_count, data->swapchain_images);

	data->image_views = malloc(sizeof(VkImageView) * data->swapchain_image_count);
	data->swapchain_framebuffers = malloc(sizeof(VkFramebuffer) * data->swapchain_image_count);

	engine_create_image_views(data);

	engine_create_framebuffers(data);
	return success_return;
}

int engine_draw_frame(struct engine_data *data)
{	

	vkWaitForFences(data->device, 1, &data->in_flight_fences[data->current_frame], VK_TRUE, UINT64_MAX);

	VkResult result = vkAcquireNextImageKHR(data->device, data->swapchain, UINT64_MAX, data->image_available_semaphores[data->current_frame], VK_NULL_HANDLE, &data->image_index);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		engine_recreate_swapchain(data);
		return success_return;
	} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		printf("failed to acquire swap chain image!");
		return error_return;
	}

	vkResetFences(data->device, 1, &data->in_flight_fences[data->current_frame]);
	
	vkResetCommandBuffer(data->command_buffers[data->current_frame], 0);
	engine_record_command_buffer(data);

	VkSemaphore wait_semaphores[] = {data->image_available_semaphores[data->current_frame]};
	VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	VkSemaphore signal_semaphores[] = {data->render_finished_semaphores[data->current_frame]};

	VkSubmitInfo submit_info = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext = NULL,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = wait_semaphores,
		.pWaitDstStageMask = wait_stages,
		.commandBufferCount = 1,
		.pCommandBuffers = &data->command_buffers[data->current_frame],
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = signal_semaphores
	};
	if(vkQueueSubmit(data->graphics_queue, 1, &submit_info, data->in_flight_fences[data->current_frame]) != VK_SUCCESS) {
		printf("failed to submit draw command buffer!");
		return error_return;
	}
	VkSwapchainKHR swapchains[] = {data->swapchain};

	VkPresentInfoKHR present_info = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.pNext = NULL,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = signal_semaphores,
		.swapchainCount = 1,
		.pSwapchains = swapchains,
		.pImageIndices = &data->image_index,
		.pResults = NULL
	};
	result = vkQueuePresentKHR(data->present_queue, &present_info);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebuffer_resized) {
		framebuffer_resized = 0;
		engine_recreate_swapchain(data);
	} else if (result != VK_SUCCESS) {
		printf("failed to present swap chain image!");
		return error_return;
	}
	return success_return;
}


int main()
{
	struct engine_data data;
	memset(&data, 0, sizeof(struct engine_data));
	engine_init_window(&data);

	if (engine_create_vulkan_instance(&data) != success_return) {
		printf("Unable to create instance Aborting");
		return error_return;
	}

	if (glfwCreateWindowSurface(data.instance, data.window, NULL, &data.surface) != VK_SUCCESS) {
		printf("Failed to create window surface!");
		return error_return;
	}

	if (engine_pick_physical_device(&data) != success_return) {
		printf("Unable to pick device! Aborting");
		return error_return;
	}

	vkGetPhysicalDeviceFeatures(data.physical_device, &data.physical_device_features);
	vkGetPhysicalDeviceProperties(data.physical_device, &data.physical_device_properties);

	if(engine_find_queue_families(&data) != success_return) {
		printf("Failed to find appropriate queue families");
		return error_return;
	}

	data.queue_families_count = data.queue_create_infos[0].queueFamilyIndex == data.queue_create_infos[1].queueFamilyIndex ? 1 : 2;

	if(engine_create_device(&data) != success_return) {
		printf("Failed to create logical device! Aborting");
		return error_return;
	}

	vkGetDeviceQueue(data.device, data.queue_create_infos[0].queueFamilyIndex, 0, &data.graphics_queue);

	vkGetDeviceQueue(data.device, data.queue_create_infos[1].queueFamilyIndex, 0, &data.present_queue);

	engine_set_surface_format(&data);

	engine_setting_present_mode(&data);

	engine_setting_swapchain_extent(&data);


	if (engine_create_swapchain(&data) != success_return) {
		printf("Failed to create swap chain! Aborting");
		return error_return;
	}

	vkGetSwapchainImagesKHR(data.device, data.swapchain, &data.swapchain_image_count, NULL);
	data.swapchain_images = malloc(sizeof(VkImage) * data.swapchain_image_count);
	vkGetSwapchainImagesKHR(data.device, data.swapchain, &data.swapchain_image_count, data.swapchain_images);

	data.image_views = malloc(sizeof(VkImageView) * data.swapchain_image_count);
	data.swapchain_framebuffers = malloc(sizeof(VkFramebuffer) * data.swapchain_image_count);

	if (engine_create_image_views(&data) != success_return) {
		printf("Failed to create Image Views");
		return error_return;
	}

	if(engine_create_render_pass(&data) !=
	   success_return) {
		printf("failed to create render pass!");
		return error_return;
	}

	if(engine_create_graphics_pipeline(&data)
	   != success_return) {
		return error_return;
	}

	if(engine_create_framebuffers(&data) != success_return) {
		return error_return;
	}

	if(engine_create_command_pool(&data) != success_return) {
		return error_return;
	}

	if(engine_create_command_buffers(&data)) {
		return error_return;
	}


	engine_create_sync_objects(&data);
	while (!glfwWindowShouldClose(data.window)) {
		glfwPollEvents();
		if(engine_draw_frame(&data) != success_return) {
			return error_return;
		}
		data.current_frame = (data.current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
	}
	vkDeviceWaitIdle(data.device);

	engine_cleanup_swapchain(&data);

	vkDestroyPipeline(data.device, data.graphics_pipeline, NULL);
	vkDestroyPipelineLayout(data.device, data.pipeline_layout, NULL);

	vkDestroyRenderPass(data.device, data.render_pass, NULL);

	for(int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vkDestroySemaphore(data.device, data.image_available_semaphores[i], NULL);
		vkDestroySemaphore(data.device, data.render_finished_semaphores[i], NULL);
		vkDestroyFence(data.device, data.in_flight_fences[i], NULL);
	}

	vkDestroyCommandPool(data.device, data.command_pool, NULL);

	vkDestroyDevice(data.device, NULL);

	vkDestroySurfaceKHR(data.instance, data.surface, NULL);

	vkDestroyInstance(data.instance, NULL);

	glfwDestroyWindow(data.window);
	glfwTerminate();
}
