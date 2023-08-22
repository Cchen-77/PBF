#ifndef RENDERER_TYPES_H
#define RENDERER_TYPES_H
#define GLFW_INCLUDE_VULKAN
#include"GLFW/glfw3.h"
#include"glm/glm.hpp"

#include<vector>
#include<array>
#include<optional>
struct QueuefamliyIndices{
    std::optional<uint32_t> graphicNcompute;
    std::optional<uint32_t> present;
    bool IsCompleted(){
        return graphicNcompute.has_value()&&present.has_value();
    }
};
struct SurfaceDetails{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentmode;
    
};
enum class TickResult{
    NONE,
    EXIT,
};
struct Vertex{
    glm::vec3 Location;
    glm::vec3 Normal;
    glm::vec4 Color;
    glm::vec2 TexCoord;
    static VkVertexInputBindingDescription GetBinding(){
       VkVertexInputBindingDescription binding{};
       binding.binding = 0;
       binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
       binding.stride = sizeof(Vertex);
       return binding;
    }
    static std::array<VkVertexInputAttributeDescription,4> GetAttributes(){
        std::array<VkVertexInputAttributeDescription,4> attributes;
        attributes[0].binding = 0;
        attributes[0].location = 0;
        attributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributes[0].offset = offsetof(Vertex,Location);

        attributes[1].binding = 0;
        attributes[1].location = 1;
        attributes[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributes[1].offset = offsetof(Vertex,Normal);

        attributes[2].binding = 0;
        attributes[2].location = 2;
        attributes[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributes[2].offset = offsetof(Vertex,Color);

        attributes[3].binding = 0;
        attributes[3].location = 3;
        attributes[3].format = VK_FORMAT_R32G32_SFLOAT;
        attributes[3].offset = offsetof(Vertex,TexCoord);
        return attributes;
    }
};
struct UniformMVPObject{
    alignas(16) glm::mat4 model = glm::mat4(1.0f);
    alignas(16) glm::mat4 view = glm::mat4(1.0f);
    alignas(16) glm::mat4 projection = glm::mat4(1.0f);
};
#endif