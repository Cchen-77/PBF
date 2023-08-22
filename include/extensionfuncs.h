#ifndef EXTENSIONFUNCS_H
#define EXTENSIONFUNCS_H
#include"vulkan/vulkan.h"
class ExtensionFuncs{
public:
    static VKAPI_ATTR VkResult VKAPI_CALL CreateDebugUtilsMessengerEXT(
        VkInstance                                  instance,
        const VkDebugUtilsMessengerCreateInfoEXT*   pCreateInfo,
        const VkAllocationCallbacks*                pAllocator,
        VkDebugUtilsMessengerEXT*                   pMessenger);
    static VKAPI_ATTR void VKAPI_CALL DestroyDebugUtilsMessengerEXT(
    VkInstance                                  instance,
    VkDebugUtilsMessengerEXT                    messenger,
    const VkAllocationCallbacks*                pAllocator);

};
#endif