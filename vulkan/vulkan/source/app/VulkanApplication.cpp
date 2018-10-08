//-----------------------------------------------------------------------------
#include "app/VulkanApplication.h"
#include <stdexcept>
#include <iostream>
#include <vector>
//-----------------------------------------------------------------------------
VulkanApplication::VulkanApplication()
{
}
//-----------------------------------------------------------------------------
VulkanApplication::~VulkanApplication()
{
}
//-----------------------------------------------------------------------------
void VulkanApplication::Start()
{
	InitWindow();
	InitVulkan();
}
//-----------------------------------------------------------------------------
void VulkanApplication::Loop()
{
	while (!glfwWindowShouldClose(Window))
	{
		glfwPollEvents();
		DrawFrame();
	}

	vkDeviceWaitIdle(VKDevice);
}
//-----------------------------------------------------------------------------
void VulkanApplication::Cleanup() const
{
	CleanupSwapChain();

	vkDestroyDescriptorSetLayout(VKDevice, VKDescriptorSetLayout, nullptr);

	for (size_t i = 0; i < VKSwapChainImages.size(); i++)
	{
		vkDestroyBuffer(VKDevice, VKUniformBuffers[i], nullptr);
		vkFreeMemory(VKDevice, VKUniformBuffersMemory[i], nullptr);
	}
	vkDestroyBuffer(VKDevice, VKIndexBuffer, nullptr);
	vkFreeMemory(VKDevice, VKIndexBufferMemory, nullptr);

	vkDestroyBuffer(VKDevice, VKVertexBuffer, nullptr);
	vkFreeMemory(VKDevice, VKVertexBufferMemory, nullptr);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkDestroySemaphore(VKDevice, VKRenderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(VKDevice, VKImageAvailableSemaphores[i], nullptr);
		vkDestroyFence(VKDevice, VKInFlightFences[i], nullptr);
	}

	// GH : VK Cleanup
	vkDestroyCommandPool(VKDevice, VKCommandPool, nullptr);
	vkDestroyDevice(VKDevice, nullptr);
	if (EnableValidationLayers)
	{
		DestroyDebugUtilsMessengerEXT(VKInstance, callback, nullptr);
	}
	
	vkDestroySurfaceKHR(VKInstance, VKSurface, nullptr);
	vkDestroyInstance(VKInstance, nullptr);

	// GH : GLFW cleanup
	glfwDestroyWindow(Window); 
	glfwTerminate();
}
//-----------------------------------------------------------------------------
void VulkanApplication::InitWindow() 
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	Window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
	glfwSetWindowUserPointer(Window, this);
	glfwSetFramebufferSizeCallback(Window, FramebufferResizeCallback);
}
//-----------------------------------------------------------------------------
void VulkanApplication::InitVulkan() 
{
	CreateInstance();
	SetupDebugCallback();
	//CreateVulkanSurface();
	CreateSurface();
	PickPhysicalDevice();
	CreateLogicalDevice();
	CreateSwapChain();
	CreateImageViews();
	CreateRenderPass();
	CreateDescriptorSetLayout();
	CreateGraphicsPipeline();
	CreateFramebuffers();
	CreateCommandPool();
	CreateVertexBuffer();
	CreateIndexBuffer();
	CreateUniformBuffer();
	CreateCommandBuffers();
	CreateSemaphores();
}
//-----------------------------------------------------------------------------
const bool VulkanApplication::CheckValidationLayerSupport() const
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char * layerName : ValidationLayers)
	{
		bool layerFound = false;
		for (const auto& layerProperties : availableLayers)
		{
			if (strcmp(layerName, layerProperties.layerName))
			{
				layerFound = true;
				break;
			}
		}

		if (!layerFound)
		{
			return false;
		}
	}
	return true;
}
//-----------------------------------------------------------------------------
const bool VulkanApplication::CheckDeviceExtensionSupport(const VkPhysicalDevice& device) const
{
	uint32_t extensionCount;
	// Get how many properties are we adding
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	// Properly fill the extensions now that we have a number
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
	std::set<std::string> requiredExtensions(DeviceExtensions.begin(), DeviceExtensions.end());

 	for (const auto & extension : availableExtensions)
	{
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}
//-----------------------------------------------------------------------------
void VulkanApplication::CreateInstance() const
{
	//  Poll against vulkan if we can validate any of the required layers
	if (EnableValidationLayers && !CheckValidationLayerSupport())
	{
		throw std::runtime_error("validation layers requested, but not available");
	}

	// GH: Setup struct for application info
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Hello Triangle";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	// GH: Create the required info
	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	// Get enabled extensions
	auto extensions = GetRequiredExtensions();
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	if (EnableValidationLayers)
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(ValidationLayers.size());
		createInfo.ppEnabledLayerNames = ValidationLayers.data();
	}
	else
	{
		createInfo.enabledLayerCount = 0;
	}
	// Create and setup all the data for creating the instance
	if (vkCreateInstance(&createInfo, nullptr, &VKInstance) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create vulkan instance!");
	}
}
//-----------------------------------------------------------------------------
void VulkanApplication::PickPhysicalDevice()
{
	VKPhysicalDevice = VK_NULL_HANDLE;
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(VKInstance, &deviceCount, nullptr);

	if (deviceCount == 0)
	{
		throw std::runtime_error("failed to find GPUs with Vulkan Support");
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(VKInstance, &deviceCount, devices.data());

	for (const auto& device : devices)
	{
		if (IsDeviceSuitable(device))
		{
			VKPhysicalDevice = device;
			break;
		}
	}

	if (VKPhysicalDevice == VK_NULL_HANDLE)
	{
		throw std::runtime_error("failed to find a suitable GPU!");
	}

	//// Get all the available physical devices and order them by score
	//std::multimap<int, VkPhysicalDevice> candidates;

	//for (const auto& device : devices)
	//{
	//	int32_t score = RateDeviceSuitability(device);
	//	candidates.insert(std::make_pair(score, device));
	//}

	//if (candidates.rbegin()->first > 0)
	//{
	//	VKPhysicalDevice = candidates.rbegin()->second;
	//}
	//else
	//{
	//	throw std::runtime_error("failed to find a suitable GPU");
	//}
}
//-----------------------------------------------------------------------------
void VulkanApplication::ListVulkanExtensions() const
{
	// GH: Enumerate the extensions from vk for extra information
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

	std::cout << "AVAILABLE VK EXTENSIONS: " << std::endl;
	for (const auto& extension : extensions)
	{
		std::cout << "\t" << extension.extensionName << std::endl;
	}
}
//-----------------------------------------------------------------------------
const std::vector<const char*> VulkanApplication::GetRequiredExtensions() const
{
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	if (EnableValidationLayers)
	{
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return extensions;
}
//-----------------------------------------------------------------------------
const SwapChainSupportDetails VulkanApplication::QuerySwapChainSupport(const VkPhysicalDevice& device)
{
	SwapChainSupportDetails details;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, VKSurface, &details.Capabilities);
	
	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, VKSurface, &formatCount, nullptr);

	if (formatCount != 0) 
	{
		details.Formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, VKSurface, &formatCount, details.Formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, VKSurface, &presentModeCount, nullptr);

	if (presentModeCount != 0) 
	{
		details.PresentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, VKSurface, &presentModeCount, details.PresentModes.data());
	}
	return details;
}
//-----------------------------------------------------------------------------
const VkSurfaceFormatKHR VulkanApplication::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED)
	{
		return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	}
	for (const auto& availableFormat : availableFormats) 
	{
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) 
		{
			return availableFormat;
		}
	}

	return availableFormats[0];

}
//-----------------------------------------------------------------------------
const VkPresentModeKHR VulkanApplication::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
	VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;
	for(const auto& availablePresentMode : availablePresentModes)
	{
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			return availablePresentMode;
		}
		else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
		{
			bestMode = availablePresentMode;
		}
	}
	return bestMode;
}
//-----------------------------------------------------------------------------
const VkExtent2D VulkanApplication::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max() )
	{
		return capabilities.currentExtent;
	}
	else
	{

		int width, height;
		glfwGetFramebufferSize(Window, &width, &height);
		VkExtent2D actualExtent = { width, height };
		actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

		return actualExtent;
	}
}
//-----------------------------------------------------------------------------

const QueueFamilyIndices VulkanApplication::FindQueueFamilies(const VkPhysicalDevice& device)
{
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	// TO-DO What does this function do and why does it needs a nullptr
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	int32_t i = 0;
	for (const auto& queueFamily : queueFamilies)
	{
		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			indices.GraphicsFamily = i;
		}
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, VKSurface, &presentSupport);
		if (queueFamily.queueCount > 0 && presentSupport)
		{
			indices.PresentFamily = i;
		}
		if (indices.IsComplete())
		{
			break;
		}
		i++;
	}
	return indices;
}
//-----------------------------------------------------------------------------
const bool VulkanApplication::IsDeviceSuitable(const VkPhysicalDevice& device)
{
	/*
	vkGetPhysicalDeviceProperties(device, &deviceProperties);
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

	return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
	deviceFeatures.geometryShader;
	*/
	QueueFamilyIndices indices = FindQueueFamilies(device);
	bool extensionsSupported = CheckDeviceExtensionSupport(device);
	bool swapChainAdequate = false;
	if (extensionsSupported)
	{
		SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(device);
		swapChainAdequate = !swapChainSupport.Formats.empty() && !swapChainSupport.PresentModes.empty();
	}
	return indices.IsComplete() && extensionsSupported && swapChainAdequate;
}
//-----------------------------------------------------------------------------
void VulkanApplication::CreateLogicalDevice()
{
	QueueFamilyIndices indices = FindQueueFamilies(VKPhysicalDevice);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = { indices.GraphicsFamily, indices.PresentFamily };
	float queuePriority = 1.0f;

	for (int queueFamily : uniqueQueueFamilies)
	{
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType					= VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex		= queueFamily;
		queueCreateInfo.queueCount				= 1;
		queueCreateInfo.pQueuePriorities		= &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	
	VkPhysicalDeviceFeatures deviceFeatures = {};
	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount		= static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos		= queueCreateInfos.data();
	
	createInfo.pEnabledFeatures			= &deviceFeatures;
	createInfo.enabledExtensionCount	= static_cast<uint32_t>(DeviceExtensions.size());
	createInfo.ppEnabledExtensionNames	= DeviceExtensions.data();

	if (EnableValidationLayers)
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(ValidationLayers.size());
		createInfo.ppEnabledLayerNames = ValidationLayers.data();
	}
	else
	{
		createInfo.enabledLayerCount = 0;
	}

	if (vkCreateDevice(VKPhysicalDevice, &createInfo, nullptr, &VKDevice) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create logical device");
	}

	vkGetDeviceQueue(VKDevice, indices.GraphicsFamily, 0, &VKGraphicsQueue);
	vkGetDeviceQueue(VKDevice, indices.PresentFamily, 0, &VKPresentQueue);

}
//-----------------------------------------------------------------------------
// Vulkan needs to have access to a drawable surface.
// Using glfwnative, we have access to our system window initialized in InitWindow
// This is the rolled out version of just calling glfwCreateWindowSurface
void VulkanApplication::CreateVulkanSurface()
{
	//VkWin32SurfaceCreateInfoKHR createInfo = {};
	//createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	//createInfo.hwnd = glfwGetWin32Window(Window);
	//createInfo.hinstance = GetModuleHandle(nullptr);

	//// Get the pointer to function from the vulkan instance.
	//// This will load the function into memory and we will be able to use it.
	//auto CreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR)(vkGetInstanceProcAddr(VKInstance, "vkCreateWin32SurfaceKHR"));
	//if (!CreateWin32SurfaceKHR || CreateWin32SurfaceKHR(VKInstance, &createInfo, nullptr, &VKSurface) != VK_SUCCESS)
	//{
	//	throw std::runtime_error("failed to create window surface!");
	//}
}
//-----------------------------------------------------------------------------
void VulkanApplication::CreateSurface()
{
	if (glfwCreateWindowSurface(VKInstance, Window, nullptr, &VKSurface) != VK_SUCCESS)
	{
		throw std::runtime_error("ffailed to create window surface!");
	}
}
//-----------------------------------------------------------------------------
void VulkanApplication::CreateSwapChain()
{
	SwapChainSupportDetails swapChainSupport	= QuerySwapChainSupport(VKPhysicalDevice);
	VkSurfaceFormatKHR surfaceFormat			= ChooseSwapSurfaceFormat(swapChainSupport.Formats);
	VkPresentModeKHR presentMode				= ChooseSwapPresentMode(swapChainSupport.PresentModes);
	VkExtent2D extent							= ChooseSwapExtent(swapChainSupport.Capabilities);

	uint32_t imageCount = swapChainSupport.Capabilities.minImageCount + 1;
	if (swapChainSupport.Capabilities.maxImageCount > 0 && imageCount > swapChainSupport.Capabilities.maxImageCount)
	{
		imageCount = swapChainSupport.Capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType			= VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface			= VKSurface;
	createInfo.minImageCount	= imageCount;
	createInfo.imageFormat		= surfaceFormat.format;
	createInfo.imageColorSpace	= surfaceFormat.colorSpace;
	createInfo.imageExtent		= extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage		= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	QueueFamilyIndices indices = FindQueueFamilies(VKPhysicalDevice);
	uint32_t queueFamilyIndices[] = { indices.GraphicsFamily, indices.PresentFamily };

	if (indices.GraphicsFamily != indices.PresentFamily)
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	createInfo.preTransform		= swapChainSupport.Capabilities.currentTransform;
	createInfo.compositeAlpha	= VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode		= presentMode;
	createInfo.clipped			= VK_TRUE;
	
	createInfo.oldSwapchain		= VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(VKDevice, &createInfo, nullptr, &VKSwapChain) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create swap chain");
	}

	vkGetSwapchainImagesKHR(VKDevice, VKSwapChain, &imageCount, nullptr);
	VKSwapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(VKDevice, VKSwapChain, &imageCount, VKSwapChainImages.data());

	VKSwapChainImageFormat	= surfaceFormat.format;
	VKSwapChainExtent		= extent;
}
//-----------------------------------------------------------------------------
void VulkanApplication::CreateImageViews()
{
	VKSwapChainImageViews.resize(VKSwapChainImages.size());

	for (size_t i = 0; i < VKSwapChainImages.size(); i++)
	{
		VkImageViewCreateInfo createInfo			= {};
		createInfo.sType							= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image							= VKSwapChainImages[i];
		createInfo.viewType							= VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format							= VKSwapChainImageFormat;
		createInfo.components.r						= VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g						= VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b						= VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a						= VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.subresourceRange.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel	= 0;
		createInfo.subresourceRange.levelCount		= 1;
		createInfo.subresourceRange.baseArrayLayer	= 0;
		createInfo.subresourceRange.layerCount		= 1;

		if (vkCreateImageView(VKDevice, &createInfo, nullptr, &VKSwapChainImageViews[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create image views!");
		}
	}
}
//-----------------------------------------------------------------------------
// Loads shaders, does not create a real pipeline
void VulkanApplication::CreateGraphicsPipeline()
{
	auto vertShaderCode = FileHelper::ReadFile(FileHelper::ContentDir + "/shader/vert.spv");
	auto fragShaderCode = FileHelper::ReadFile(FileHelper::ContentDir + "/shader/frag.spv");

	VkShaderModule vertShaderModule = CreateShaderModule(vertShaderCode);
	VkShaderModule fragShaderModule = CreateShaderModule(fragShaderCode);

	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	
	auto bindingDescription = Vertex::GetBindingDescription();
	auto attributeDescriptions = Vertex::GetAttributeDescriptions();

	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = {};
	inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width	= (float)VKSwapChainExtent.width;
	viewport.height = (float)VKSwapChainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = VKSwapChainExtent;

	VkPipelineViewportStateCreateInfo viewportStateInfo = {};
	viewportStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateInfo.viewportCount = 1;
	viewportStateInfo.pViewports = &viewport;
	viewportStateInfo.scissorCount = 1;
	viewportStateInfo.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizerInfo = {};
	rasterizerInfo.sType					= VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizerInfo.depthClampEnable			= VK_FALSE;
	rasterizerInfo.rasterizerDiscardEnable	= VK_FALSE;
	rasterizerInfo.polygonMode				= VK_POLYGON_MODE_FILL;
	rasterizerInfo.lineWidth				= 1.0f;
	rasterizerInfo.cullMode					= VK_CULL_MODE_BACK_BIT;
	rasterizerInfo.frontFace				= VK_FRONT_FACE_CLOCKWISE;
	rasterizerInfo.depthBiasEnable			= VK_FALSE;

	VkPipelineMultisampleStateCreateInfo multisamplingInfo = {};
	multisamplingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisamplingInfo.sampleShadingEnable = VK_FALSE;
	multisamplingInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {};
	colorBlendAttachmentState.colorWriteMask =	  VK_COLOR_COMPONENT_R_BIT 
												| VK_COLOR_COMPONENT_G_BIT 
												| VK_COLOR_COMPONENT_B_BIT
												| VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachmentState.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo colorBlendingInfo = {};
	colorBlendingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendingInfo.logicOpEnable = VK_FALSE;
	colorBlendingInfo.logicOp = VK_LOGIC_OP_COPY;
	colorBlendingInfo.attachmentCount = 1;
	colorBlendingInfo.pAttachments = &colorBlendAttachmentState;
	colorBlendingInfo.blendConstants[0] = 0.0f;
	colorBlendingInfo.blendConstants[1] = 0.0f;
	colorBlendingInfo.blendConstants[2] = 0.0f;
	colorBlendingInfo.blendConstants[3] = 0.0f;

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pushConstantRangeCount = &VKDescriptorSetLayout;

	if (vkCreatePipelineLayout(VKDevice, &pipelineLayoutInfo, nullptr, &VKPipelineLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create pipeline layout");
	}

	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
	pipelineInfo.pViewportState = &viewportStateInfo;
	pipelineInfo.pRasterizationState = &rasterizerInfo;
	pipelineInfo.pMultisampleState = &multisamplingInfo;
	pipelineInfo.pColorBlendState = &colorBlendingInfo;
	pipelineInfo.layout = VKPipelineLayout;
	pipelineInfo.renderPass = VKRenderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	if (vkCreateGraphicsPipelines(VKDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &VKGraphicsPipeline) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create graphics pipeline!");
	}

	vkDestroyShaderModule(VKDevice, fragShaderModule, nullptr);
	vkDestroyShaderModule(VKDevice, vertShaderModule, nullptr);
}
//-----------------------------------------------------------------------------
void VulkanApplication::CreateRenderPass()
{
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format			= VKSwapChainImageFormat;
	colorAttachment.samples			= VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp			= VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp			= VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp	= VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout		= VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment	= 0;
	colorAttachmentRef.layout		= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint		= VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount	= 1;
	subpass.pColorAttachments		= &colorAttachmentRef;

	VkSubpassDependency dependency = {};
	dependency.srcSubpass		= VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass		= 0;
	dependency.srcStageMask		= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask	= 0;
	dependency.dstStageMask		= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask	= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;


	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType			= VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount	= 1;
	renderPassInfo.pAttachments		= &colorAttachment;
	renderPassInfo.subpassCount		= 1;
	renderPassInfo.pSubpasses		= &subpass;
	renderPassInfo.dependencyCount	= 1;
	renderPassInfo.pDependencies	= &dependency;

	if (vkCreateRenderPass(VKDevice, &renderPassInfo, nullptr, &VKRenderPass) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create render pass!");
	}
}
//-----------------------------------------------------------------------------
VkShaderModule VulkanApplication::CreateShaderModule(const std::vector<char>& code)
{
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(VKDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create shader module");
	}

	return shaderModule;
}
//-----------------------------------------------------------------------------
void VulkanApplication::CreateFramebuffers()
{
	VKSwapChainFramebuffers.resize(VKSwapChainImageViews.size());
	for (size_t i = 0; i < VKSwapChainImageViews.size(); i++)
	{
		VkImageView attachments[] = 
		{
			VKSwapChainImageViews[i]
		};

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = VKRenderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = VKSwapChainExtent.width;
		framebufferInfo.height = VKSwapChainExtent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(VKDevice, &framebufferInfo, nullptr, &VKSwapChainFramebuffers[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create framebuffer!");
		}
	}
}
//-----------------------------------------------------------------------------
void VulkanApplication::CreateCommandPool()
{
	QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(VKPhysicalDevice);
	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamilyIndices.GraphicsFamily;

	if (vkCreateCommandPool(VKDevice, &poolInfo, nullptr, &VKCommandPool) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create command pool!");
	}
}
//-----------------------------------------------------------------------------
void VulkanApplication::CreateCommandBuffers()
{
	VKCommandBuffers.resize(VKSwapChainFramebuffers.size());

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = VKCommandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)VKCommandBuffers.size();

	if (vkAllocateCommandBuffers(VKDevice, &allocInfo, VKCommandBuffers.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate command buffers!");
	}

	for (size_t i = 0; i < VKCommandBuffers.size(); i++)
	{
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

		if (vkBeginCommandBuffer(VKCommandBuffers[i], &beginInfo) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to begin recording command buffer");
		}

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = VKRenderPass;
		renderPassInfo.framebuffer = VKSwapChainFramebuffers[i];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = VKSwapChainExtent;

		VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		vkCmdBeginRenderPass(VKCommandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

			vkCmdBindPipeline(VKCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, VKGraphicsPipeline);
			VkBuffer vertexBuffers[] = { VKVertexBuffer };
			VkDeviceSize offsets[] = { 0 };

			vkCmdBindVertexBuffers(VKCommandBuffers[i], 0, 1, vertexBuffers, offsets);

			vkCmdBindIndexBuffer(VKCommandBuffers[i], VKIndexBuffer, 0, VK_INDEX_TYPE_UINT16);
			vkCmdDrawIndexed(VKCommandBuffers[i], static_cast<uint32_t>(class_indices.size()), 1, 0, 0, 0);

		vkCmdEndRenderPass(VKCommandBuffers[i]);

		if (vkEndCommandBuffer(VKCommandBuffers[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to record command buffer!");
		}

	}
}
//-----------------------------------------------------------------------------
void VulkanApplication::CreateSemaphores()
{
	VKImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	VKRenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	VKInFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		if (vkCreateSemaphore(VKDevice, &semaphoreInfo, nullptr, &VKImageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(VKDevice, &semaphoreInfo, nullptr, &VKRenderFinishedSemaphores[i]) != VK_SUCCESS || 
			vkCreateFence(VKDevice, &fenceInfo, nullptr, &VKInFlightFences[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create semaphores!");
		}
	}
}
//-----------------------------------------------------------------------------
void VulkanApplication::CreateVertexBuffer() 
{
	vertices = Vertex::MakeRGBTriangle();
	VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
	
	// OLD STYLE C: WHY
	void* data;
	vkMapMemory(VKDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, vertices.data(), (size_t)bufferSize);
	vkUnmapMemory(VKDevice, stagingBufferMemory);

	CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VKVertexBuffer, VKVertexBufferMemory);
	CopyBuffer(stagingBuffer, VKVertexBuffer, bufferSize);

	vkDestroyBuffer(VKDevice, stagingBuffer, nullptr);
	vkFreeMemory(VKDevice, stagingBufferMemory, nullptr);
}
//-----------------------------------------------------------------------------
void VulkanApplication::CreateBuffer(const VkDeviceSize size, const VkBufferUsageFlags usage, const VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
	VkBufferCreateInfo bufferInfo	= {};
	bufferInfo.sType				= VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size					= size;
	bufferInfo.usage				= usage;
	bufferInfo.sharingMode			= VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(VKDevice, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create buffer!");
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(VKDevice, buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo	= {};
	allocInfo.sType					= VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize		= memRequirements.size;
	allocInfo.memoryTypeIndex		= FindMemoryType(memRequirements.memoryTypeBits, properties);
	
	if (vkAllocateMemory(VKDevice, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate buffer memory");
	}

	vkBindBufferMemory(VKDevice, buffer, bufferMemory, 0);
}
//-----------------------------------------------------------------------------
void VulkanApplication::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = VKCommandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(VKDevice, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	VkBufferCopy copyRegion = {};
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(VKGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(VKGraphicsQueue);

	vkFreeCommandBuffers(VKDevice, VKCommandPool, 1, &commandBuffer);
}
//-----------------------------------------------------------------------------
void VulkanApplication::CreateIndexBuffer()
{
	class_indices = Indices::MakeSquareIndices();
	VkDeviceSize bufferSize = sizeof(class_indices[0]) * class_indices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
	
	void* data;
	vkMapMemory(VKDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, class_indices.data(), (size_t)bufferSize);
	vkUnmapMemory(VKDevice, stagingBufferMemory);

	CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VKIndexBuffer, VKIndexBufferMemory);
	CopyBuffer(stagingBuffer, VKIndexBuffer, bufferSize);

	vkDestroyBuffer(VKDevice, stagingBuffer, nullptr);
	vkFreeMemory(VKDevice, stagingBufferMemory, nullptr);
}
//-----------------------------------------------------------------------------
void VulkanApplication::CreateDescriptorSetLayout()
{
	VkDescriptorSetLayoutBinding uboLayoutBinding = {};
	uboLayoutBinding.binding			= 0;
	uboLayoutBinding.descriptorType		= VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount	= 1;
	uboLayoutBinding.stageFlags			= VK_SHADER_STAGE_VERTEX_BIT;
	uboLayoutBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings = &uboLayoutBinding;

	if (vkCreateDescriptorSetLayout(VKDevice, &layoutInfo, nullptr, &VKDescriptorSetLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create descriptor set layout!");
	}
}
//-----------------------------------------------------------------------------
void VulkanApplication::CreateUniformBuffer()
{
	VkDeviceSize bufferSize = sizeof(UniformTransformBufferObject);
	VKUniformBuffers.resize(VKSwapChainImages.size());
	VKUniformBuffersMemory.resize(VKSwapChainImages.size());

	for (size_t i = 0; i < VKSwapChainImages.size(); i++)
	{
		CreateBuffer(bufferSize, 
					VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
					VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
					VKUniformBuffers[i], VKUniformBuffersMemory[i]);
	}
}
//-----------------------------------------------------------------------------
void VulkanApplication::DrawFrame()
{
	vkWaitForFences(VKDevice, 1, &VKInFlightFences[CurrentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());
	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(VKDevice, VKSwapChain, std::numeric_limits<std::uint64_t>::max(), VKImageAvailableSemaphores[CurrentFrame], VK_NULL_HANDLE, &imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		RecreateSwapChain();	
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		throw std::runtime_error("Failed to acquire swap chain image!");
	}

	
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { VKImageAvailableSemaphores[CurrentFrame] };

	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount	= 1;
	submitInfo.pWaitSemaphores		= waitSemaphores;
	submitInfo.pWaitDstStageMask	= waitStages;
	submitInfo.commandBufferCount	= 1;
	submitInfo.pCommandBuffers		= &VKCommandBuffers[imageIndex];

	VkSemaphore signalSemaphores[]	= { VKRenderFinishedSemaphores[CurrentFrame] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores	= signalSemaphores;

	vkResetFences(VKDevice, 1, &VKInFlightFences[CurrentFrame]);
	result = vkQueueSubmit(VKGraphicsQueue, 1, &submitInfo, VKInFlightFences[CurrentFrame]);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to submit draw command buffer!");
	}

	VkPresentInfoKHR presentInfo	= {};
	presentInfo.sType				= VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount	= 1;
	presentInfo.pWaitSemaphores		= signalSemaphores;

	VkSwapchainKHR swapChains[]		= { VKSwapChain };
	presentInfo.swapchainCount		= 1;
	presentInfo.pSwapchains			= swapChains;
	presentInfo.pImageIndices		= &imageIndex;

	result = vkQueuePresentKHR(VKPresentQueue, &presentInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized)
	{
		framebufferResized = false;
		RecreateSwapChain();
	}
	else if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to acquire swap chain image!");
	}
	CurrentFrame = (CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}
//-----------------------------------------------------------------------------
void VulkanApplication::UpdateUniformBuffer(uint32_t currentImage)
{
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	UniformTransformBufferObject ubo = {};
	ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.proj = glm::perspective(glm::radians(45.0f), VKSwapChainExtent.width / (float)VKSwapChainExtent.height, 0.1f, 10.0f);
	// GH: OGL inversion clip coordinates.
	ubo.proj[1][1] *= -1;

	void* data;
	vkMapMemory(VKDevice, VKUniformBuffersMemory[currentImage], 0, sizeof(ubo), 0 & data);
	memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(VKDevice, VKUniformBuffersMemory[currentImage]);
}
//-----------------------------------------------------------------------------
void VulkanApplication::RecreateSwapChain()
{
	int width = 0, height = 0;
	while (width == 0 || height == 0)
	{
		glfwGetFramebufferSize(Window, &width, &height);
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(VKDevice);
	CleanupSwapChain();
	CreateSwapChain();
	CreateImageViews();
	CreateRenderPass();
	CreateGraphicsPipeline();
	CreateFramebuffers();
	CreateCommandBuffers();
}
//-----------------------------------------------------------------------------
void VulkanApplication::CleanupSwapChain() const
{
	for (auto frameBuffer : VKSwapChainFramebuffers)
	{
		vkDestroyFramebuffer(VKDevice, frameBuffer, nullptr);
	}

	vkFreeCommandBuffers(VKDevice, VKCommandPool, static_cast<uint32_t>(VKCommandBuffers.size()), VKCommandBuffers.data());

	vkDestroyPipeline(VKDevice, VKGraphicsPipeline, nullptr);
	vkDestroyPipelineLayout(VKDevice, VKPipelineLayout, nullptr);
	vkDestroyRenderPass(VKDevice, VKRenderPass, nullptr);

	for(auto image : VKSwapChainImageViews)
	{
		vkDestroyImageView(VKDevice, image, nullptr);
	}

	vkDestroySwapchainKHR(VKDevice, VKSwapChain, nullptr);
}
//-----------------------------------------------------------------------------
void VulkanApplication::SetupDebugCallback() const
{
	if (!EnableValidationLayers) return;

	VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
		| VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
		| VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
		| VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
		| VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

	createInfo.pfnUserCallback = DebugCallback;
	//createInfo.pUserData = nullptr;
	if (CreateDebugUtilsMessengerEXT(VKInstance, &createInfo, nullptr, &callback) != VK_SUCCESS) {
		throw std::runtime_error("failed to set up debug callback!");
	}
}
//-----------------------------------------------------------------------------
// TO-DO: arbitrary scoring for now. Need to add more information and a better rating system
const int32_t VulkanApplication::RateDeviceSuitability(const VkPhysicalDevice& device) const
{
	int32_t score = 0;
	// Rate higher the discrete devices type
	if (VKDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
	{
		score += 1000;
	}
	
	score += VKDeviceProperties.limits.maxImageDimension2D;

	if (VKDeviceFeatures.geometryShader == false)
	{
		return 0;
	}
	return score;
}
//-----------------------------------------------------------------------------
const uint32_t VulkanApplication::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(VKPhysicalDevice, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
	{
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}

	throw std::runtime_error("Failed to find suitable memory type");
}
//-----------------------------------------------------------------------------
