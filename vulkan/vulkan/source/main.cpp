#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <iostream>
#include "app/VulkanApplication.h"
int main()
{
	VulkanApplication* vkApp = new VulkanApplication();
	
	vkApp->Start();
	vkApp->Loop();
	vkApp->Cleanup();
	return 0;
}