//-----------------------------------------------------------------------------
#ifndef _VULKANAPPLICATION_H_
#define _VULKANAPPLICATION_H_
//-----------------------------------------------------------------------------
// This defines allow access to GLFW & Vulkan OS specific surface handling
//#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
//-----------------------------------------------------------------------------
#define NOMINMAX
#pragma region STL Include
#include <vector>
#include <limits>
#include <algorithm>
#include <math.h>
#include <stdexcept>
#include <map>
#include <set>
#include <iostream>
#pragma endregion
#pragma region Vulkan include
#include <vulkan/vk_icd.h>
#include <vulkan/vk_layer.h>
#include <vulkan/vk_layer.h>
#include <vulkan/vk_sdk_platform.h>
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan.h>
#pragma endregion
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include "FileHelper.h"

#include "geom/Indices.h"
#include "geom/Vertex.h"
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>
using namespace geom
//-----------------------------------------------------------------------------
struct QueueFamilyIndices
{
	uint32_t GraphicsFamily	= -1;
	uint32_t PresentFamily	= -1;
	bool IsComplete()
	{
		return GraphicsFamily >= 0 && PresentFamily >= 0;
	}
};

//-----------------------------------------------------------------------------
struct UniformTransformBufferObject
{
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
};
//-----------------------------------------------------------------------------
struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR Capabilities;
	std::vector<VkSurfaceFormatKHR> Formats;
	std::vector<VkPresentModeKHR> PresentModes;
};
//-----------------------------------------------------------------------------
const std::vector<const char*> ValidationLayers = { "VK_LAYER_LUNARG_standard_validation" };
const std::vector<const char*> DeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
//-----------------------------------------------------------------------------
#ifdef NDEBUG
const bool EnableValidationLayers = false;
#else
const bool EnableValidationLayers = true;
#endif

#pragma region VK_EXT_DEBUG
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
#pragma endregion
//-----------------------------------------------------------------------------
class VulkanApplication
{
public:
	VulkanApplication();
	~VulkanApplication();

	void Start();
	void Loop() ;
	void Cleanup() const;

	bool framebufferResized = false;

private:

	// Initialization funcs
	void InitWindow();
	void InitVulkan();
	const bool CheckValidationLayerSupport() const;
	const bool CheckDeviceExtensionSupport(const VkPhysicalDevice& device) const;
	void CreateInstance() const;
	void PickPhysicalDevice();
	void ListVulkanExtensions() const;
	// Extension support
	const std::vector<const char*> GetRequiredExtensions() const;
	const SwapChainSupportDetails QuerySwapChainSupport(const VkPhysicalDevice& device);
	const VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	const VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	const VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
	const QueueFamilyIndices FindQueueFamilies(const VkPhysicalDevice& device);
	const bool IsDeviceSuitable(const VkPhysicalDevice& device);
#pragma region Creation 
	void CreateLogicalDevice();
	void CreateVulkanSurface();
	void CreateSurface();
	void CreateSwapChain();
	void CreateImageViews();
	void CreateGraphicsPipeline();
	void CreateRenderPass();
	void CreateFramebuffers();
	void CreateCommandPool();
	void CreateCommandBuffers();
	void CreateSemaphores();
	void CreateVertexBuffer();
	void CreateBuffer(const VkDeviceSize size, const VkBufferUsageFlags usage, const VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	void CreateIndexBuffer();
	void CreateDescriptorSetLayout();
	void CreateUniformBuffer();
#pragma endregion

#pragma region Update

	void DrawFrame();
	void UpdateUniformBuffer(uint32_t currentImage);
	void RecreateSwapChain();
	void CleanupSwapChain() const;
#pragma endregion
	VkShaderModule CreateShaderModule(const std::vector<char>& code);
	void SetupDebugCallback() const;
	const int32_t RateDeviceSuitability(const VkPhysicalDevice& device) const;

	const uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
private:
	
	const int WIDTH = 800;
	const int HEIGHT = 600;
	const int MAX_FRAMES_IN_FLIGHT = 2;
	size_t CurrentFrame = 0;

	
	// GH Add this to questions. How mutable should be handled?
	// Should mutable be abused? Is it even const correct to do that?
	mutable GLFWwindow*  Window;
#pragma region Vulkan Vars
	mutable VkInstance VKInstance;
	mutable VkDebugUtilsMessengerEXT callback;
	VkPhysicalDeviceProperties VKDeviceProperties;
	VkPhysicalDeviceFeatures VKDeviceFeatures;
	VkPhysicalDevice VKPhysicalDevice;
	VkDevice VKDevice;
	VkQueue VKGraphicsQueue;
	VkSurfaceKHR VKSurface;
	VkQueue VKPresentQueue;
	VkSwapchainKHR VKSwapChain;
	std::vector<VkImage> VKSwapChainImages;
	VkFormat VKSwapChainImageFormat;
	VkExtent2D VKSwapChainExtent;
	VkRenderPass VKRenderPass;
	VkPipeline VKGraphicsPipeline;
	VkDescriptorSetLayout VKDescriptorSetLayout;
	VkPipelineLayout VKPipelineLayout;

#pragma region VK Buffers
	VkCommandPool VKCommandPool;
	VkBuffer VKVertexBuffer;
	VkDeviceMemory VKVertexBufferMemory;
	VkBuffer VKIndexBuffer;
	VkDeviceMemory VKIndexBufferMemory;

	std::vector<VkBuffer> VKUniformBuffers;
	std::vector<VkDeviceMemory> VKUniformBuffersMemory;
	std::vector<VkCommandBuffer> VKCommandBuffers;
	std::vector<VkImageView> VKSwapChainImageViews;
	std::vector<VkFramebuffer> VKSwapChainFramebuffers;
#pragma endregion

#pragma region VK Semaphore / Sync Objects
	std::vector<VkSemaphore> VKImageAvailableSemaphores;
	std::vector<VkSemaphore> VKRenderFinishedSemaphores;
	std::vector<VkFence> VKInFlightFences;
#pragma endregion
	
	std::vector<uint16_t> class_indices;
	std::vector<Vertex> vertices;
#pragma endregion
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
};

#pragma region GLFW_CALLBACK
static void FramebufferResizeCallback(GLFWwindow* window, int width, int height)
{
	auto app = reinterpret_cast<VulkanApplication*>(glfwGetWindowUserPointer(window));
	app->framebufferResized = true;
}
#pragma endregion
//-----------------------------------------------------------------------------
#endif // _VULKANAPPLICATION_H_
//-----------------------------------------------------------------------------