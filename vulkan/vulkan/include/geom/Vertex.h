#ifndef _VERTEX_H_
#define _VERTEX_H_
#include <glm/glm.hpp>
#include <array>
#include <vulkan/vulkan.h>
namespace geom
{
	class Vertex
	{
	public:
		glm::vec2 pos;
		glm::vec3 color;

		static VkVertexInputBindingDescription GetBindingDescription()
		{
			VkVertexInputBindingDescription bindingDescription = {};

			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(Vertex);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
			return bindingDescription;
		}

		static std::array<VkVertexInputAttributeDescription, 2> GetAttributeDescriptions()
		{
			std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions = {};

			attributeDescriptions[0].binding = 0;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDescriptions[0].offset = offsetof(Vertex, pos);

			attributeDescriptions[1].binding = 0;
			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDescriptions[1].offset = offsetof(Vertex, color);

			return attributeDescriptions;
		}

		static std::vector<Vertex> MakeRGBTriangle()
		{
			std::vector<Vertex> vertices =
			{	// X      Y       R     G     B
				{{0.0f, -0.5f},{1.0f, 1.0f, 1.0f}},
				{{0.5f, 0.5f},{0.0f, 1.0f, 0.0f}},
				{{-0.5f, 0.5f},{0.0f, 0.0f, 1.0f}}
			};
			return vertices;
		}
	};
}
#endif // !_VERTEX_H_
