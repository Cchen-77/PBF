#ifndef EXTENSIONFUNCS_H
#define EXTENSIONFUNCS_H
#include"vulkan/vulkan.h"
class ExtensionFuncs{
public:
    static VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugUtilsMessengerEXT(
        VkInstance                                  instance,
        const VkDebugUtilsMessengerCreateInfoEXT*   pCreateInfo,
        const VkAllocationCallbacks*                pAllocator,
        VkDebugUtilsMessengerEXT*                   pMessenger);
    static VKAPI_ATTR void VKAPI_CALL vkDestroyDebugUtilsMessengerEXT(
    VkInstance                                  instance,
    VkDebugUtilsMessengerEXT                    messenger,
    const VkAllocationCallbacks*                pAllocator);
    static VKAPI_ATTR void VKAPI_CALL vkCmdSetColorBlendEnableEXT(VkDevice device,VkCommandBuffer commandBuffer, uint32_t firstAttachment, uint32_t attachmentCount, 
    const VkBool32* pColorBlendEnables);
     static VKAPI_ATTR void VKAPI_CALL vkCmdSetDepthTestEnable(VkDevice device,VkCommandBuffer commandBuffer,VkBool32 depthTestEnable);
     

};
#endif