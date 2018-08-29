//-----------------------------------------------------------------------------
#ifndef _VULKANAPPLICATION_H_
#define _VULKANAPPLICATION_H_
//-----------------------------------------------------------------------------
#include <vector>
#include <stdexcept>
#include <vulkan/vk_icd.h>
#include <vulkan/vk_layer.h>
#include <vulkan/vk_layer.h>
#include <vulkan/vk_sdk_platform.h>
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <iostream>

const std::vector<const char*> ValidationLayers = { "VK_LAYER_LUNARG_standard_validation" };

//#define NDEBUG 1
#ifdef NDEBUG
const bool EnableValidationLayers = false;
#else
const bool EnableValidationLayers = true;
#endif
static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, 
								const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
								const VkAllocationCallbacks* pAllocator, 
								VkDebugUtilsMessengerEXT* pCallback)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		return func(instance, pCreateInfo, pAllocator, pCallback);
	}
	else
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}
static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT callback, const VkAllocationCallbacks* pAllocator) {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, callback, pAllocator);
	}
}
//-----------------------------------------------------------------------------
class VulkanApplication
{
public:
	VulkanApplication();
	~VulkanApplication();

	void Start();
	void Loop() const;
	void Cleanup();

private:
	void InitWindow();
	void InitVulkan();
	bool CheckValidationLayerSupport() const;
	void CreateInstance();

	void ListVulkanExtensions() const;
	// Extension support
	std::vector<const char*> GetRequiredExtensions();
private:
	// load this by 
	const int WIDTH = 800;
	const int HEIGHT = 600;

	GLFWwindow*  Window;
	VkInstance VKInstance;
	VkDebugUtilsMessengerEXT callback;
// DEBUG MESSAGING & Callback
	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
														VkDebugUtilsMessageTypeFlagsEXT messageType,
														const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
														void* pUserData)
	{
		std::cerr << "Validation Layer: " << pCallbackData->pMessage << std::endl;
		if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
			// Message is important enough to show
		}
		return VK_FALSE;
	}



	void SetupDebugCallback();
};
//-----------------------------------------------------------------------------
#endif // _VULKANAPPLICATION_H_
//-----------------------------------------------------------------------------