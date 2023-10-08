#include "extensionfuncs.h"

#include<iostream>
VKAPI_ATTR VkResult VKAPI_CALL ExtensionFuncs::vkCreateDebugUtilsMessengerEXT(VkInstance instance, 
const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, 
const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pMessenger)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)(vkGetInstanceProcAddr(instance,"vkCreateDebugUtilsMessengerEXT"));
    if(!func) return VK_ERROR_EXTENSION_NOT_PRESENT;
    return func(instance,pCreateInfo,pAllocator,pMessenger);
}

VKAPI_ATTR void VKAPI_CALL ExtensionFuncs::vkDestroyDebugUtilsMessengerEXT(VkInstance instance, 
VkDebugUtilsMessengerEXT messenger, const VkAllocationCallbacks *pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)(vkGetInstanceProcAddr(instance,"vkDestroyDebugUtilsMessengerEXT"));
    if(!func) return;
    return func(instance,messenger,pAllocator);
}

VKAPI_ATTR void VKAPI_CALL ExtensionFuncs::vkCmdSetColorBlendEnableEXT(VkDevice device,VkCommandBuffer commandBuffer, uint32_t firstAttachment, uint32_t attachmentCount, const VkBool32 *pColorBlendEnables)
{
    auto func = (PFN_vkCmdSetColorBlendEnableEXT)(vkGetDeviceProcAddr(device,"vkCmdSetColorBlendEnableEXT"));
    if(!func) return;
    func(commandBuffer,firstAttachment,attachmentCount,pColorBlendEnables);
}

VKAPI_ATTR void VKAPI_CALL ExtensionFuncs::vkCmdSetDepthTestEnable(VkDevice device,VkCommandBuffer commandBuffer, VkBool32 depthTestEnable)
{
    auto func = (PFN_vkCmdSetDepthTestEnable)(vkGetDeviceProcAddr(device,"vkCmdSetDepthTestEnable"));
    if(!func){
        throw std::runtime_error("failed to get function vkCmdSetDepthTestEnable!");
        return;
    }
    func(commandBuffer,depthTestEnable);
}
