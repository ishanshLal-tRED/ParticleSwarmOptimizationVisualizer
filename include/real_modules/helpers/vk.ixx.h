#include <logging.hxx>

import <set>;
export import <span>;

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>;

import Helpers.GLFW;
export module Helpers.Vk;

namespace Helper{
	export {
		template<bool EnableValidationLayers = true>
		void createInstance (VkInstance&, VkDebugUtilsMessengerEXT&
			,VkApplicationInfo, const std::span<const char*>,
			VkBool32 (VKAPI_PTR *f)(
				VkDebugUtilsMessageSeverityFlagBitsEXT,VkDebugUtilsMessageTypeFlagsEXT,
			    const VkDebugUtilsMessengerCallbackDataEXT*,void*));

		void setupDebugMessenger (VkDebugUtilsMessengerEXT&
			,VkInstance, VkDebugUtilsMessengerCreateInfoEXT&,
			VkBool32 (VKAPI_PTR *f)(
				VkDebugUtilsMessageSeverityFlagBitsEXT, VkDebugUtilsMessageTypeFlagsEXT,
			    const VkDebugUtilsMessengerCallbackDataEXT*,void*));

		void createSurface (VkSurfaceKHR&
			,VkInstance, HWND, HINSTANCE hinstance = GetModuleHandle(nullptr));

		VkPhysicalDevice pickPhysicalDevice (VkInstance, bool (*f)(VkPhysicalDevice, void*), void*);

		template<bool EnableValidationLayers = true>
		void createLogicalDevice (VkDevice&
			,VkPhysicalDevice, std::vector<std::pair<VkQueue*, uint32_t>>
			,const std::span<const char*>, const std::span<const char*>);

		void createSwapChain (VkSwapchainKHR&, std::vector<VkImage>&
			,VkSurfaceKHR, VkDevice, VkSurfaceCapabilitiesKHR, VkExtent2D
			,VkSurfaceFormatKHR, VkPresentModeKHR, uint32_t, uint32_t);

		void createImageViews (std::vector<VkImageView>&
			, VkDevice, VkFormat, std::vector<VkImage>&);
		
		VkShaderModule createShaderModule (VkDevice device, const std::vector<char>& byte_code);
	
		template<int num_shader_stages> // full size means
		void createGraphicsPipelineDefaultSize (VkPipelineLayout&, VkPipeline&
			,VkDevice, VkRenderPass
			,VkPipelineLayoutCreateInfo, const VkPipelineShaderStageCreateInfo[]
			,VkPipelineVertexInputStateCreateInfo, VkPipelineInputAssemblyStateCreateInfo
			,VkExtent2D, const std::span<VkDynamicState>);
		
		void createRenderPasses (VkRenderPass&
			,VkDevice, VkAttachmentDescription, const std::span<VkSubpassDescription>);

		void createFramebuffers (std::vector<VkFramebuffer>&
			,VkDevice, VkRenderPass, const std::span<VkImageView>
			,VkExtent2D);
		
		void createCommandPoolAndBuffer (VkCommandPool&, std::span<VkCommandBuffer>
			,VkDevice, uint32_t);

		uint32_t findMemoryType (VkPhysicalDevice, uint32_t, VkMemoryPropertyFlags);

		void createBuffer (VkBuffer&, VkDeviceMemory&
			,VkDevice, VkPhysicalDevice, VkDeviceSize, VkBufferUsageFlags, VkMemoryPropertyFlags);

		void copyBuffer (VkBuffer&
			,VkBuffer, VkDeviceSize, VkDevice, VkCommandPool, VkQueue);

		void copyBuffer (VkImage&
			,VkBuffer, VkExtent2D
			,VkDevice, VkCommandPool, VkQueue);
		
		void createImage (VkImage&, VkDeviceMemory&
			,VkDevice, VkPhysicalDevice, VkExtent2D
			,VkFormat, VkImageTiling, VkImageUsageFlags
			,VkMemoryPropertyFlags);
	
		VkCommandBuffer beginSingleTimeCommands (VkDevice, VkCommandPool);

		void endSingleTimeCommands (VkCommandBuffer&
			,VkDevice, VkCommandPool, VkQueue);

		void transitionImageLayout (VkDevice, VkCommandPool, VkQueue
			,VkImage, VkFormat, VkImageLayout, VkImageLayout);

		std::string physicalDeviceTypeString(VkPhysicalDeviceType type);
	};

    template<bool EnableValidationLayers>
	void createInstance (VkInstance& instance, VkDebugUtilsMessengerEXT& debug_messenger
		,VkApplicationInfo app_info, std::span<const char*> req_validation_layers
		,VkBool32 (VKAPI_PTR *vk_debug_callback)(
			VkDebugUtilsMessageSeverityFlagBitsEXT, VkDebugUtilsMessageTypeFlagsEXT,
		    const VkDebugUtilsMessengerCallbackDataEXT*,void*) ) 
	{
        std::vector<const char*> vk_extensions_to_enable;
        auto glfw_extensions = Helper::GetGLFWRequiredVkExtensions();
        vk_extensions_to_enable.insert(vk_extensions_to_enable.end(), glfw_extensions.begin(), glfw_extensions.end());
	    if (EnableValidationLayers) {
	    	vk_extensions_to_enable.push_back (VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	    }
        
        { // Vk Extensions
			uint32_t extension_count = 0;
			vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
			std::vector<VkExtensionProperties> extensions(extension_count);
			vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extensions.data ());
			
			//LOG_trace("available extensions:");
			//for (const auto& extension : extensions)
			//	LOG_raw("\n\t {}", extension.extensionName);
			//
			//LOG_trace("{} extensions supported", extension_count);
		}
	
		{ // Check validation layer support
			uint32_t layer_count;
			vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
			std::vector<VkLayerProperties> validation_layers(layer_count);
			vkEnumerateInstanceLayerProperties(&layer_count, validation_layers.data ());

			std::vector<const char*> unavailable_layers;
			for (const char* layer_name : req_validation_layers) {
			    bool layerFound = false;
			
			    for (const auto& layer_properties : validation_layers) {
			        if (strcmp(layer_name, layer_properties.layerName) == 0) {
			            layerFound = true;
			            break;
			        }
			    }
			    if (!layerFound) {
			        unavailable_layers.push_back (layer_name);
			    }
			}
			if (!unavailable_layers.empty ()) {
				LOG_error("Missing Layer(s):");
				for (const char* extension_name: unavailable_layers)
					LOG_raw("\n\t {}", extension_name);
				THROW_CORE_Critical("{:d} layer(s) not found", unavailable_layers.size ());
			}
		}
		
		VkDebugUtilsMessengerCreateInfoEXT debug_messenger_create_info { // Reusing later when creating debug messenger
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
			.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
			.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
			.pfnUserCallback = vk_debug_callback,
			.pUserData = nullptr // Optional
		};

		VkInstanceCreateInfo create_info{
			.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			//.pNext = (EnableValidationLayers ? (VkDebugUtilsMessengerCreateInfoEXT*) &debug_messenger_create_info : nullptr),
			//// Disabling due to non fatal error, Refer: http://disq.us/p/2o823o8
			.pApplicationInfo = &app_info,
			.enabledLayerCount = uint32_t(EnableValidationLayers ? req_validation_layers.size () : 0),
			.ppEnabledLayerNames = (EnableValidationLayers ? req_validation_layers.data () : nullptr),
			.enabledExtensionCount = uint32_t(vk_extensions_to_enable.size()),
			.ppEnabledExtensionNames = vk_extensions_to_enable.data()
		};

		if (vkCreateInstance(&create_info, nullptr, &instance) != VK_SUCCESS) 
			THROW_CORE_Critical("failed to create instance!");

		if (EnableValidationLayers) { // Create Debug Utils Messenger
			setupDebugMessenger (debug_messenger, instance, debug_messenger_create_info, vk_debug_callback);
		}
    }

    void setupDebugMessenger (VkDebugUtilsMessengerEXT& debug_messenger
		,VkInstance instance, VkDebugUtilsMessengerCreateInfoEXT &debug_messenger_info
		,VkBool32 (VKAPI_PTR *vk_debug_callback)(
			VkDebugUtilsMessageSeverityFlagBitsEXT, VkDebugUtilsMessageTypeFlagsEXT,
		    const VkDebugUtilsMessengerCallbackDataEXT*,void*))
	{
		VkResult result = VK_ERROR_EXTENSION_NOT_PRESENT;
		auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
		if (func != nullptr) {
		    result = func(instance, &debug_messenger_info, nullptr, &debug_messenger);
		};
		if (result != VK_SUCCESS)
			THROW_CORE_Critical("failed setting up vk_debug_messenger");
	}

    void createSurface(VkSurfaceKHR& surface
		,VkInstance instance, HWND window_handle, HINSTANCE hinstance) 
	{
		VkWin32SurfaceCreateInfoKHR surface_info{
			.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
			.hinstance = hinstance,
			.hwnd = window_handle
		};

		if (vkCreateWin32SurfaceKHR (instance, &surface_info, nullptr, &surface) != VK_SUCCESS) 
			THROW_CORE_Critical("failed to create window surface");
	}

    VkPhysicalDevice pickPhysicalDevice(VkInstance instance	
		,bool (*is_device_suitable)(VkPhysicalDevice, void*), void* user_supplied_data) 
	{
		uint32_t device_count = 0;
		vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
		if (device_count == 0)
			THROW_CORE_Critical("failed to find hardware with vulkan support");
		std::vector<VkPhysicalDevice> vk_enabled_devices (device_count);
		vkEnumeratePhysicalDevices(instance, &device_count, vk_enabled_devices.data ());

		// Can be improved, aka when more than one GPU is available
		for (auto device: vk_enabled_devices){
			if (is_device_suitable (device, user_supplied_data)) {
				return device;
			}
		}
		
		THROW_CORE_Critical("failed to find hardware with vulkan support");
	};
	
	template<bool EnableValidationLayers>
    void createLogicalDevice(VkDevice& logical_device
		,VkPhysicalDevice physical_device, std::vector<std::pair<VkQueue*, uint32_t>> vk_queues_and_family_indice_pairs
		,std::span<const char*> req_validation_layers, std::span<const char*> req_extensions) 
	{
		std::set<uint32_t> unique_queue_families;
		for (auto& queue_family_pair: vk_queues_and_family_indice_pairs) {
			unique_queue_families.insert(queue_family_pair.second);
		}

		std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
		queue_create_infos.reserve (unique_queue_families.size());
		for (auto family_indice: unique_queue_families){
			float queue_priority = 1.0f; // need more information on it
			queue_create_infos.push_back(VkDeviceQueueCreateInfo {
				.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
				.queueFamilyIndex = family_indice,
				.queueCount = 1,
				.pQueuePriorities = &queue_priority
			});
		}

		VkPhysicalDeviceFeatures device_features {.samplerAnisotropy = VK_TRUE};
		vkGetPhysicalDeviceFeatures (physical_device, &device_features);
		
		VkDeviceCreateInfo create_info{
			.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			.queueCreateInfoCount = uint32_t(queue_create_infos.size ()),
			.pQueueCreateInfos = queue_create_infos.data (),
			.enabledLayerCount = uint32_t(EnableValidationLayers ? req_validation_layers.size () : 0),
			.ppEnabledLayerNames = (EnableValidationLayers ? req_validation_layers.data () : nullptr),
			.enabledExtensionCount = uint32_t(req_extensions.size ()),
			.ppEnabledExtensionNames = req_extensions.data (),
			.pEnabledFeatures = &device_features
		};

		if (vkCreateDevice (physical_device, &create_info, nullptr, &logical_device) != VK_SUCCESS) 
			THROW_CORE_Critical ("Failed to vreate logical device");

		for (auto& queue_family_pair: vk_queues_and_family_indice_pairs) {
			vkGetDeviceQueue (logical_device, queue_family_pair.second, 0, queue_family_pair.first);
		}
	}

    void createSwapChain (VkSwapchainKHR& swapchain, std::vector<VkImage> &swapchain_images
		,VkSurfaceKHR surface, VkDevice logical_device
		,VkSurfaceCapabilitiesKHR swapchain_capabilities
		,VkExtent2D extent, VkSurfaceFormatKHR format, VkPresentModeKHR present_mode
		,uint32_t graphics_family_indice, uint32_t presentation_family_indice)
	{
		uint32_t image_count = ((swapchain_capabilities.maxImageCount == 0) ? swapchain_capabilities.minImageCount + 1 : swapchain_capabilities.maxImageCount);
		VkSwapchainCreateInfoKHR swapchain_create_info {
			.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
			.surface = surface,
			.minImageCount = image_count,
			.imageFormat = format.format,
			.imageColorSpace = format.colorSpace,
			.imageExtent = extent,
			.imageArrayLayers = 1,
			.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,

			.preTransform = swapchain_capabilities.currentTransform,
			.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
			.presentMode = present_mode,
			.clipped = VK_TRUE,
			.oldSwapchain = VK_NULL_HANDLE
		};
		
		if (graphics_family_indice == presentation_family_indice) {
			swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			swapchain_create_info.queueFamilyIndexCount = 0;
			swapchain_create_info.pQueueFamilyIndices = nullptr;
		} else {
			uint32_t queue_family_indices[] = {graphics_family_indice, presentation_family_indice};
			
			swapchain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			swapchain_create_info.queueFamilyIndexCount = uint32_t(std::size(queue_family_indices));
			swapchain_create_info.pQueueFamilyIndices = queue_family_indices;
		}
	
		if (vkCreateSwapchainKHR (logical_device, &swapchain_create_info, nullptr, &swapchain) != VK_SUCCESS)
			THROW_CORE_Critical("failed to create swapchain !");

		uint32_t swapchain_images_count;
		vkGetSwapchainImagesKHR(logical_device, swapchain, &swapchain_images_count, nullptr);
		swapchain_images.resize(swapchain_images_count);
		vkGetSwapchainImagesKHR(logical_device, swapchain, &swapchain_images_count, swapchain_images.data());
	}

    void createImageViews(std::vector<VkImageView> &swapchain_images_view
		,VkDevice logical_device, VkFormat swapchain_image_format
		,std::vector<VkImage> &swapchain_images) 
	{
		swapchain_images_view.resize(swapchain_images.size());
		for (size_t i = 0; i < swapchain_images.size (); ++i) {
			VkImageViewCreateInfo create_info {
				.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				.image = swapchain_images[i],
				.viewType = VK_IMAGE_VIEW_TYPE_2D,
				.format = swapchain_image_format,
				.components = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
				.subresourceRange = VkImageSubresourceRange {
					    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					    .baseMipLevel = 0,
					    .levelCount = 1,
					    .baseArrayLayer = 0,
					    .layerCount = 1
				}
			};

			if (vkCreateImageView (logical_device, &create_info, nullptr, &swapchain_images_view[i]) != VK_SUCCESS)
				THROW_Critical ("failed to create imagfe views!");
		}
	}

	VkShaderModule createShaderModule (VkDevice device, const std::vector<char>& byte_code) {
		VkShaderModuleCreateInfo create_info{
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.codeSize = byte_code.size(),
			.pCode = (const uint32_t*)(byte_code.data()) // should be aligned as (uint32_t);
		};

		VkShaderModule shader_module;
		if (vkCreateShaderModule(device, &create_info, nullptr, &shader_module) != VK_SUCCESS) 
			THROW_Critical ("failed to create shader module!");
		return shader_module;
	}

	void createRenderPasses (VkRenderPass& render_pass
		,VkDevice device, VkAttachmentDescription color_attachment
		,const std::span<VkSubpassDescription> subpasses) 
	{
		VkSubpassDependency dependency {
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.srcAccessMask = 0,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
		};
		
		VkRenderPassCreateInfo render_pass_info {
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			.attachmentCount = 1,
			.pAttachments = &color_attachment,
			.subpassCount = uint32_t(subpasses.size ()),
			.pSubpasses = subpasses.data (),
			.dependencyCount = 1,
			.pDependencies = &dependency
		};
	
		if (vkCreateRenderPass(device, &render_pass_info, nullptr, &render_pass) != VK_SUCCESS)
			THROW_CORE_Critical ("failed to create render pass!");		
	}
	
	template<int num_shader_stages>
	void createGraphicsPipelineDefaultSize (VkPipelineLayout& pipeline_layout, VkPipeline& graphics_pipeline
		,VkDevice device, VkRenderPass render_pass
		,VkPipelineLayoutCreateInfo pipeline_layout_info, const VkPipelineShaderStageCreateInfo shader_stages[]
		,VkPipelineVertexInputStateCreateInfo vertex_input_info, VkPipelineInputAssemblyStateCreateInfo input_assembly_info
		,VkExtent2D swapchain_extent, const std::span<VkDynamicState> dynamic_states)
	{
		VkViewport viewport{
			.x = 0.0f,
			.y = 0.0f,
			.width  = float (swapchain_extent.width),
			.height = float (swapchain_extent.height),
			.minDepth = 0.0f,
			.maxDepth = 1.0f
		};
		VkRect2D scissor{
			.offset = {0, 0},
			.extent = swapchain_extent
		};
		VkPipelineViewportStateCreateInfo viewport_state{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			.viewportCount = 1,
			.pViewports = &viewport,
			.scissorCount = 1,
			.pScissors = &scissor
		};
		constexpr bool enable_depth_clamping = false; // clamps fragments beyond near and far planes // GPU specific feature
		VkPipelineRasterizationStateCreateInfo rasterizer {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
			.depthClampEnable = enable_depth_clamping ? VK_TRUE : VK_FALSE,
			.rasterizerDiscardEnable = VK_FALSE,
			.polygonMode = VK_POLYGON_MODE_FILL,
			.cullMode = VK_CULL_MODE_NONE,
			.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
			.depthBiasEnable = VK_FALSE,
			.depthBiasConstantFactor = 0.0f, // Optional
			.depthBiasClamp = 0.0f, // Optional
			.depthBiasSlopeFactor = 0.0f, // Optional

			.lineWidth = 1.0f
		};
		VkPipelineMultisampleStateCreateInfo multisampling {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
			.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
			.sampleShadingEnable = VK_FALSE,
			.minSampleShading = 1.0f, // Optional
			.pSampleMask = nullptr, // Optional
			.alphaToCoverageEnable = VK_FALSE, // Optional
			.alphaToOneEnable = VK_FALSE // Optional
		};
		VkPipelineColorBlendAttachmentState color_blend_attachment {
			.blendEnable = VK_TRUE,

			.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA, // Optional
			.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, // Optional
			.colorBlendOp = VK_BLEND_OP_ADD, // Optional
			.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA, // Optional
			.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, // Optional
			.alphaBlendOp = VK_BLEND_OP_ADD, // Optional

			.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
		};
		VkPipelineColorBlendStateCreateInfo color_blending {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
			.logicOpEnable = VK_FALSE,
			.logicOp = VK_LOGIC_OP_COPY, // Optional
			.attachmentCount = 1,
			.pAttachments = &color_blend_attachment,
			.blendConstants = {0.0f, 0.0f, 0.0f, 0.0f} // Optional
		};

		if (vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr, &pipeline_layout) != VK_SUCCESS) 
			{THROW_CORE_Critical ("failed to create pipeline layout")};

		VkGraphicsPipelineCreateInfo pipeline_info {
			.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.stageCount = num_shader_stages,  
			.pStages = shader_stages,  
			.pVertexInputState = &vertex_input_info,  
			.pInputAssemblyState = &input_assembly_info, 
			.pViewportState = &viewport_state,  
			.pRasterizationState = &rasterizer,  
			.pMultisampleState = &multisampling,  
			.pDepthStencilState = nullptr, // Optional  
			.pColorBlendState = &color_blending,
			.pDynamicState = nullptr, // Optional
			.layout = pipeline_layout,
			.renderPass = render_pass,
			.subpass = 0,
			.basePipelineHandle = VK_NULL_HANDLE, // Optional
			.basePipelineIndex = -1 // Optional
		};

		VkPipelineDynamicStateCreateInfo dynamic_state;
		if (!dynamic_states.empty ()) {// States that can be tweaked after pipeline creation // will be required to be specified at draw time
			dynamic_state = VkPipelineDynamicStateCreateInfo {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
				.dynamicStateCount = uint32_t(dynamic_states.size()),
				.pDynamicStates = dynamic_states.data(),
			};
			// Add dynamic states
			pipeline_info.pDynamicState = &dynamic_state;
		}

		if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &graphics_pipeline) != VK_SUCCESS)
		    THROW_CORE_Critical ("failed to create graphics pipeline!");
	}

	void createFramebuffers (std::vector<VkFramebuffer>& framebuffers
		,VkDevice device, VkRenderPass render_pass, const std::span<VkImageView> images_view
		,VkExtent2D extent)
	{
		framebuffers.resize (images_view.size ());
		for (uint32_t i = 0; i < images_view.size (); ++i) {
			VkImageView attachment[] = { images_view[i] };
			
			VkFramebufferCreateInfo framebuffer_info {
				.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
				.renderPass = render_pass,
				.attachmentCount = 1,
				.pAttachments = attachment,
				.width  = extent.width,
				.height = extent.height,
				.layers = 1
			};
			
			if (vkCreateFramebuffer(device, &framebuffer_info, nullptr, &framebuffers[i]) != VK_SUCCESS)
			    THROW_CORE_Critical("failed to create framebuffer!");
			
		}
	}

	void createCommandPoolAndBuffer (VkCommandPool& command_pool, std::span<VkCommandBuffer> command_buffers
		,VkDevice device, uint32_t queue_family_index)
	{
		// Create command pool
		VkCommandPoolCreateInfo pool_info {
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
			.queueFamilyIndex = queue_family_index
		};

		if (vkCreateCommandPool (device, &pool_info, nullptr, &command_pool) != VK_SUCCESS) 
			THROW_CORE_Critical ("failed to create command pool!");

		// Create command buffer
		VkCommandBufferAllocateInfo alloc_info {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = command_pool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = uint32_t (command_buffers.size ())
		};
		
		if (vkAllocateCommandBuffers(device, &alloc_info, command_buffers.data ()) != VK_SUCCESS) {
		    THROW_CORE_Critical ("failed to allocate command buffers!");
		}
	}

	uint32_t findMemoryType (VkPhysicalDevice physical_device, uint32_t type_filter
		,VkMemoryPropertyFlags properties) 
	{
		VkPhysicalDeviceMemoryProperties mem_properties;
		vkGetPhysicalDeviceMemoryProperties (physical_device, &mem_properties);
		
		for (uint32_t i = 0; i < mem_properties.memoryTypeCount; ++i)
			if ( (type_filter & (1 << i)) && 
				((mem_properties.memoryTypes[i].propertyFlags & properties) == properties) )
				return i;
		
		THROW_Critical ("failed to find suitable memory type");
	}

	void createBuffer (VkBuffer& buffer, VkDeviceMemory& buffer_memory
		,VkDevice device, VkPhysicalDevice physical_device, VkDeviceSize size
		,VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
	{
		VkBufferCreateInfo buffer_info {
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = size,
			.usage = usage,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE
		};
	
	    if (vkCreateBuffer(device, &buffer_info, nullptr, &buffer) != VK_SUCCESS) {
	        throw std::runtime_error("failed to create buffer!");
	    }
	
	    VkMemoryRequirements mem_requirements;
	    vkGetBufferMemoryRequirements(device, buffer, &mem_requirements);
	
	    VkMemoryAllocateInfo alloc_info {
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.allocationSize = mem_requirements.size,
			.memoryTypeIndex = findMemoryType(physical_device, mem_requirements.memoryTypeBits, properties)
		};
	
	    if (vkAllocateMemory(device, &alloc_info, nullptr, &buffer_memory) != VK_SUCCESS) {
	        throw std::runtime_error("failed to allocate buffer memory!");
	    }
	
	    vkBindBufferMemory(device, buffer, buffer_memory, 0);
	};

	void copyBuffer (VkBuffer& dst_buffer
		,VkBuffer src_buffer, VkDeviceSize buffer_size
		,VkDevice device, VkCommandPool command_pool, VkQueue graphics_queue)
	{
		
	    VkCommandBuffer command_buffer = beginSingleTimeCommands (device, command_pool);
		
		VkBufferCopy copy_region{
			.srcOffset = 0, // Optional
			.dstOffset = 0, // Optional
			.size = buffer_size
		};
		vkCmdCopyBuffer (command_buffer, src_buffer, dst_buffer, 1, &copy_region);

		endSingleTimeCommands (command_buffer, device, command_pool, graphics_queue);
	}
	
	void createImage (VkImage& image, VkDeviceMemory& image_memory
		,VkDevice device, VkPhysicalDevice physical_device, VkExtent2D size
		,VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage
		,VkMemoryPropertyFlags properties)
	{
		VkImageCreateInfo image_info {
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.imageType = VK_IMAGE_TYPE_2D,
			.format = format,
			.extent = VkExtent3D {.width = size.width, .height = size.height, .depth = 1},
			.mipLevels = 1,
			.arrayLayers = 1,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.tiling = tiling,
			.usage = usage,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
		};
	
	    if (vkCreateImage (device, &image_info, nullptr, &image) != VK_SUCCESS)
	        THROW_Critical ("failed to create image!");
	
	    VkMemoryRequirements mem_requirements;
	    vkGetImageMemoryRequirements (device, image, &mem_requirements);
	
	    VkMemoryAllocateInfo alloc_info {
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.allocationSize = mem_requirements.size,
			.memoryTypeIndex = findMemoryType (physical_device, mem_requirements.memoryTypeBits, properties)
		};
	
	    if (vkAllocateMemory (device, &alloc_info, nullptr, &image_memory) != VK_SUCCESS)
	        THROW_Critical ("failed to allocate buffer memory!");
	
	    vkBindImageMemory (device, image, image_memory, 0);
	};
		
	VkCommandBuffer beginSingleTimeCommands (VkDevice logical_device
		,VkCommandPool command_pool) 
	{
		VkCommandBufferAllocateInfo alloc_info {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = command_pool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = 1
		};

		VkCommandBuffer command_buffer;
		vkAllocateCommandBuffers (logical_device, &alloc_info, &command_buffer);

		VkCommandBufferBeginInfo begin_info{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
		};

		vkBeginCommandBuffer (command_buffer, &begin_info);

		return command_buffer;
	}
		
	void endSingleTimeCommands (VkCommandBuffer& command_buffer
		,VkDevice logical_device, VkCommandPool command_pool, VkQueue queue)
	{
		vkEndCommandBuffer (command_buffer);

		VkSubmitInfo submit_info {
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.commandBufferCount = 1,
			.pCommandBuffers = &command_buffer
		};
		vkQueueSubmit (queue, 1, &submit_info, VK_NULL_HANDLE);

		vkQueueWaitIdle (queue);

		vkFreeCommandBuffers (logical_device, command_pool, 1, &command_buffer);
	}
	
	void transitionImageLayout (VkDevice logical_device, VkCommandPool command_pool, VkQueue queue
		,VkImage image, VkFormat /*format*/, VkImageLayout old_layout, VkImageLayout new_layout) 
	{
		auto command_buffer = beginSingleTimeCommands (logical_device, command_pool);

		VkImageMemoryBarrier barrier {
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,

			.srcAccessMask = 0, // Later
			.dstAccessMask = 0, // Later

			.oldLayout = old_layout,
			.newLayout = new_layout,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = image,
			.subresourceRange = VkImageSubresourceRange {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1},
		};

		VkPipelineStageFlags source_stage, destination_stage;
		if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		    barrier.srcAccessMask = 0;
		    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		
		    source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		    destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		} else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		
		    source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		    destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		} else 
		    THROW_Critical ("unsupported layout transition!");
		

		vkCmdPipelineBarrier (command_buffer
			,source_stage, destination_stage
			,0
			,0, nullptr
			,0, nullptr
			,1, &barrier);

		endSingleTimeCommands (command_buffer, logical_device, command_pool, queue);
	}

	void copyBuffer (VkImage& dst_image
		,VkBuffer src_buffer, VkExtent2D size
		,VkDevice device, VkCommandPool command_pool, VkQueue graphics_queue)
	{
		VkBufferImageCopy copy_region{
			.bufferOffset = 0,
			.bufferRowLength = 0,
			.bufferImageHeight = 0,

			.imageSubresource = VkImageSubresourceLayers{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .mipLevel = 0, .baseArrayLayer = 0, .layerCount = 1},
			
			.imageOffset = {0,0,0},
			.imageExtent = {size.width, size.height, 1}
		};

		transitionImageLayout (device, command_pool, graphics_queue
			,dst_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	    VkCommandBuffer command_buffer = beginSingleTimeCommands (device, command_pool);
		vkCmdCopyBufferToImage (command_buffer, src_buffer, dst_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);
		endSingleTimeCommands (command_buffer, device, command_pool, graphics_queue);

		transitionImageLayout (device, command_pool, graphics_queue
			,dst_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	}
	std::string physicalDeviceTypeString(VkPhysicalDeviceType type) {
		switch (type)
		{
#define STR(r) case VK_PHYSICAL_DEVICE_TYPE_ ##r: return #r
			STR(OTHER);
			STR(INTEGRATED_GPU);
			STR(DISCRETE_GPU);
			STR(VIRTUAL_GPU);
			STR(CPU);
#undef STR
		default: return "UNKNOWN_DEVICE_TYPE";
		}
	};
};