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
enum class TickWindowResult{
    NONE,
    HIDE,
    EXIT,
};
struct Particle{
    alignas(16) glm::vec3 Location;
    alignas(16) glm::vec3 Velocity;
    alignas(16) glm::vec3 DeltaLocation;
    alignas(4) float Lambda;
    alignas(4) float Density;
    alignas(4) float Mass;

    alignas(16) glm::vec3 TmpVelocity;

    alignas(4) uint32_t CellHash;
    alignas(4) uint32_t TmpCellHash;

    alignas(4) uint32_t NumNgbrs;

    static VkVertexInputBindingDescription GetBinding(){
        VkVertexInputBindingDescription binding{};
        binding.binding = 0;
        binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        binding.stride = sizeof(Particle);
        return binding;
    } 
    static std::array<VkVertexInputAttributeDescription,1> GetAttributes(){
        std::array<VkVertexInputAttributeDescription,1> attributes;
        attributes[0].binding = 0;
        attributes[0].location = 0;
        attributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributes[0].offset = offsetof(Particle,Location);
        return attributes;
    }
};
struct UniformRenderingObject{
    alignas(4) float zNear;
    alignas(4) float zFar;
    alignas(4) float fovy;
    alignas(4) float aspect;

    alignas(16) glm::mat4 model = glm::mat4(1.0f);
    alignas(16) glm::mat4 view = glm::mat4(1.0f);
    alignas(16) glm::mat4 projection = glm::mat4(1.0f);
    alignas(16) glm::mat4 inv_projection = glm::mat4(1.0f);
    
    alignas(4) float particleRadius;
};
struct UniformSimulatingObject{
    alignas(4) float dt;
    alignas(4) float accumulated_t;
    alignas(4) float restDensity;
    alignas(4) float sphRadius;
    alignas(4) uint32_t numParticles;

    alignas(4) float coffPoly6;
    alignas(4) float coffSpiky;
    alignas(4) float coffGradSpiky;

    alignas(4) float scorrK;
    alignas(4) float scorrN;
    alignas(4) float scorrQ;
};
struct UniformNSObject{
    alignas(4) uint32_t numParticles;

    alignas(4) uint32_t workgroup_count;
    alignas(4) uint32_t hashsize;

    alignas(4) float sphRadius;
};
struct UniformBoxInfoObject{
    alignas(8) glm::vec2 clampX;
    alignas(8) glm::vec2 clampY;
    alignas(8) glm::vec2 clampZ; 
};
#endif