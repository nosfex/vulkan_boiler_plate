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
void VulkanApplication::InitWindow()
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	Window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
}
//-----------------------------------------------------------------------------
void VulkanApplication::CreateInstance()
{

	if(EnableValidationLayers && !CheckValidationLayerSupport())
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
	// GH: Create and setup all the data for creating the instance
	if (vkCreateInstance(&createInfo, nullptr, &VKInstance) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create vulkan instance!");
	}


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
std::vector<const char*> VulkanApplication::GetRequiredExtensions() 
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
void VulkanApplication::InitVulkan() 
{
	CreateInstance();

	SetupDebugCallback();
}
//-----------------------------------------------------------------------------
void VulkanApplication::SetupDebugCallback()
{
	if (!EnableValidationLayers) return;

	VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity =	VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT 
								|	VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
								|	VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType =	VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
							|	VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT 
							|	VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

	createInfo.pfnUserCallback = DebugCallback;
	//createInfo.pUserData = nullptr;
	if (CreateDebugUtilsMessengerEXT(VKInstance, &createInfo, nullptr, &callback) != VK_SUCCESS) {
		throw std::runtime_error("failed to set up debug callback!");
	}
}
//-----------------------------------------------------------------------------
void VulkanApplication::Cleanup()
{
	if (EnableValidationLayers)
	{
		DestroyDebugUtilsMessengerEXT(VKInstance, callback, nullptr);
	}
	// GH : VK Cleanup
	vkDestroyInstance(VKInstance, nullptr);
	// GH :  GLFW cleanup
	glfwDestroyWindow(Window);
	glfwTerminate();
}
//-----------------------------------------------------------------------------
bool VulkanApplication::CheckValidationLayerSupport() const
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
void VulkanApplication::Loop() const
{
	while (!glfwWindowShouldClose(Window))
	{
		glfwPollEvents();
	}
}
//-----------------------------------------------------------------------------