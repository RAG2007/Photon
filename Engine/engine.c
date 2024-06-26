#include <stdint.h>
#include <stdio.h>
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
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#define GLFW_INCLUDE_VULKAN

#define UNLIKELY(expression) __builtin_expect (expression, 0)

unsigned int WIDTH = 1000;
unsigned int HEIGHT = 1000;

#define MAX_FRAMES_IN_FLIGHT 2

int error_return = 1, success_return = 0;

int window_pointer;

struct uniform_buffer_widow_size {
	unsigned int width;
	unsigned int height;
};

struct queue_family_indices {
	int graphics_family;
	int present_family;
};

struct vertex {
	float position[3];
	float color[3];
};

struct circle {
	float position[3];
	float color[3];
	//float radius;
};

int triangle_vertices_count = 6;
const struct vertex triangle_vertices[] = {
	{{-0.9f, -0.9f, 0.0f}, {1.0f, 1.0f, 1.0f}},
	{{0.7f, -0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
	{{0.9f, 0.6f, 0.0f}, {1.0f, 1.0f, 1.0f}},
	{{-0.9f, 0.9f, 0.2f}, {1.0f, 0.0f, 1.0f}},
	{{0.8f, -0.6f, 0.2f}, {1.0f, 0.0f, 1.0f}},
	{{0.2f, 0.8f, 0.8f}, {1.0f, 0.0f, 1.0f}}
};

uint32_t triangle_index_count = 6;
const uint32_t triangle_indices[] = {
	0, 1, 2, 3, 4, 5
};

int circle_vertices_count = 3;
const struct circle circle_vertices[] = {
	{{-0.8f, 0.6f, 0.1f}, {1.0f, 0.0f, 0.0f}},
	{{0.5f, -0.8f, 0.1f}, {1.0f, 0.0f, 0.0f}},
	{{0.5f, -0.2f, 0.1f}, {1.0f, 0.0f, 0.0f}}
};

uint32_t circle_index_count = 3;
const uint32_t circle_indices[] = {
	0, 1, 2
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
	VkExtent2D swapchain_extent;
	
	VkSwapchainKHR swapchain;
	uint32_t swapchain_image_count;
	VkImage *swapchain_images;
	VkImageView *swapchain_image_views;
	VkShaderModule shader_module;
	VkRenderPass render_pass;

	VkPipelineLayout triangle_pipeline_layout;
	VkPipeline triangle_graphics_pipeline;

	VkDescriptorSetLayout descriptor_set_layout;

	VkPipelineLayout circle_pipeline_layout;
	VkPipeline circle_graphics_pipeline;

	VkFramebuffer *swapchain_framebuffers;
	VkCommandPool graphics_command_pool;
	VkCommandPool copy_vertex_buffer_command_pool;


	VkBuffer triangle_staging_buffer;
	VkMemoryRequirements triangle_staging_memory_requirements;
	VkDeviceMemory triangle_staging_buffer_memory;
	VkBuffer triangle_main_vertex_buffer;
	VkBuffer triangle_main_index_buffer;
	VkDeviceMemory triangle_main_vertex_buffer_memory;
	VkDeviceMemory triangle_main_index_buffer_memory;

	VkBuffer circle_staging_buffer;
	VkMemoryRequirements circle_staging_memory_requirements;
	VkDeviceMemory circle_staging_buffer_memory;
	VkBuffer circle_main_vertex_buffer;
	VkBuffer circle_main_index_buffer;
	VkDeviceMemory circle_main_vertex_buffer_memory;
	VkDeviceMemory circle_main_index_buffer_memory;

	struct uniform_buffer_widow_size uniform_buffer;
	VkBuffer uniform_buffers[MAX_FRAMES_IN_FLIGHT];
	VkDeviceMemory uniform_buffers_memory[MAX_FRAMES_IN_FLIGHT];
	void* uniform_buffers_mapped[MAX_FRAMES_IN_FLIGHT];
	
	VkDescriptorPool descriptor_pool;
	VkDescriptorSet descriptor_sets[MAX_FRAMES_IN_FLIGHT];
	VkFence vertex_buffer_copy_fence;
	VkImage depth_image;
	VkDeviceMemory depth_image_memory;
	VkImageView depth_image_view;
	VkFormat depth_format;

	VkCommandBuffer command_buffers[2];
	uint32_t current_image_index;
	VkSemaphore image_available_semaphores[2];
	VkSemaphore render_finished_semaphores[2];
	VkFence in_flight_fences[2];
	uint32_t current_frame;
};

int framebuffer_resized = 0;

void *user_window_pointer;

int engine_descriptor_set_layout(struct engine_data *data) {
	VkDescriptorSetLayoutBinding uniform_buffer_layout_binding = {	
		.binding = 0,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		.pImmutableSamplers = NULL,
	};

	VkDescriptorSetLayoutCreateInfo layout_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.bindingCount = 1,
		.pBindings = &uniform_buffer_layout_binding
	};

	if (vkCreateDescriptorSetLayout(data->device, &layout_info, NULL, &data->descriptor_set_layout) != VK_SUCCESS) {
		printf("Failed to create descriptor set layout! Aborting\n");
		return error_return;
	}
	return success_return;
}


VkVertexInputBindingDescription get_vertex_binding_description()
{
	VkVertexInputBindingDescription binding_description = {
		.binding = 0,
		.stride = sizeof(struct vertex),
		.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
	};
	return binding_description;
}

VkVertexInputAttributeDescription *get_vertex_input_attribute_description(int vertex_type)
{
	if(vertex_type == 0) {
		VkVertexInputAttributeDescription *attribute_descriptions = malloc(2 * sizeof(VkVertexInputAttributeDescription));
		attribute_descriptions[0] = (VkVertexInputAttributeDescription){
			.location = 0,
			.binding = 0,
			.format = VK_FORMAT_R32G32B32_SFLOAT,
			.offset = offsetof(struct vertex, position)
		};
		attribute_descriptions[1] = (VkVertexInputAttributeDescription){
			.location = 1,
			.binding = 0,
			.format = VK_FORMAT_R32G32B32_SFLOAT,
			.offset = offsetof(struct vertex, color)
		};
		return attribute_descriptions;
	}
	VkVertexInputAttributeDescription *attribute_descriptions = malloc(2 * sizeof(VkVertexInputAttributeDescription));
	attribute_descriptions[0] = (VkVertexInputAttributeDescription){
		.location = 0,
		.binding = 0,
		.format = VK_FORMAT_R32G32B32_SFLOAT,
		.offset = offsetof(struct circle, position)
	};
	attribute_descriptions[1] = (VkVertexInputAttributeDescription){
		.location = 1,
		.binding = 0,
		.format = VK_FORMAT_R32G32B32_SFLOAT,
		.offset = offsetof(struct circle, color)
	};
	/*attribute_descriptions[2] = (VkVertexInputAttributeDescription){
		.location = 2,
		.binding = 0,
		.format = VK_FORMAT_R32_SFLOAT,
		.offset = offsetof(struct circle, radius)
	};*/
	return attribute_descriptions;
}

void engine_resize_callback(GLFWwindow* window, int width, int height)
{
	framebuffer_resized = 1;
}

void engine_init_window(struct engine_data *data)
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	data->window = glfwCreateWindow(WIDTH, HEIGHT, "Photon", NULL, NULL);
	glfwSetWindowUserPointer(data->window, user_window_pointer);
	glfwSetFramebufferSizeCallback(data->window, engine_resize_callback);
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

	if (UNLIKELY(vkCreateInstance(&create_info, NULL, &data->instance) != VK_SUCCESS)) {
		printf("Failed to create instance! Aborting\n");
		return error_return;
	}
	return success_return;
}

int engine_pick_physical_device(struct engine_data *data)
{
	uint32_t p_device_count = 0;
	if (UNLIKELY(vkEnumeratePhysicalDevices(data->instance, &p_device_count,
				       NULL) != VK_SUCCESS))
		return error_return;

	VkPhysicalDevice p_device_list[p_device_count];
	if (UNLIKELY(vkEnumeratePhysicalDevices(data->instance, &p_device_count,
				       p_device_list) != VK_SUCCESS))
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
	if (UNLIKELY(vkCreateDevice(data->physical_device, &create_info, NULL, &data->device) != VK_SUCCESS)) {
		printf("failed to create device! Aborting\n");
		return error_return;
	}
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

int engine_setting_swapchain_extent(struct engine_data *data)
{
	int width, height;
	glfwGetFramebufferSize(data->window, &width, &height);
	data->swapchain_extent.height = height;
	data->swapchain_extent.width = width;
	return success_return;
}

int engine_create_swapchain(struct engine_data *data)
{
	VkSurfaceCapabilitiesKHR capabilities;
	if (UNLIKELY(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(data->physical_device, data->surface,
						  &capabilities) != VK_SUCCESS)) {
		printf("Failed to get physical device surface capabilities! Aborting\n");
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
		.imageExtent = data->swapchain_extent,
		.imageArrayLayers = 1,
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.preTransform = capabilities.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = VK_PRESENT_MODE_FIFO_KHR,
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

	if (UNLIKELY(vkCreateSwapchainKHR(data->device, &create_info, NULL, &data->swapchain) != VK_SUCCESS)) {
		printf("Failed to create swapchain! Aborting\n");
		return error_return;
	}
	return success_return;
}

int engine_create_image_view(struct engine_data *data, VkImage image, VkFormat format, VkImageView *image_view)
{
	VkImageViewCreateInfo create_info = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.image = image,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = format, //data->surface_format.format
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
	if (UNLIKELY(vkCreateImageView(data->device, &create_info, NULL, image_view) != VK_SUCCESS))
		return error_return;
	return success_return;
}


char *engine_read_file(char *name, int *length)
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

	if (UNLIKELY(vkCreateShaderModule(data->device, &create_info, NULL, shader_module) != VK_SUCCESS)) {
		printf("Failed to create Shader Module! Aborting\n");
		return error_return;
	}
	return success_return;
}

int engine_find_supported_depth_format(struct engine_data *data, VkFormat *format) {
	VkFormatProperties properties;
	VkFormatFeatureFlags features = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
	vkGetPhysicalDeviceFormatProperties(data->physical_device, VK_FORMAT_D32_SFLOAT,&properties);
	if ((properties.optimalTilingFeatures & features) == features) {
		*format = VK_FORMAT_D32_SFLOAT;
		return success_return;
	}
	return error_return;
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
	VkFormat depth_format;
	engine_find_supported_depth_format(data, &depth_format);
	VkAttachmentDescription depth_attachment = {
		.format = depth_format,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};
        
	VkAttachmentReference color_attachment_ref = {
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	VkAttachmentReference depth_attachment_ref = {
		.attachment = 1,
		.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
	};

	VkSubpassDescription subpass = {
		.flags = 0,
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.inputAttachmentCount = 0,
		.pInputAttachments = 0,
		.colorAttachmentCount = 1,
		.pColorAttachments = &color_attachment_ref,
		.pResolveAttachments = NULL,
		.pDepthStencilAttachment = &depth_attachment_ref,
		.preserveAttachmentCount = 0,
		.pPreserveAttachments = NULL
	};

	VkSubpassDependency dependency = {
		.srcSubpass = VK_SUBPASS_EXTERNAL,
		.dstSubpass = 0,
		.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
		.srcAccessMask = 0,
		.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
		.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		.dependencyFlags = 0
	};

	VkAttachmentDescription attachments[] = {
		color_attachment,
		depth_attachment
	};

	VkRenderPassCreateInfo render_pass_info = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.attachmentCount = 2,
		.pAttachments = attachments,
		.subpassCount = 1,
		.pSubpasses = &subpass,
		.dependencyCount = 1,
		.pDependencies = &dependency
	};

	if (UNLIKELY(vkCreateRenderPass(data->device, &render_pass_info, NULL, &data->render_pass) != VK_SUCCESS))
		return error_return;
	return success_return;
}

int engine_create_graphics_pipeline(struct engine_data *data, VkPipeline *graphics_pipeline, VkPipelineLayout *pipeline_layout, char *vert_dir, char *frag_dir, int vertex_type)
{
	int length_vert;
	int length_frag;
	char *vert_shader_code = engine_read_file(vert_dir, &length_vert);

	char *frag_shader_code = engine_read_file(frag_dir, &length_frag);

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

	VkVertexInputBindingDescription binding_description = get_vertex_binding_description();

	VkVertexInputAttributeDescription *attribute_descriptions = get_vertex_input_attribute_description(vertex_type);
	int attribute_description_count = 2;
	/*if(vertex_type == 1) {
		attribute_description_count = 3;
	}*/
	VkPipelineVertexInputStateCreateInfo vertex_input_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.vertexBindingDescriptionCount = 1,
		.pVertexBindingDescriptions = &binding_description,
		.vertexAttributeDescriptionCount = attribute_description_count,
		.pVertexAttributeDescriptions = attribute_descriptions
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
		.width = (float) data->swapchain_extent.width,
		.height = (float) data->swapchain_extent.height,
		.minDepth = 0.0f,
		.maxDepth = 1.0f
	};

	VkRect2D scissor = {
		.offset = {0, 0},
		.extent = data->swapchain_extent
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
		.cullMode = VK_CULL_MODE_NONE,
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

	VkPipelineDepthStencilStateCreateInfo depth_stencil = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthTestEnable = VK_TRUE,
		.depthWriteEnable = VK_TRUE,
		.depthCompareOp = VK_COMPARE_OP_LESS,
		.depthBoundsTestEnable = VK_FALSE,
		.stencilTestEnable = VK_FALSE
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
		.pushConstantRangeCount = 0,
		.pPushConstantRanges = NULL
	};
	if(vertex_type == 1) {
		pipeline_layout_info.setLayoutCount = 1;
		pipeline_layout_info.pSetLayouts = &data->descriptor_set_layout;
	} else {
		pipeline_layout_info.setLayoutCount = 0;
		pipeline_layout_info.pSetLayouts = NULL;
	}

	if (UNLIKELY(vkCreatePipelineLayout(data->device, &pipeline_layout_info, NULL,
				   pipeline_layout) != VK_SUCCESS)) {
		printf("Failed to create pipeline layout!\n");
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
		.pDepthStencilState = &depth_stencil,
		.pColorBlendState = &color_blending,
		.pDynamicState = &dynamic_state,
		.layout = *pipeline_layout,
		.renderPass = data->render_pass,
		.subpass = 0,
		.basePipelineHandle = VK_NULL_HANDLE,
		.basePipelineIndex = -1
	};

	if (UNLIKELY(vkCreateGraphicsPipelines(data->device, VK_NULL_HANDLE, 1,
				      &pipeline_info, NULL, graphics_pipeline) 
		     != VK_SUCCESS)) {
		printf("failed to create graphics pipeline! Aborting\n");
		return error_return;
	}

	free(attribute_descriptions);
	free(frag_shader_code);
	free(vert_shader_code);

	vkDestroyShaderModule(data->device, vert_shader_module, NULL);
	vkDestroyShaderModule(data->device, frag_shader_module, NULL);

	return success_return;
}

int engine_create_framebuffers(struct engine_data *data)
{
	for (int i = 0; i < data->swapchain_image_count; i++) {
		VkImageView attachments[] = {
			data->swapchain_image_views[i],
			data->depth_image_view
		};

		VkFramebufferCreateInfo framebuffer_info = {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.pNext = NULL,
			.flags = 0,
			.renderPass = data->render_pass,
			.attachmentCount = 2,
			.pAttachments = attachments,
			.width = data->swapchain_extent.width,
			.height = data->swapchain_extent.height,
			.layers = 1
		};
		if (UNLIKELY(vkCreateFramebuffer(data->device, &framebuffer_info, 0,
		  		       &data->swapchain_framebuffers[i]) != VK_SUCCESS)) {
			printf("Failed to create framebuffer! Aborting\n");
			return error_return;
		}
	}
	return success_return;
}

int engine_create_command_pool(struct engine_data *data, int is_vertex_buffer_pool)
{
	VkCommandPoolCreateInfo command_pool_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.pNext = NULL,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = data->queue_create_infos[0].queueFamilyIndex
	};

	VkCommandPool *command_pool = &data->graphics_command_pool;
	if(is_vertex_buffer_pool) {
		command_pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
		command_pool = &data->copy_vertex_buffer_command_pool;
	}
	
	if (UNLIKELY(vkCreateCommandPool(data->device, &command_pool_info, NULL, command_pool)
	    != VK_SUCCESS)) {
		printf("Failed to create command pool! Aborting\n");
		return error_return;
	}
	return success_return;
}

int engine_find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties, struct engine_data *data)
{
	VkPhysicalDeviceMemoryProperties memory_properties;
	vkGetPhysicalDeviceMemoryProperties(data->physical_device, &memory_properties);

	for(int i = 0; i < memory_properties.memoryTypeCount; i++) {
		if (type_filter & (1 << i) && (memory_properties.memoryTypes[i].propertyFlags & properties) == properties)
			return i;
	}
	printf("failed to find suitable memory type! Aborting\n");
	return error_return;
}

int engine_create_image(struct engine_data *data, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage *image, VkDeviceMemory *image_memory) {
	VkImageCreateInfo imageInfo = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.extent.width = width,
		.extent.height = height,
		.extent.depth = 1,
		.mipLevels = 1,
		.arrayLayers = 1,
		.format = format,
		.tiling = tiling,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.usage = usage,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE
	};

	if (vkCreateImage(data->device, &imageInfo, NULL, image) != VK_SUCCESS) {
		printf("failed to create image! Aborting\n");
		return error_return;
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(data->device, *image, &memRequirements);

	VkMemoryAllocateInfo allocInfo =  {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = memRequirements.size,
		.memoryTypeIndex = engine_find_memory_type(memRequirements.memoryTypeBits, properties, data),
	};

	if (vkAllocateMemory(data->device, &allocInfo, NULL, image_memory) != VK_SUCCESS) {
		printf("failed to allocate image memory! Aborting\n");
		return error_return;
	}

	vkBindImageMemory(data->device, *image, *image_memory, 0);
	return success_return;
}

int engine_create_depth_resources(struct engine_data *data) {
	VkFormat format;
	if(engine_find_supported_depth_format(data, &format) != success_return) {
		printf("failed to get supported format! Aborting\n");
		return error_return;
	}
	engine_create_image(data, data->swapchain_extent.width, data->swapchain_extent.height, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &data->depth_image, &data->depth_image_memory);
	if(engine_create_image_view(data, data->depth_image, format, &data->depth_image_view) != success_return) {
		printf("failed to create image view! Aborting\n");
		return error_return;
	}
	return success_return;
}


int engine_create_buffer(struct engine_data *data, VkDeviceSize size,
	VkBufferUsageFlags usage,
	VkMemoryPropertyFlags properties,
	VkBuffer *buffer,
	VkDeviceMemory *buffer_memory)
{
	VkBufferCreateInfo buffer_create_info = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.size = size,
		.usage =  usage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = NULL
	};

	if (vkCreateBuffer(data->device, &buffer_create_info, NULL, buffer)
	    != VK_SUCCESS) {
		printf("failed to create buffer! Aborting\n");
		return error_return;
	}

	VkMemoryRequirements memory_requirements;
	vkGetBufferMemoryRequirements(data->device, *buffer, &memory_requirements);

	VkMemoryAllocateInfo allocation_info = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = NULL,
		.allocationSize = memory_requirements.size,
		.memoryTypeIndex = engine_find_memory_type(memory_requirements.memoryTypeBits, properties, data)
	};

	if(UNLIKELY(vkAllocateMemory(data->device, &allocation_info, NULL, buffer_memory) != VK_SUCCESS)) {
		printf("Failed to allocate buffer memory! Aborting\n");
		return error_return;
	}
	vkBindBufferMemory(data->device, *buffer, *buffer_memory, 0);
	return success_return;
}

void engine_copy_buffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size, struct engine_data *data)
{
	VkCommandBufferAllocateInfo allocation_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext = NULL,
		.commandPool = data->copy_vertex_buffer_command_pool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1
	};

	VkCommandBuffer command_buffer;
	vkAllocateCommandBuffers(data->device, &allocation_info, &command_buffer);

	VkCommandBufferBeginInfo begin_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = NULL,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		.pInheritanceInfo = NULL
	};

	vkBeginCommandBuffer(command_buffer, &begin_info);

	VkBufferCopy copy_region = {
		.srcOffset = 0,
		.dstOffset = 0,
		.size = size
	};

	vkCmdCopyBuffer(command_buffer, src_buffer, dst_buffer, 1, &copy_region);

	vkEndCommandBuffer(command_buffer);

	VkSubmitInfo submit_info = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext = NULL,
		.waitSemaphoreCount = 0,
		.pWaitSemaphores = NULL,
		.pWaitDstStageMask = NULL,
		.commandBufferCount = 1,
		.pCommandBuffers = &command_buffer,
		.signalSemaphoreCount = 0,
		.pSignalSemaphores = NULL
	};

	vkQueueSubmit(data->graphics_queue, 1, &submit_info, data->vertex_buffer_copy_fence);

	vkWaitForFences(data->device, 1, &data->vertex_buffer_copy_fence, VK_TRUE, UINT64_MAX);

	vkFreeCommandBuffers(data->device, data->copy_vertex_buffer_command_pool, 1, &command_buffer);

	vkResetFences(data->device, 1, &data->vertex_buffer_copy_fence);
}

int engine_create_vertex_buffers(struct engine_data *data)
{
	VkDeviceSize triangle_buffer_size = sizeof(struct vertex) * triangle_vertices_count;
	
	engine_create_buffer(data, triangle_buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &data->triangle_staging_buffer,
		      &data->triangle_staging_buffer_memory);

	void* triangle_vertex_buffer_data;
	vkMapMemory(data->device, data->triangle_staging_buffer_memory, 0, triangle_buffer_size, 0, &triangle_vertex_buffer_data);
	memcpy(triangle_vertex_buffer_data, triangle_vertices, triangle_buffer_size);
	vkUnmapMemory(data->device, data->triangle_staging_buffer_memory);

	engine_create_buffer(data, triangle_buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT |
		     VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &data->triangle_main_vertex_buffer,
		     &data->triangle_main_vertex_buffer_memory);

	engine_copy_buffer(data->triangle_staging_buffer, data->triangle_main_vertex_buffer, triangle_buffer_size, data);


	vkDestroyBuffer(data->device, data->triangle_staging_buffer, NULL);
	vkFreeMemory(data->device, data->triangle_staging_buffer_memory, NULL);

	VkDeviceSize circle_buffer_size = sizeof(struct circle) * circle_vertices_count;
	
	engine_create_buffer(data, circle_buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &data->circle_staging_buffer,
		      &data->circle_staging_buffer_memory);

	void* circle_vertex_buffer_data;
	vkMapMemory(data->device, data->circle_staging_buffer_memory, 0, circle_buffer_size, 0, &circle_vertex_buffer_data);
	memcpy(circle_vertex_buffer_data, circle_vertices, circle_buffer_size);
	vkUnmapMemory(data->device, data->circle_staging_buffer_memory);

	engine_create_buffer(data, circle_buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT |
		     VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &data->circle_main_vertex_buffer,
		     &data->circle_main_vertex_buffer_memory);

	engine_copy_buffer(data->circle_staging_buffer, data->circle_main_vertex_buffer, circle_buffer_size, data);

	vkDestroyBuffer(data->device, data->circle_staging_buffer, NULL);
	vkFreeMemory(data->device, data->circle_staging_buffer_memory, NULL);
	
	return success_return;
}

int engine_create_index_buffers(struct engine_data *data)
{
	VkDeviceSize buffer_size = sizeof(uint32_t) * triangle_index_count;
	
	engine_create_buffer(data, buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			     &data->triangle_staging_buffer,
		    	     &data->triangle_staging_buffer_memory);

	void* index_buffer_data;
	vkMapMemory(data->device, data->triangle_staging_buffer_memory, 0, buffer_size, 0, &index_buffer_data);
	memcpy(index_buffer_data, triangle_indices, buffer_size);
	vkUnmapMemory(data->device, data->triangle_staging_buffer_memory);

	engine_create_buffer(data, buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT |
			     VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			     &data->triangle_main_index_buffer,
		   	     &data->triangle_main_index_buffer_memory);

	engine_copy_buffer(data->triangle_staging_buffer, data->triangle_main_index_buffer, buffer_size, data);

	vkDestroyBuffer(data->device, data->triangle_staging_buffer, NULL);
	vkFreeMemory(data->device, data->triangle_staging_buffer_memory, NULL);

	buffer_size = sizeof(uint32_t) * circle_index_count;
	
	engine_create_buffer(data, buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			     &data->circle_staging_buffer,
		    	     &data->circle_staging_buffer_memory);

	vkMapMemory(data->device, data->circle_staging_buffer_memory, 0, buffer_size, 0, &index_buffer_data);
	memcpy(index_buffer_data, circle_indices, buffer_size);
	vkUnmapMemory(data->device, data->circle_staging_buffer_memory);

	engine_create_buffer(data, buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT |
			     VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			     &data->circle_main_index_buffer,
		   	     &data->circle_main_index_buffer_memory);

	engine_copy_buffer(data->circle_staging_buffer, data->circle_main_index_buffer, buffer_size, data);

	vkDestroyBuffer(data->device, data->circle_staging_buffer, NULL);
	vkFreeMemory(data->device, data->circle_staging_buffer_memory, NULL);
	return success_return;
}

int engine_create_uniform_buffers(struct engine_data *data) {
	VkDeviceSize buffer_size = sizeof(struct uniform_buffer_widow_size);

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		if (engine_create_buffer(data, buffer_size,  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&data->uniform_buffers[i], &data->uniform_buffers_memory[i]) != success_return) {
			return error_return;
		}

		if (vkMapMemory(data->device, data->uniform_buffers_memory[i], 0, buffer_size,0, &data->uniform_buffers_mapped[i]) != VK_SUCCESS) {
			return error_return;
		}
	}
	return success_return;
}

int engine_create_descriptor_pool(struct engine_data *data) {
	VkDescriptorPoolSize  pool_size = {
		.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = (uint32_t)MAX_FRAMES_IN_FLIGHT
	};
	
	VkDescriptorPoolCreateInfo pool_create_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.maxSets = (uint32_t)MAX_FRAMES_IN_FLIGHT,
		.poolSizeCount = 1,
		.pPoolSizes = &pool_size
	};

	if (vkCreateDescriptorPool(data->device, &pool_create_info, NULL, &data->descriptor_pool) != VK_SUCCESS) {
		printf("Failed to create descriptor pool! Aborting\n");
		return error_return;
	}
	return success_return;
}

int engine_create_command_buffers(struct engine_data *data)	
{
	VkCommandBufferAllocateInfo alloc_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext = NULL,
		.commandPool = data->graphics_command_pool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = (uint32_t)MAX_FRAMES_IN_FLIGHT
	};

	if (vkAllocateCommandBuffers(data->device, &alloc_info, data->command_buffers) !=
	    VK_SUCCESS) {
		printf("Failed to create command buffers! Aborting\n");
		return error_return;
	}
	return success_return;
}

int engine_create_descriptor_sets(struct engine_data *data) {
	VkDescriptorSetLayout layouts[MAX_FRAMES_IN_FLIGHT];
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		layouts[i] = data->descriptor_set_layout;
	
	VkDescriptorSetAllocateInfo alloc_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext = NULL,
		.descriptorPool = data->descriptor_pool,
		.descriptorSetCount = (uint32_t)MAX_FRAMES_IN_FLIGHT,
		.pSetLayouts = layouts
	};
	if (vkAllocateDescriptorSets(data->device, &alloc_info, data->descriptor_sets) != VK_SUCCESS) {
		printf("Failed to allocate descriptor sets! Aborting\n");
		return error_return;
	}

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		VkDescriptorBufferInfo buffer_info = {
			.buffer = data->uniform_buffers[i],
			.offset = 0,
			.range = sizeof(struct uniform_buffer_widow_size)
		};

		VkWriteDescriptorSet descriptor_write = {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.pNext = NULL,
			.dstSet = data->descriptor_sets[i],
			.dstBinding = 0,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.pImageInfo = NULL,
			.pBufferInfo = &buffer_info,
			.pTexelBufferView = NULL
		};

		vkUpdateDescriptorSets(data->device, 1, &descriptor_write, 0, NULL);
	}
	return success_return;
}

int engine_record_command_buffer(struct engine_data *data)
{
	VkCommandBufferBeginInfo begin_info = {
		.sType =  VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = NULL,
		.flags = 0,
		.pInheritanceInfo = NULL,
	};
	VkCommandBuffer *command_buffer = &data->command_buffers[data->current_frame];
	if (UNLIKELY(vkBeginCommandBuffer(*command_buffer, &begin_info) != VK_SUCCESS)) {
		printf("Failed to create begin recording command buffer! Aborting\n");
		return error_return;
	}

	VkClearValue clear_values[2];
        clear_values[0].color = (VkClearColorValue){{0.0f, 0.0f, 0.0f, 1.0f}};
        clear_values[1].depthStencil = (VkClearDepthStencilValue){1.0f, 0.0f};

	VkRenderPassBeginInfo render_pass_info = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.pNext = NULL,
		.renderPass = data->render_pass,
		.framebuffer = data->swapchain_framebuffers[data->current_image_index],
		.renderArea = {
			.offset = {0, 0},
			.extent = data->swapchain_extent
		},
		.clearValueCount = 2,
		.pClearValues = clear_values
	};

	vkCmdBeginRenderPass(*command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(*command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
	data->triangle_graphics_pipeline);

	VkViewport viewport = {
		.x = 0.0f,
		.y = 0.0f,
		.width = (float)data->swapchain_extent.width,
		.height = (float)data->swapchain_extent.height,
		.minDepth = 0.0f,
		.maxDepth = 1.0f
	};
	vkCmdSetViewport(*command_buffer, 0, 1, &viewport);

	VkRect2D scissor = {
		.offset = {0, 0},
		.extent = data->swapchain_extent
	};
	vkCmdSetScissor(*command_buffer, 0, 1, &scissor);

	VkBuffer triangle_vertex_buffers[] = {data->triangle_main_vertex_buffer};
	VkDeviceSize offsets[] = {0};

	vkCmdBindVertexBuffers(*command_buffer, 0, 1, triangle_vertex_buffers, offsets);

	vkCmdBindIndexBuffer(*command_buffer, data->triangle_main_index_buffer, 0, VK_INDEX_TYPE_UINT32);

	vkCmdBindDescriptorSets(*command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, data->triangle_pipeline_layout, 0, 1, &data->descriptor_sets[data->current_frame], 0, NULL);

	vkCmdDrawIndexed(*command_buffer, triangle_index_count, 1, 0, 0, 0);

	vkCmdBindPipeline(*command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
	data->circle_graphics_pipeline);

	vkCmdSetViewport(*command_buffer, 0, 1, &viewport);

	vkCmdSetScissor(*command_buffer, 0, 1, &scissor);

	VkBuffer circle_vertex_buffers[] = {data->circle_main_vertex_buffer};

	vkCmdBindVertexBuffers(*command_buffer, 0, 1, circle_vertex_buffers, offsets);

	vkCmdBindIndexBuffer(*command_buffer, data->circle_main_index_buffer, 0, VK_INDEX_TYPE_UINT32);

	vkCmdBindDescriptorSets(*command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, data->circle_pipeline_layout, 0, 1, &data->descriptor_sets[data->current_frame], 0, NULL);

	vkCmdDrawIndexed(*command_buffer, circle_index_count, 1, 0, 0, 0);
      
	vkCmdEndRenderPass(*command_buffer);

	if (UNLIKELY(vkEndCommandBuffer(*command_buffer) != VK_SUCCESS)) {
		printf("failed to record command buffer! Aborting\n");
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

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		if (UNLIKELY(vkCreateSemaphore(data->device, &semaphore_create_info, NULL, &data->image_available_semaphores[i]) != VK_SUCCESS) ||
		    UNLIKELY(vkCreateSemaphore(data->device, &semaphore_create_info, NULL, &data->render_finished_semaphores[i]) != VK_SUCCESS) ||
		    UNLIKELY(vkCreateFence(data->device, &fence_info, NULL, &data->in_flight_fences[i]) != VK_SUCCESS)) {
			printf("failed to create synchronization objects! Aborting\n");
			return error_return;
		}
	}
	return success_return;
}

void engine_cleanup_swapchain(struct engine_data *data)
{
	vkDestroyImageView(data->device, data->depth_image_view, NULL);
	vkDestroyImage(data->device, data->depth_image, NULL);
	vkFreeMemory(data->device, data->depth_image_memory, NULL);
	for (int i = 0; i < data->swapchain_image_count; i++) {
		vkDestroyFramebuffer(data->device, data->swapchain_framebuffers[i], NULL);
	}
	for (int i = 0; i < data->swapchain_image_count; i++) {
		vkDestroyImageView(data->device, data->swapchain_image_views[i], NULL);
	}
	vkDestroySwapchainKHR(data->device, data->swapchain, NULL);
}

int engine_recreate_swapchain(struct engine_data *data)
{
	int width = 0, height = 0;
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(data->window, &width, &height);
            glfwWaitEvents();
        }

	vkDeviceWaitIdle(data->device);

	engine_cleanup_swapchain(data);

	engine_set_surface_format(data);
	engine_setting_swapchain_extent(data);

	engine_create_swapchain(data);

	vkGetSwapchainImagesKHR(data->device, data->swapchain, &data->swapchain_image_count, NULL);
	data->swapchain_images = malloc(sizeof(VkImage) * data->swapchain_image_count);
	vkGetSwapchainImagesKHR(data->device, data->swapchain, &data->swapchain_image_count, data->swapchain_images);

	data->swapchain_image_views = malloc(sizeof(VkImageView) * data->swapchain_image_count);
	data->swapchain_framebuffers = malloc(sizeof(VkFramebuffer) * data->swapchain_image_count);
	for(int i = 0; i < data->swapchain_image_count; i++) {
		if(engine_create_image_view(data, data->swapchain_images[i], data->surface_format.format, &data->swapchain_image_views[i]) != success_return) {
			printf("Failed to recreate Image Views! Aborting\n");
			return error_return;
		}
	}
	engine_create_depth_resources(data);
	engine_create_framebuffers(data);
	return success_return;
}

int engine_draw_frame(struct engine_data *data)
{	
	vkWaitForFences(data->device, 1, &data->in_flight_fences[data->current_frame], VK_TRUE, UINT64_MAX);

	VkResult result = vkAcquireNextImageKHR(data->device, data->swapchain, UINT64_MAX, data->image_available_semaphores[data->current_frame], VK_NULL_HANDLE, &data->current_image_index);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		engine_recreate_swapchain(data);
		data->uniform_buffer.height = data->swapchain_extent.height;
		data->uniform_buffer.width = data->swapchain_extent.width;
		memcpy(data->uniform_buffers_mapped[data->current_frame], &data->uniform_buffer, sizeof(data->uniform_buffer));

		return success_return;
	} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		printf("failed to acquire swap chain image! Aborting\n");
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

	if (UNLIKELY(vkQueueSubmit(data->graphics_queue, 1, &submit_info, data->in_flight_fences[data->current_frame]) != VK_SUCCESS)) {
		printf("failed to submit draw command buffer! Aborting\n");
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
		.pImageIndices = &data->current_image_index,
		.pResults = NULL
	};
	result = vkQueuePresentKHR(data->present_queue, &present_info);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebuffer_resized) {
		framebuffer_resized = 0;
		engine_recreate_swapchain(data);
		data->uniform_buffer.height = data->swapchain_extent.height;
		data->uniform_buffer.width = data->swapchain_extent.width;
		memcpy(data->uniform_buffers_mapped[data->current_frame], &data->uniform_buffer, sizeof(data->uniform_buffer));
	} else if (UNLIKELY(result != VK_SUCCESS)) {
		printf("failed to present swap chain image! Aborting\n");
		return error_return;
	}
	return success_return;
}

void engine_cleanup(struct engine_data *data) {
	engine_cleanup_swapchain(data);

	vkDestroyDescriptorSetLayout(data->device, data->descriptor_set_layout, NULL);		

	free(data->swapchain_images);
	free(data->swapchain_image_views);
	free(data->swapchain_framebuffers);

	vkDestroyBuffer(data->device, data->triangle_main_vertex_buffer, NULL);
	vkFreeMemory(data->device, data->triangle_main_vertex_buffer_memory, NULL);
	vkDestroyBuffer(data->device, data->triangle_main_index_buffer, NULL);
	vkFreeMemory(data->device, data->triangle_main_index_buffer_memory, NULL);

	vkDestroyBuffer(data->device, data->circle_main_vertex_buffer, NULL);
	vkFreeMemory(data->device, data->circle_main_vertex_buffer_memory, NULL);
	vkDestroyBuffer(data->device, data->circle_main_index_buffer, NULL);
	vkFreeMemory(data->device, data->circle_main_index_buffer_memory, NULL);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vkDestroyBuffer(data->device, data->uniform_buffers[i], NULL);
		vkFreeMemory(data->device, data->uniform_buffers_memory[i], NULL);
	}
	vkDestroyDescriptorPool(data->device, data->descriptor_pool, NULL);
	vkDestroyDescriptorSetLayout(data->device, data->descriptor_set_layout, NULL);

	vkDestroyFence(data->device, data->vertex_buffer_copy_fence, NULL);
	vkDestroyCommandPool(data->device, data->copy_vertex_buffer_command_pool, NULL);

	vkDestroyPipeline(data->device, data->triangle_graphics_pipeline, NULL);
	vkDestroyPipelineLayout(data->device, data->triangle_pipeline_layout, NULL);

	vkDestroyPipeline(data->device, data->circle_graphics_pipeline, NULL);
	vkDestroyPipelineLayout(data->device, data->circle_pipeline_layout, NULL);

	vkDestroyRenderPass(data->device, data->render_pass, NULL);

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vkDestroySemaphore(data->device, data->image_available_semaphores[i], NULL);
		vkDestroySemaphore(data->device, data->render_finished_semaphores[i], NULL);
		vkDestroyFence(data->device, data->in_flight_fences[i], NULL);
	}

	vkDestroyCommandPool(data->device, data->graphics_command_pool, NULL);

	vkDestroyDevice(data->device, NULL);

	vkDestroySurfaceKHR(data->instance, data->surface, NULL);

	vkDestroyInstance(data->instance, NULL);

	glfwDestroyWindow(data->window);
	glfwTerminate();
}

int main()
{
	struct engine_data data;
	memset(&data, 0, sizeof(struct engine_data));
	engine_init_window(&data);

	if (engine_create_vulkan_instance(&data) != success_return)
		return error_return;

	if (UNLIKELY(glfwCreateWindowSurface(data.instance, data.window, NULL, &data.surface) != VK_SUCCESS)) {
		printf("Failed to create window surface! Aborting\n");
		return error_return;
	}

	if (engine_pick_physical_device(&data) != success_return) {
		printf("Unable to pick device! Aborting\n");
		return error_return;
	}

	vkGetPhysicalDeviceFeatures(data.physical_device, &data.physical_device_features);
	vkGetPhysicalDeviceProperties(data.physical_device, &data.physical_device_properties);

	if (engine_find_queue_families(&data) != success_return) {
		printf("Failed to find appropriate queue families! Aborting\n");
		return error_return;
	}

	data.queue_families_count = data.queue_create_infos[0].queueFamilyIndex == data.queue_create_infos[1].queueFamilyIndex ? 1 : 2;

	if (engine_create_device(&data) != success_return)
		return error_return;

	vkGetDeviceQueue(data.device, data.queue_create_infos[0].queueFamilyIndex, 0, &data.graphics_queue);

	vkGetDeviceQueue(data.device, data.queue_create_infos[1].queueFamilyIndex, 0, &data.present_queue);

	engine_set_surface_format(&data);

	engine_setting_swapchain_extent(&data);

	if (engine_create_swapchain(&data) != success_return) {
		printf("Failed to create swap chain! Aborting\n");
		return error_return;
	}

	vkGetSwapchainImagesKHR(data.device, data.swapchain, &data.swapchain_image_count, NULL);
	data.swapchain_images = malloc(sizeof(VkImage) * data.swapchain_image_count);
	vkGetSwapchainImagesKHR(data.device, data.swapchain, &data.swapchain_image_count, data.swapchain_images);

	data.swapchain_image_views = malloc(sizeof(VkImageView) * data.swapchain_image_count);
	data.swapchain_framebuffers = malloc(sizeof(VkFramebuffer) * data.swapchain_image_count);

	for(int i = 0; i < data.swapchain_image_count; i++) {
		if(engine_create_image_view(&data, data.swapchain_images[i], data.surface_format.format, &data.swapchain_image_views[i]) != success_return) {
			printf("Failed to create Image Views! Aborting\n");
			return error_return;
		}
	}

	if (engine_create_render_pass(&data) != success_return) {
		printf("failed to create render pass! Aborting\n");
		return error_return;
	}

	if (engine_create_graphics_pipeline(&data, &data.triangle_graphics_pipeline,
	&data.triangle_pipeline_layout, "Engine/shaders/compiled/tr-vert.spv", "Engine/shaders/compiled/tr-frag.spv", 0)
	   != success_return)
		return error_return;

	if (engine_descriptor_set_layout(&data) != success_return)
		return error_return;

	if (engine_create_graphics_pipeline(&data, &data.circle_graphics_pipeline, &data.circle_pipeline_layout, "Engine/shaders/compiled/cr-vert.spv", "Engine/shaders/compiled/cr-frag.spv", 1)
	  != success_return)
		return error_return;

	if (engine_create_command_pool(&data, 0) != success_return)
		return error_return;

	if (engine_create_command_pool(&data, 1) != success_return)
		return error_return;

	if(engine_create_depth_resources(&data) != success_return)
		return error_return;

	if (engine_create_framebuffers(&data) != success_return)
		return error_return;

	VkFenceCreateInfo fence_info = {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0
	};

	vkCreateFence(data.device, &fence_info, NULL, &data.vertex_buffer_copy_fence);

	if(engine_create_vertex_buffers(&data) != success_return)
		return error_return;

	if(engine_create_index_buffers(&data) != success_return)
		return error_return;
	if (engine_create_uniform_buffers(&data) != success_return)
		return error_return;

	if (engine_create_command_buffers(&data))
		return error_return;

	if (engine_create_descriptor_pool(&data))
		return error_return;

	if (engine_create_descriptor_sets(&data))
		return error_return;

	engine_create_sync_objects(&data);
	while (!glfwWindowShouldClose(data.window)) {
		glfwPollEvents();
		if (engine_draw_frame(&data) != success_return) {
			return error_return;
		}
		data.current_frame = (data.current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
	}
	vkDeviceWaitIdle(data.device);

	engine_cleanup(&data);
}
