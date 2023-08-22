#include "extensionfuncs.h"

VKAPI_ATTR VkResult VKAPI_CALL ExtensionFuncs::CreateDebugUtilsMessengerEXT(VkInstance instance, 
const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, 
const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pMessenger)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)(vkGetInstanceProcAddr(instance,"vkCreateDebugUtilsMessengerEXT"));
    if(!func) return VK_ERROR_EXTENSION_NOT_PRESENT;
    return func(instance,pCreateInfo,pAllocator,pMessenger);
}

VKAPI_ATTR void VKAPI_CALL ExtensionFuncs::DestroyDebugUtilsMessengerEXT(VkInstance instance, 
VkDebugUtilsMessengerEXT messenger, const VkAllocationCallbacks *pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)(vkGetInstanceProcAddr(instance,"vkDestroyDebugUtilsMessengerEXT"));
    if(!func) return;
    return func(instance,messenger,pAllocator);
}
