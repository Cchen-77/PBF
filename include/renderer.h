#ifndef RENDER_H
#define RENDER_H
#define GLFW_INCLUDE_VULKAN
#include"GLFW/glfw3.h"
#include"glm/glm.hpp"
#include"renderer_types.h"

#include<vector>
#include<array>
#include<optional>
#include<string>

class Renderer{
public:
    Renderer(uint32_t w,uint32_t h,RendererFeaturesFlag feature,bool validation = false,std::string texfile="");
    virtual ~Renderer();
public:
    TickResult Tick(float DeltaTime);
    void Draw();
private:
    void Init();
    void Cleanup();
private:
    void CreateInstance();
    void CreateDebugMessenger();
    void CreateSurface();
    void PickPhysicalDevice();
    void CreateLogicalDevice();

    void GetMSAASampleCount();
    void CreateSupportObjects();
    void CleanupSupportObjects();
    void CreateCommandPool();
    void CreateVertexBuffer();
    void CreateIndexBuffer();
    void CreateParticleBuffer();
    void CreateUniformMVPBuffer();
    void CreateUniformComputeBuffer();
    void CreateTextureResources();
    void CreateDepthResources();
    void CreateMSAAResources();

    void CreateSwapChain();
    void CleanupSwapChain();
    void RecreateSwapChain();

    void CreateDescriptorSetLayout();
    void CreateDescriptorPool();
    void CreateDescriptorSet();

    void CreateRenderPass();
    void CreateGraphicPipelineLayout();
    void CreateGraphicPipeline();

    void CreateComputePipelineLayout();
    void CreateComputePipeline();

    void CreateFramebuffers();

private:
    void GetRequestInstaceExts(std::vector<const char*>& exts);
    void GetRequestInstanceLayers(std::vector<const char*>& layers);
    void MakeMessengerInfo(VkDebugUtilsMessengerCreateInfoEXT& createinfo);
    QueuefamliyIndices GetPhysicalDeviceQueueFamilyIndices(VkPhysicalDevice pdevice);
    bool IsPhysicalDeviceSuitable(VkPhysicalDevice pdevice);
    void GetRequestDeviceExts(std::vector<const char*>& exts);
    void GetRequestDeviceFeature(VkPhysicalDeviceFeatures& features);
    SurfaceDetails GetSurfaceDetails();
    VkSurfaceFormatKHR ChooseSwapChainImageFormat(std::vector<VkSurfaceFormatKHR>& formats);
    VkPresentModeKHR ChooseSwapChainImagePresentMode(std::vector<VkPresentModeKHR>& presentmodes);
    VkExtent2D ChooseSwapChainImageExtents(VkSurfaceCapabilitiesKHR& capabilities);
    
    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,VkDebugUtilsMessageTypeFlagsEXT messageTypes,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,void* pUserData);
    static void  WindowResizeCallback(GLFWwindow* window,int width,int height);
    VkShaderModule MakeShaderModule(const char* filename);

    VkCommandBuffer CreateCommandBuffer();
    void SubmitCommandBuffer(VkCommandBuffer cb,VkSubmitInfo submitinfom,VkFence fence);

    void CreateBuffer(VkBuffer& buffer,VkDeviceMemory& memory,VkDeviceSize size,VkBufferUsageFlags usage,VkMemoryPropertyFlags memproperties);
    uint32_t ChooseMemoryType(uint32_t typefilter,VkMemoryPropertyFlags properties);
    void CleanupBuffer(VkBuffer& buffer,VkDeviceMemory& memory,bool mapped);
    void CreateImage(VkImage &image, VkDeviceMemory &memory,VkExtent3D extent,VkFormat format, VkImageUsageFlags usage,VkSampleCountFlagBits samplecount);
    void ImageLayoutTransition(VkImage& image,VkImageLayout oldlayout,VkImageLayout newlayout,VkImageAspectFlags asepct);
private:
    VkImageView CreateImageView(VkImage image,VkFormat format,VkImageAspectFlags aspectMask);
private:
    GLFWwindow* Window = nullptr;
    VkInstance Instance;
    VkDebugUtilsMessengerEXT Messenger;
    VkSurfaceKHR Surface;

    VkPhysicalDevice PDevice;
    VkDevice LDevice;
    VkQueue GraphicNComputeQueue;
    VkQueue PresentQueue;

    VkCommandPool CommandPool;
    VkSwapchainKHR SwapChain;
    VkFormat SwapChainImageFormat;
    VkExtent2D SwapChainImageExtent;
    std::vector<VkImage> SwapChainImages;
    std::vector<VkImageView> SwapChainImageViews;
    std::vector<VkFramebuffer> SwapChainFramebuffers;

    VkDescriptorPool DescriptorPool;

    VkRenderPass GraphicRenderPass;
    VkDescriptorSetLayout GraphicDescriptorSetLayout;
    VkDescriptorSet GraphicDescriptorSet;
    VkPipelineLayout GraphicPipelineLayout;
    VkPipeline GraphicPipeline;

    VkDescriptorSetLayout ComputeDescriptorSetLayout;
    std::vector<VkDescriptorSet> ComputeDescriptorSet;
    VkPipelineLayout ComputePipelineLayout;
    VkPipeline ComputePipeline;

    std::vector<VkFence> InFlightFences;
    std::vector<VkSemaphore> ImageAvaliable;
    std::vector<VkSemaphore> RenderingFinish;

    VkBuffer VertexBuffer;
    VkDeviceMemory VertexBufferMemory;
    VkBuffer IndexBuffer;
    VkDeviceMemory IndexBufferMemory;

    VkBuffer UnifromMVPBuffer;
    VkDeviceMemory UnfiromMVPBufferMemory;
    void* MappedMVPBuffer;
    VkBuffer UniformComputeBuffer;
    VkDeviceMemory UniformComputeBufferMemory;
    void* MappedComputeBuffer;

    VkImage TextureImage;
    VkDeviceMemory TextureImageMemory;
    VkImageView TextureImageView;
    VkSampler TextureSampler;

    std::vector<VkImage> DepthImage;
    std::vector<VkDeviceMemory> DepthImageMemory;
    std::vector<VkImageView> DepthImageView;

    VkSampleCountFlagBits MSAASampleCount;

    std::vector<VkImage> MSAAImages;
    std::vector<VkDeviceMemory> MSAAImageMemory;
    std::vector<VkImageView> MSAAImageView;

    std::vector<VkBuffer> ParticleBuffers;
    std::vector<VkDeviceMemory> ParticleBufferMemory;

public:
    void SetVertices(const std::vector<Vertex>& vs, const std::vector<uint32_t>& is);
    void SetMVP(glm::mat4& model,glm::mat4& view,glm::mat4& projection);
private:
    RendererFeaturesFlag FeatureFlag;
    uint32_t Width;
    uint32_t Height;
    std::vector<Vertex> vertices = {
        {{0.5,-0.5,0},{},{},{1,0}},
        {{-0.5,-0.5,0},{},{},{0,0}},
        {{-0.5,0.5,0},{},{},{0,1}},
        {{0.5,0.5,0},{},{},{1,1}}
    };
    std::vector<uint32_t> indexs ={
        0,1,2,0,2,3
    };
    UniformMVPObject mvp;
    UniformComputeObject computeobj;
    bool bEnableValidation = false;
    uint32_t CurrentFlight = 0;
    uint32_t MAXInFlightRendering = 2;
    std::string texturefile = "";    
    uint32_t ParticleCount = 10;

     bool bFramebufferResized = false;
};
#endif