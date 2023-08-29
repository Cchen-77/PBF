#include"renderer.h"
#include"extensionfuncs.h"
#include"helperfuncs.h"

#define STB_IMAGE_IMPLEMENTATION
#include"stb_image.h"

#include<iostream>
#include<cstring>
#include<cstdio>
#include<exception>
#include<set>
#include<array>
#include<algorithm>



#define Allocator nullptr

VKAPI_ATTR VkBool32 VKAPI_CALL Renderer::DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes, 
const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData)
{
    std::cerr<<pCallbackData->pMessage<<std::endl;
    return VK_FALSE;
}
void Renderer::WindowResizeCallback(GLFWwindow *window, int width, int height)
{
    auto renderer =  reinterpret_cast<Renderer*>(glfwGetWindowUserPointer(window));
    renderer->bFramebufferResized = true;
    
}
VkShaderModule Renderer::MakeShaderModule(const char *filename)
{
    std::vector<char> bytes;
    HelperFuncs::ReadFile(filename,bytes);
    VkShaderModuleCreateInfo createinfo{};
    createinfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createinfo.codeSize = bytes.size();
    createinfo.pCode = reinterpret_cast<uint32_t*>(bytes.data());
    VkShaderModule module;
    if(vkCreateShaderModule(LDevice,&createinfo,Allocator,&module)!=VK_SUCCESS){
        throw std::runtime_error("failed to create shader module!");
    }
    return module;
   
}
VkCommandBuffer Renderer::CreateCommandBuffer()
{
    VkCommandBuffer cb;
    VkCommandBufferAllocateInfo allocateinfo{};
    allocateinfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateinfo.commandPool = CommandPool;
    allocateinfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateinfo.commandBufferCount = 1;
    if(vkAllocateCommandBuffers(LDevice,&allocateinfo,&cb)!=VK_SUCCESS){
        throw std::runtime_error("failed to allocate command buffer!");
    }
    VkCommandBufferBeginInfo begininfo{};
    begininfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begininfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if(vkBeginCommandBuffer(cb,&begininfo)!=VK_SUCCESS){
        throw std::runtime_error("failed to begin command buffer!");
    }
    return cb;
}
void Renderer::SubmitCommandBuffer(VkCommandBuffer cb,VkSubmitInfo submitinfo,VkFence fence)
{
    submitinfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitinfo.commandBufferCount = 1;
    submitinfo.pCommandBuffers = &cb;
    if(vkEndCommandBuffer(cb)!=VK_SUCCESS){
        throw std::runtime_error("failed to end command buffer!");
    }
    if(vkQueueSubmit(GraphicNComputeQueue,1,&submitinfo,fence)!=VK_SUCCESS){
        throw std::runtime_error("failed to submit command buffer!");
    }
    vkQueueWaitIdle(GraphicNComputeQueue);
    vkFreeCommandBuffers(LDevice,CommandPool,1,&cb);
}
void Renderer::CreateBuffer(VkBuffer &buffer, VkDeviceMemory &memory,VkDeviceSize size,VkBufferUsageFlags usage,VkMemoryPropertyFlags mempropperties)
{
    VkBufferCreateInfo createinfo{};
    createinfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    createinfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createinfo.size = size;
    createinfo.usage = usage;
    if(vkCreateBuffer(LDevice,&createinfo,Allocator,&buffer)!=VK_SUCCESS){
        throw std::runtime_error("failed to create buffer!");
    }
    VkMemoryRequirements requirements{};
    vkGetBufferMemoryRequirements(LDevice,buffer,&requirements);
    VkMemoryAllocateInfo allocateinfo{};
    allocateinfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateinfo.allocationSize = requirements.size;
    allocateinfo.memoryTypeIndex = ChooseMemoryType(requirements.memoryTypeBits,mempropperties);
    
    if(vkAllocateMemory(LDevice,&allocateinfo,Allocator,&memory)!=VK_SUCCESS){
        throw std::runtime_error("failed to allocate memory for buffer!");
    }
    vkBindBufferMemory(LDevice,buffer,memory,0);
}
uint32_t Renderer::ChooseMemoryType(uint32_t typefilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memproperties{};
    vkGetPhysicalDeviceMemoryProperties(PDevice,&memproperties);
    for(uint32_t i=0;i<memproperties.memoryTypeCount;++i){
        if(!(typefilter&(1<<i))) continue;
        if((properties&memproperties.memoryTypes[i].propertyFlags) == properties){
            return i;
        }
    }
    throw std::runtime_error("failed to choose a suitable memory type!");
}
void Renderer::CleanupBuffer(VkBuffer &buffer, VkDeviceMemory &memory,bool mapped)
{
    if(mapped)
        vkUnmapMemory(LDevice,memory);
    vkDestroyBuffer(LDevice,buffer,Allocator);
    vkFreeMemory(LDevice,memory,Allocator);
}
void Renderer::CreateImage(VkImage &image, VkDeviceMemory &memory,VkExtent3D extent,VkFormat format,
                           VkImageUsageFlags usage,VkSampleCountFlagBits samplecount)
{
    VkImageCreateInfo createinfo{};
    createinfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    createinfo.arrayLayers = 1;
    createinfo.extent = extent;
    createinfo.format = format;
    createinfo.imageType = VK_IMAGE_TYPE_2D;
    createinfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    createinfo.mipLevels = 1;
    createinfo.samples = samplecount;
    createinfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createinfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    createinfo.usage = usage;

    if(vkCreateImage(LDevice,&createinfo,Allocator,&image)!=VK_SUCCESS){
        throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements memrequirement{};
    vkGetImageMemoryRequirements(LDevice,image,&memrequirement);
    VkMemoryAllocateInfo allocateinfo{};
    allocateinfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateinfo.allocationSize = memrequirement.size;
    allocateinfo.memoryTypeIndex = ChooseMemoryType(memrequirement.memoryTypeBits,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if(vkAllocateMemory(LDevice,&allocateinfo,Allocator,&memory)!=VK_SUCCESS){
        throw std::runtime_error("failed to allcate memory for image!");
    }

    vkBindImageMemory(LDevice,image,memory,0);

}
void Renderer::ImageLayoutTransition(VkImage& image,VkImageLayout oldlayout,VkImageLayout newlayout,VkImageAspectFlags asepct)
{
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = image;
    barrier.newLayout = newlayout;
    barrier.oldLayout = oldlayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.aspectMask = asepct;
    VkPipelineStageFlags srcstage;
    VkPipelineStageFlags dststage;
    if(oldlayout == VK_IMAGE_LAYOUT_UNDEFINED && newlayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL){
        srcstage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        barrier.srcAccessMask = 0;
        dststage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    }
    else if(oldlayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newlayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL){
        srcstage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        dststage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    }
    else if(oldlayout == VK_IMAGE_LAYOUT_UNDEFINED && newlayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL){
        srcstage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        barrier.srcAccessMask = 0;
        dststage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }
    else if(oldlayout == VK_IMAGE_LAYOUT_UNDEFINED && newlayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL){
        srcstage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        barrier.srcAccessMask = 0;
        dststage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT|VK_ACCESS_SHADER_READ_BIT;
    }
    else{
        throw std::runtime_error("bad oldlayout to newlayout!");
    }   
    auto cb = CreateCommandBuffer();
    vkCmdPipelineBarrier(cb,srcstage,dststage,0,0,nullptr,0,nullptr,1,&barrier);
    VkSubmitInfo submitinfo{};
    SubmitCommandBuffer(cb,submitinfo,VK_NULL_HANDLE);
}
VkImageView Renderer::CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectMask)
{
    VkImageViewCreateInfo createinfo{};
    createinfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createinfo.format = format;
    createinfo.image = image;
    createinfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createinfo.subresourceRange.aspectMask = aspectMask;
    createinfo.subresourceRange.baseArrayLayer = 0;
    createinfo.subresourceRange.baseMipLevel = 0;
    createinfo.subresourceRange.layerCount = 1;
    createinfo.subresourceRange.levelCount = 1;
    VkImageView imageview;
    if(vkCreateImageView(LDevice,&createinfo,Allocator,&imageview)!=VK_SUCCESS){
        throw std::runtime_error("failed to create image view!");
    }
    return imageview;
}
void Renderer::SetMVP(glm::mat4 &model, glm::mat4 &view, glm::mat4 &projection)
{
    vkDeviceWaitIdle(LDevice);
    mvp.model = model;
    mvp.view = view;
    mvp.projection = projection;
    memcpy(MappedMVPBuffer,&mvp,sizeof(mvp));
}
Renderer::Renderer(uint32_t w, uint32_t h, RendererFeaturesFlag feature,bool validation,std::string texfile)
{
    Width = w;
    Height = h;   
    FeatureFlag = feature;
    bEnableValidation = validation;
    texturefile = texfile;
    Init();
}
Renderer::~Renderer()
{
    Cleanup();
}
void Renderer::Init()
{
    glfwInit();
    glfwWindowHint(GLFW_RESIZABLE,GLFW_TRUE);
    glfwWindowHint(GLFW_CLIENT_API,GLFW_NO_API);
    Window = glfwCreateWindow(Width,Height,"jason's renderer",nullptr,nullptr);
    glfwSetWindowUserPointer(Window,this);
    glfwSetFramebufferSizeCallback(Window,&Renderer::WindowResizeCallback);
    CreateInstance();
    CreateDebugMessenger();
    CreateSurface();
    PickPhysicalDevice();
    CreateLogicalDevice();

    GetMSAASampleCount();
    CreateSupportObjects();
    CreateCommandPool();
    CreateVertexBuffer();
    CreateIndexBuffer();
    

    if(FeatureFlag&RF_PARTICLE){
        CreateParticleBuffer();
        CreateUniformComputeBuffer();
    }
 

    CreateUniformMVPBuffer();
    CreateTextureResources();
    CreateSwapChain();
    CreateDepthResources();
    CreateMSAAResources();

    CreateDescriptorSetLayout();
    CreateDescriptorPool();
    CreateDescriptorSet();
    CreateRenderPass();
    CreateGraphicPipelineLayout();
    CreateGraphicPipeline();
  
    if(FeatureFlag&RF_PARTICLE){
        CreateComputePipelineLayout();
        CreateComputePipeline();
    }
   
    CreateFramebuffers();
}
void Renderer::Cleanup()
{
    vkDeviceWaitIdle(LDevice);
    if(FeatureFlag&RF_PARTICLE){
        vkDestroyPipeline(LDevice,ComputePipeline,Allocator);
        vkDestroyPipelineLayout(LDevice,ComputePipelineLayout,Allocator);
    }
    vkDestroyPipeline(LDevice,GraphicPipeline,Allocator);
    vkDestroyPipelineLayout(LDevice,GraphicPipelineLayout,Allocator);
    vkDestroyRenderPass(LDevice,GraphicRenderPass,Allocator);

    vkDestroyDescriptorPool(LDevice,DescriptorPool,Allocator);
    vkDestroyDescriptorSetLayout(LDevice,GraphicDescriptorSetLayout,Allocator); 
    //========================================================
    if(FeatureFlag&RF_PARTICLE){
        vkDestroyDescriptorSetLayout(LDevice,ComputeDescriptorSetLayout,Allocator);
    }
    //========================================================
     for(uint32_t i=0;i<MSAAImages.size();++i){
        vkDestroyImageView(LDevice,MSAAImageView[i],Allocator);
        vkDestroyImage(LDevice,MSAAImages[i],Allocator);
        vkFreeMemory(LDevice,MSAAImageMemory[i],Allocator);
    }
    for(uint32_t i=0;i<DepthImage.size();++i){
        vkDestroyImageView(LDevice,DepthImageView[i],Allocator);
        vkDestroyImage(LDevice,DepthImage[i],Allocator);
        vkFreeMemory(LDevice,DepthImageMemory[i],Allocator);
    }
    
    CleanupSwapChain();
    //========================================================
    if(FeatureFlag&RF_TEXTRUE){
        vkDestroySampler(LDevice,TextureSampler,Allocator);
        vkDestroyImageView(LDevice,TextureImageView,Allocator);
        vkDestroyImage(LDevice,TextureImage,Allocator);
        vkFreeMemory(LDevice,TextureImageMemory,Allocator);
    }
    //========================================================
    //========================================================
    if(FeatureFlag&RF_PARTICLE){
        for(uint32_t i=0;i<MAXInFlightRendering;++i){
            CleanupBuffer(ParticleBuffers[i],ParticleBufferMemory[i],false);
        }
        CleanupBuffer(UniformComputeBuffer,UniformComputeBufferMemory,true);
    }
    //========================================================

    CleanupBuffer(UnifromMVPBuffer,UnfiromMVPBufferMemory,true);
    CleanupBuffer(IndexBuffer,IndexBufferMemory,false);
    CleanupBuffer(VertexBuffer,VertexBufferMemory,false);
    vkDestroyCommandPool(LDevice,CommandPool,Allocator);
    CleanupSupportObjects();

    vkDestroyDevice(LDevice,Allocator);
    vkDestroySurfaceKHR(Instance,Surface,Allocator);
    ExtensionFuncs::DestroyDebugUtilsMessengerEXT(Instance,Messenger,Allocator);
    vkDestroyInstance(Instance,Allocator);

    glfwDestroyWindow(Window);
    glfwTerminate();
}
void Renderer::CreateInstance()
{
    VkApplicationInfo appinfo{};
    appinfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appinfo.pApplicationName = "Jason's Renderer";
    
    VkInstanceCreateInfo createinfo{};
    createinfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    std::vector<const char*> exts;
    std::vector<const char*> layers;
    GetRequestInstaceExts(exts);
    GetRequestInstanceLayers(layers);
    createinfo.enabledExtensionCount = static_cast<uint32_t>(exts.size());
    createinfo.ppEnabledExtensionNames = exts.data();
    createinfo.enabledLayerCount = static_cast<uint32_t>(layers.size());
    createinfo.ppEnabledLayerNames = layers.data();
    createinfo.pApplicationInfo = &appinfo;
    VkDebugUtilsMessengerCreateInfoEXT messengerinfo{};
    MakeMessengerInfo(messengerinfo);
    createinfo.pNext = &messengerinfo;
    if(vkCreateInstance(&createinfo,Allocator,&Instance)!=VK_SUCCESS){
        throw std::runtime_error("failed to create instance!");
    }

}
void Renderer::GetMSAASampleCount()
{
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(PDevice,&properties);

    auto samplecounts = properties.limits.sampledImageColorSampleCounts;
    if(samplecounts&VK_SAMPLE_COUNT_32_BIT){
        MSAASampleCount = VK_SAMPLE_COUNT_32_BIT;
        return;
    }
     if(samplecounts&VK_SAMPLE_COUNT_16_BIT){
        MSAASampleCount = VK_SAMPLE_COUNT_16_BIT;
        return;
    }
     if(samplecounts&VK_SAMPLE_COUNT_8_BIT){
        MSAASampleCount = VK_SAMPLE_COUNT_8_BIT;
        return;
    }
     if(samplecounts&VK_SAMPLE_COUNT_4_BIT){
        MSAASampleCount = VK_SAMPLE_COUNT_4_BIT;
        return;
    }
     if(samplecounts&VK_SAMPLE_COUNT_2_BIT){
        MSAASampleCount = VK_SAMPLE_COUNT_2_BIT;
        return;
    }
    MSAASampleCount = VK_SAMPLE_COUNT_1_BIT;
    return;
}
void Renderer::CreateSupportObjects()
{
    RenderingFinish.resize(MAXInFlightRendering);
    ImageAvaliable.resize(MAXInFlightRendering);
    InFlightFences.resize(MAXInFlightRendering);
    for(uint32_t i=0;i<MAXInFlightRendering;++i){
        VkSemaphoreCreateInfo seminfo{};
        seminfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        seminfo.flags = VK_SEMAPHORE_TYPE_BINARY;
        if(vkCreateSemaphore(LDevice,&seminfo,Allocator,&ImageAvaliable[i])!=VK_SUCCESS){
            throw std::runtime_error("failed to create sem:imageavaliable!");
        }
        if(vkCreateSemaphore(LDevice,&seminfo,Allocator,&RenderingFinish[i])!=VK_SUCCESS){
            throw std::runtime_error("failed to create sem:renderingfinish!");
        }
        VkFenceCreateInfo fenceinfo{};
        fenceinfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceinfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        if(vkCreateFence(LDevice,&fenceinfo,Allocator,&InFlightFences[i])!=VK_SUCCESS){
            throw std::runtime_error("failed to create fence:inflight!");
        }

    }
}
void Renderer::CleanupSupportObjects()
{
    for(uint32_t i=0;i<MAXInFlightRendering;++i){
        vkDestroySemaphore(LDevice,ImageAvaliable[i],Allocator);
        vkDestroySemaphore(LDevice,RenderingFinish[i],Allocator);
        vkDestroyFence(LDevice,InFlightFences[i],Allocator);
    }
}
void Renderer::CreateDebugMessenger()
{
    VkDebugUtilsMessengerCreateInfoEXT createinfo{};
    MakeMessengerInfo(createinfo);
    if(ExtensionFuncs::CreateDebugUtilsMessengerEXT(Instance,&createinfo,Allocator,&Messenger)!=VK_SUCCESS){
        throw std::runtime_error("failed to create debug messenger!");
    }
}
void Renderer::CreateSurface()
{
    if(glfwCreateWindowSurface(Instance,Window,Allocator,&Surface)!=VK_SUCCESS){
        throw std::runtime_error("faile to create surface!");
    }
}
void Renderer::PickPhysicalDevice()
{
    uint32_t pdevice_count;
    vkEnumeratePhysicalDevices(Instance,&pdevice_count,nullptr);
    std::vector<VkPhysicalDevice> pdeives(pdevice_count);
    vkEnumeratePhysicalDevices(Instance,&pdevice_count,pdeives.data());
    for(auto& pdevice:pdeives){
        if(IsPhysicalDeviceSuitable(pdevice)){
            PDevice = pdevice;
            return;
        }
    }
    throw std::runtime_error("failed to find a suitable physical device!");

}
void Renderer::CreateLogicalDevice()
{
    auto queueindices = GetPhysicalDeviceQueueFamilyIndices(PDevice);
    std::array<uint32_t,2> indices = {queueindices.graphicNcompute.value(),queueindices.present.value()};
    std::sort(indices.begin(),indices.end());
    std::vector<VkDeviceQueueCreateInfo> queueinfos;
    float priority = 1.0f;
    for(int i=0;i<indices.size();++i){
        if(i>0&&indices[i] == indices[i-1]) continue;
        VkDeviceQueueCreateInfo queueinfo{};
        queueinfo = VkDeviceQueueCreateInfo{};
        queueinfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueinfo.pQueuePriorities = &priority;
        queueinfo.queueCount = 1;
        queueinfo.queueFamilyIndex = indices[i];
        queueinfos.push_back(queueinfo);
    }
    VkDeviceCreateInfo createinfo{};
    createinfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    std::vector<const char*> exts;
    GetRequestDeviceExts(exts);
    createinfo.enabledExtensionCount = static_cast<uint32_t>(exts.size());
    createinfo.ppEnabledExtensionNames = exts.data();
    VkPhysicalDeviceFeatures features;
    GetRequestDeviceFeature(features);
    createinfo.pEnabledFeatures = &features;
    createinfo.queueCreateInfoCount = static_cast<uint32_t>(queueinfos.size());
    createinfo.pQueueCreateInfos = queueinfos.data();
    if(vkCreateDevice(PDevice,&createinfo,Allocator,&LDevice)!=VK_SUCCESS){
        throw std::runtime_error("failed to create logical device!");
    }
    vkGetDeviceQueue(LDevice,queueindices.graphicNcompute.value(),0,&GraphicNComputeQueue);
    vkGetDeviceQueue(LDevice,queueindices.present.value(),0,&PresentQueue);
}
void Renderer::CreateCommandPool()
{
    auto queueindices = GetPhysicalDeviceQueueFamilyIndices(PDevice);
    VkCommandPoolCreateInfo createinfo{};
    createinfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    createinfo.queueFamilyIndex = queueindices.graphicNcompute.value();
    createinfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    if(vkCreateCommandPool(LDevice,&createinfo,Allocator,&CommandPool)!=VK_SUCCESS){
        throw std::runtime_error("failed to create command pool!");
    }
}
void Renderer::CreateVertexBuffer()
{
    VkDeviceSize size = sizeof(Vertex)*vertices.size();
    VkBuffer stagingbuffer;
    VkDeviceMemory stagingmemory;
    CreateBuffer(stagingbuffer,stagingmemory,size,VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    CreateBuffer(VertexBuffer,VertexBufferMemory,size,VK_BUFFER_USAGE_VERTEX_BUFFER_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    void* data;
    vkMapMemory(LDevice,stagingmemory,0,size,0,&data);
    memcpy(data,vertices.data(),size);
    auto cb = CreateCommandBuffer();
    VkBufferCopy region{};
    region.srcOffset = 0;
    region.dstOffset = 0;
    region.size = size;
    vkCmdCopyBuffer(cb,stagingbuffer,VertexBuffer,1,&region);
    VkSubmitInfo submitinfo{};
    SubmitCommandBuffer(cb,submitinfo,VK_NULL_HANDLE);
    CleanupBuffer(stagingbuffer,stagingmemory,true);
}
void Renderer::CreateIndexBuffer()
{
    VkDeviceSize size = sizeof(uint32_t)*indexs.size();
    VkBuffer stagingbuffer;
    VkDeviceMemory stagingmemory;
    CreateBuffer(stagingbuffer,stagingmemory,size,VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    CreateBuffer(IndexBuffer,IndexBufferMemory,size,VK_BUFFER_USAGE_INDEX_BUFFER_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    void* data;
    vkMapMemory(LDevice,stagingmemory,0,size,0,&data);
    memcpy(data,indexs.data(),size);
    auto cb = CreateCommandBuffer();
    VkBufferCopy region{};
    region.srcOffset = 0;
    region.dstOffset = 0;
    region.size = size;
    vkCmdCopyBuffer(cb,stagingbuffer,IndexBuffer,1,&region);
    VkSubmitInfo submitinfo{};
    SubmitCommandBuffer(cb,submitinfo,VK_NULL_HANDLE);
    CleanupBuffer(stagingbuffer,stagingmemory,true);
}

void Renderer::CreateParticleBuffer()
{
    std::vector<Particle> InitParticle(ParticleCount);
    //========================================================
    //              TODO:INIT THE PARTICLE
    //========================================================

    ParticleBufferMemory.resize(MAXInFlightRendering);
    ParticleBuffers.resize(MAXInFlightRendering);
    VkDeviceSize size = ParticleCount*sizeof(Particle);
    VkBuffer stagingbuffer;
    VkDeviceMemory stagingmemory;
    CreateBuffer(stagingbuffer,stagingmemory,size,VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    void* data;
    vkMapMemory(LDevice,stagingmemory,0,size,0,&data);
    memcpy(data,indexs.data(),size);
    VkBufferCopy region{};
    region.srcOffset = 0;
    region.dstOffset = 0;
    region.size = size;
    auto cb = CreateCommandBuffer();
    for(uint32_t i=0;i<MAXInFlightRendering;++i){
        CreateBuffer(ParticleBuffers[i],ParticleBufferMemory[i],size,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT|VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        vkCmdCopyBuffer(cb,stagingbuffer,ParticleBuffers[i],1,&region);
    }
    VkSubmitInfo submitinfo{};
    SubmitCommandBuffer(cb,submitinfo,VK_NULL_HANDLE);
    CleanupBuffer(stagingbuffer,stagingmemory,true);
}

void Renderer::CreateUniformMVPBuffer()
{
    VkDeviceSize size = sizeof(UniformMVPObject);
    CreateBuffer(UnifromMVPBuffer,UnfiromMVPBufferMemory,size,VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT|VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    vkMapMemory(LDevice,UnfiromMVPBufferMemory,0,size,0,&MappedMVPBuffer);
    memcpy(MappedMVPBuffer,&mvp,size);
}

void Renderer::CreateUniformComputeBuffer()
{
    VkDeviceSize size = sizeof(UniformComputeObject);
    CreateBuffer(UniformComputeBuffer,UniformComputeBufferMemory,size,VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT|VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    vkMapMemory(LDevice,UniformComputeBufferMemory,0,size,0,&MappedComputeBuffer);
    memcpy(MappedComputeBuffer,&mvp,size);
}

void Renderer::CreateTextureResources()
{
    if(!(FeatureFlag&RF_TEXTRUE)) return;
    uint32_t w,h,channel;
    stbi_uc* pixels = stbi_load(texturefile.c_str(),reinterpret_cast<int*>(&w),reinterpret_cast<int*>(&h),
    reinterpret_cast<int*>(&channel),STBI_rgb_alpha);
    VkBuffer stagingbuffer;
    VkDeviceMemory stagingmemory;
    VkDeviceSize size = w*h*4;
    CreateBuffer(stagingbuffer,stagingmemory,size,VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    void* data;
    vkMapMemory(LDevice,stagingmemory,0,size,0,&data);
    memcpy(data,pixels,size);
    STBI_FREE(pixels);
    CreateImage(TextureImage,TextureImageMemory,{w,h,1},VK_FORMAT_R8G8B8A8_SRGB,VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_SAMPLED_BIT,VK_SAMPLE_COUNT_1_BIT);
    VkBufferImageCopy region{};
    region.bufferImageHeight = 0;
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.imageExtent.width = w;
    region.imageExtent.height = h;
    region.imageExtent.depth = 1;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageSubresource.mipLevel = 0;
  
    ImageLayoutTransition(TextureImage,VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,VK_IMAGE_ASPECT_COLOR_BIT);  
    auto cb  = CreateCommandBuffer();
    vkCmdCopyBufferToImage(cb,stagingbuffer,TextureImage,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&region); 
    VkSubmitInfo submitinfo{};
    SubmitCommandBuffer(cb,submitinfo,VK_NULL_HANDLE);
    CleanupBuffer(stagingbuffer,stagingmemory,true);
    ImageLayoutTransition(TextureImage,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,VK_IMAGE_ASPECT_COLOR_BIT); 
    TextureImageView = CreateImageView(TextureImage,VK_FORMAT_R8G8B8A8_SRGB,VK_IMAGE_ASPECT_COLOR_BIT);

    VkSamplerCreateInfo sampelerinfo{};
    sampelerinfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampelerinfo.anisotropyEnable = VK_TRUE;
    sampelerinfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    sampelerinfo.magFilter = VK_FILTER_LINEAR;
    sampelerinfo.minFilter = VK_FILTER_LINEAR;
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(PDevice,&properties);
    sampelerinfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;

    if(vkCreateSampler(LDevice,&sampelerinfo,Allocator,&TextureSampler)!=VK_SUCCESS){
        throw std::runtime_error("failed to create texture sampler!");
    }
}

void Renderer::CreateDepthResources()
{
    DepthImage.resize(SwapChainImages.size());
    DepthImageMemory.resize(SwapChainImages.size());
    DepthImageView.resize(SwapChainImages.size());
    VkExtent3D extent = {SwapChainImageExtent.width,SwapChainImageExtent.height,1};
    for(uint32_t i=0;i<DepthImage.size();++i){
        CreateImage(DepthImage[i],DepthImageMemory[i],extent,VK_FORMAT_D32_SFLOAT,VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,MSAASampleCount);
        DepthImageView[i] = CreateImageView(DepthImage[i],VK_FORMAT_D32_SFLOAT,VK_IMAGE_ASPECT_DEPTH_BIT);
        ImageLayoutTransition(DepthImage[i],VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,VK_IMAGE_ASPECT_DEPTH_BIT);
    }
}

void Renderer::CreateMSAAResources()
{
    MSAAImages.resize(SwapChainImages.size());
    MSAAImageMemory.resize(SwapChainImages.size());
    MSAAImageView.resize(SwapChainImages.size());
    VkExtent3D extent = {SwapChainImageExtent.width,SwapChainImageExtent.height,1};
    for(uint32_t i=0;i<MSAAImages.size();++i){
        CreateImage(MSAAImages[i],MSAAImageMemory[i],extent,SwapChainImageFormat,VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,MSAASampleCount);
        MSAAImageView[i] = CreateImageView(MSAAImages[i],SwapChainImageFormat,VK_IMAGE_ASPECT_COLOR_BIT);
        ImageLayoutTransition(MSAAImages[i],VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,VK_IMAGE_ASPECT_COLOR_BIT);
    }
}

void Renderer::CreateSwapChain()
{
    auto surfacedetails = GetSurfaceDetails();
    auto format = ChooseSwapChainImageFormat(surfacedetails.formats);
    auto presentmode = ChooseSwapChainImagePresentMode(surfacedetails.presentmode);
    auto extent = ChooseSwapChainImageExtents(surfacedetails.capabilities);
    VkSwapchainCreateInfoKHR createinfo{};
    createinfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createinfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    createinfo.surface = Surface;
    createinfo.clipped = VK_TRUE;
    createinfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createinfo.imageArrayLayers = 1;
    createinfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createinfo.imageColorSpace = format.colorSpace;
    createinfo.imageExtent = extent;
    createinfo.imageFormat = format.format;
    std::vector<uint32_t> indices;
    if(GraphicNComputeQueue == PresentQueue){
        createinfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    else{
        auto queueindices = GetPhysicalDeviceQueueFamilyIndices(PDevice);
        createinfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createinfo.queueFamilyIndexCount = 2;
        indices.push_back(queueindices.graphicNcompute.value());
        indices.push_back(queueindices.present.value());
        createinfo.pQueueFamilyIndices = indices.data();
    }
    createinfo.minImageCount = surfacedetails.capabilities.minImageCount+1;
    if(createinfo.minImageCount > surfacedetails.capabilities.maxImageCount){
        createinfo.minImageCount = surfacedetails.capabilities.maxImageCount;
    }
    if(vkCreateSwapchainKHR(LDevice,&createinfo,Allocator,&SwapChain)!=VK_SUCCESS){
        throw std::runtime_error("failed to create swapchain!");
    }
    SwapChainImageFormat = format.format;
    SwapChainImageExtent = extent;
    uint32_t image_count;
    vkGetSwapchainImagesKHR(LDevice,SwapChain,&image_count,nullptr);
    SwapChainImages.resize(image_count);
    SwapChainImageViews.resize(image_count);
    vkGetSwapchainImagesKHR(LDevice,SwapChain,&image_count,SwapChainImages.data());
    for(uint32_t i=0;i<image_count;++i){
        SwapChainImageViews[i] = CreateImageView(SwapChainImages[i],SwapChainImageFormat,VK_IMAGE_ASPECT_COLOR_BIT);
    }
}
void Renderer::CleanupSwapChain()
{
    for(auto& framebuffer:SwapChainFramebuffers){
        vkDestroyFramebuffer(LDevice,framebuffer,Allocator);
    }
    for(auto& imageview:SwapChainImageViews){
        vkDestroyImageView(LDevice,imageview,Allocator);
    }
    vkDestroySwapchainKHR(LDevice,SwapChain,Allocator);

}
void Renderer::RecreateSwapChain()
{
    vkDeviceWaitIdle(LDevice);
    for(uint32_t i=0;i<MSAAImages.size();++i){
        vkDestroyImageView(LDevice,MSAAImageView[i],Allocator);
        vkDestroyImage(LDevice,MSAAImages[i],Allocator);
        vkFreeMemory(LDevice,MSAAImageMemory[i],Allocator);
    }
    for(uint32_t i=0;i<DepthImage.size();++i){
        vkDestroyImageView(LDevice,DepthImageView[i],Allocator);
        vkDestroyImage(LDevice,DepthImage[i],Allocator);
        vkFreeMemory(LDevice,DepthImageMemory[i],Allocator);
    }
    CleanupSwapChain();
    CreateSwapChain();
    CreateDepthResources();
    CreateMSAAResources();
    CreateFramebuffers();
}
void Renderer::CreateDescriptorSetLayout()
{
    //========================================================
    //        Create Graphic Descriptor Set Layout
    //========================================================
    std::array<VkDescriptorSetLayoutBinding,2> gbindings{};
    gbindings[0].binding = 0;
    gbindings[0].descriptorCount = 1;
    gbindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    gbindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
   

    gbindings[1].binding = 1;
    gbindings[1].descriptorCount = 1;
    gbindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    gbindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo gcreateinfo{};
    gcreateinfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    gcreateinfo.bindingCount = static_cast<uint32_t>(gbindings.size());
    gcreateinfo.pBindings = gbindings.data();
    
    if(vkCreateDescriptorSetLayout(LDevice,&gcreateinfo,Allocator,&GraphicDescriptorSetLayout)!=VK_SUCCESS){
        throw std::runtime_error("failed to create graphic descriptor set layout!");
    }

    //========================================================
    //        Create Compute Descriptor Set Layout
    //========================================================
    if(!(FeatureFlag&RF_PARTICLE)) return;
    std::array<VkDescriptorSetLayoutBinding,3> cbindings{};
    cbindings[0].binding = 0;
    cbindings[0].descriptorCount = 1;
    cbindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    cbindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    cbindings[1].binding = 1;
    cbindings[1].descriptorCount = 1;
    cbindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    cbindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    cbindings[2].binding = 2;
    cbindings[2].descriptorCount = 1;
    cbindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    cbindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo ccreateinfo{};
    ccreateinfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    ccreateinfo.bindingCount = static_cast<uint32_t>(cbindings.size());
    ccreateinfo.pBindings = cbindings.data();
    if(vkCreateDescriptorSetLayout(LDevice,&ccreateinfo,Allocator,&ComputeDescriptorSetLayout)!=VK_SUCCESS){
        throw std::runtime_error("failed to create compute descriptor set layout!");
    }
    
        
}
void Renderer::CreateDescriptorPool()
{
    std::array<VkDescriptorPoolSize,3> poolsizes{};
    poolsizes[0].descriptorCount = 1+MAXInFlightRendering;
    poolsizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolsizes[1].descriptorCount = 1;
    poolsizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolsizes[2].descriptorCount = 2*MAXInFlightRendering;
    poolsizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

    VkDescriptorPoolCreateInfo createinfo{};
    createinfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    createinfo.maxSets = 2*MAXInFlightRendering;
    createinfo.poolSizeCount = static_cast<uint32_t>(poolsizes.size());
    createinfo.pPoolSizes = poolsizes.data();

    if(vkCreateDescriptorPool(LDevice,&createinfo,Allocator,&DescriptorPool)!=VK_SUCCESS){
        throw std::runtime_error("failed to create descriptor pool!");
    }
}
void Renderer::CreateDescriptorSet()
{

    //========================================================
    //        Create Graphic Descriptor Set 
    //========================================================
    VkDescriptorSetAllocateInfo gallocateinfo{};
    gallocateinfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    gallocateinfo.descriptorPool = DescriptorPool;
    gallocateinfo.descriptorSetCount = 1;
    gallocateinfo.pSetLayouts = &GraphicDescriptorSetLayout;
    if(vkAllocateDescriptorSets(LDevice,&gallocateinfo,&GraphicDescriptorSet)!=VK_SUCCESS){
        throw std::runtime_error("failed to allocate graphic descriptor set!");
    }
    std::vector<VkWriteDescriptorSet> gwrites;

    VkDescriptorBufferInfo mvpbufferinfo{};
    mvpbufferinfo.buffer = UnifromMVPBuffer;
    mvpbufferinfo.offset = 0;
    mvpbufferinfo.range = sizeof(UniformMVPObject);
    gwrites.push_back(VkWriteDescriptorSet{});
    gwrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    gwrites[0].descriptorCount = 1;
    gwrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    gwrites[0].dstArrayElement = 0;
    gwrites[0].dstBinding = 0;
    gwrites[0].dstSet = GraphicDescriptorSet;
    gwrites[0].pBufferInfo = &mvpbufferinfo;
    VkDescriptorImageInfo teximageinfo{};
    teximageinfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    teximageinfo.imageView = TextureImageView;
    teximageinfo.sampler = TextureSampler;
    if(FeatureFlag&RF_TEXTRUE){
        gwrites.push_back(VkWriteDescriptorSet{});
        gwrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        gwrites[1].descriptorCount = 1;
        gwrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        gwrites[1].dstArrayElement = 0;
        gwrites[1].dstBinding = 1;
        gwrites[1].dstSet = GraphicDescriptorSet;
        gwrites[1].pImageInfo = &teximageinfo;
    }
    vkUpdateDescriptorSets(LDevice,gwrites.size(),gwrites.data(),0,nullptr);

    
    //========================================================
    //        Create Compute Descriptor Set 
    //========================================================
    if(!(FeatureFlag&RF_PARTICLE)) return;
    ComputeDescriptorSet.resize(MAXInFlightRendering);
    VkDescriptorSetAllocateInfo callocateinfo{};
    callocateinfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    callocateinfo.descriptorPool = DescriptorPool;
    callocateinfo.descriptorSetCount = 1;
    callocateinfo.pSetLayouts = &ComputeDescriptorSetLayout;
    for(uint32_t i=0;i<MAXInFlightRendering;++i){
        if(vkAllocateDescriptorSets(LDevice,&callocateinfo,&ComputeDescriptorSet[i])!=VK_SUCCESS){
            throw std::runtime_error("failed to allocate compute descriptor set!");
        }
    }
    std::array<VkWriteDescriptorSet,3> cwrites{};
    cwrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    cwrites[0].descriptorCount = 1;
    cwrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    cwrites[0].dstArrayElement = 0;
    cwrites[0].dstBinding = 0;
        
    cwrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    cwrites[1].descriptorCount = 1;
    cwrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    cwrites[1].dstArrayElement = 0;
    cwrites[1].dstBinding = 1;

    cwrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    cwrites[2].descriptorCount = 1;
    cwrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    cwrites[2].dstArrayElement = 0;
    cwrites[2].dstBinding = 1;
    for(uint32_t i=0;i<MAXInFlightRendering;++i){
        VkDescriptorBufferInfo computeobjbufferinfo{};
        computeobjbufferinfo.buffer = UniformComputeBuffer;
        computeobjbufferinfo.offset = 0;
        computeobjbufferinfo.range = sizeof(UniformComputeObject);
        VkDescriptorBufferInfo particlebufferinfo_thisframe{};
        particlebufferinfo_thisframe.buffer = ParticleBuffers[i];
        particlebufferinfo_thisframe.offset = 0;
        particlebufferinfo_thisframe.range = sizeof(Particle)*ParticleCount;
        VkDescriptorBufferInfo particlebufferinfo_lastframe{};
        particlebufferinfo_lastframe.buffer = ParticleBuffers[(i-1)%MAXInFlightRendering];
        particlebufferinfo_lastframe.offset = 0;
        particlebufferinfo_lastframe.range = sizeof(Particle)*ParticleCount;
        
        cwrites[0].dstSet = ComputeDescriptorSet[i];
        cwrites[0].pBufferInfo = &computeobjbufferinfo;
        cwrites[1].dstSet = ComputeDescriptorSet[i];
        cwrites[1].pBufferInfo = &particlebufferinfo_thisframe;
        cwrites[2].dstSet = ComputeDescriptorSet[i];
        cwrites[2].pBufferInfo =  &particlebufferinfo_lastframe;

        vkUpdateDescriptorSets(LDevice,cwrites.size(),cwrites.data(),0,nullptr);
    }

}
void Renderer::CreateRenderPass()
{
    VkAttachmentDescription colorattachment{};
    colorattachment.format = SwapChainImageFormat;
    colorattachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorattachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorattachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorattachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorattachment.samples = MSAASampleCount;
    colorattachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorattachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    VkAttachmentDescription depthattachement{};
    depthattachement.format = VK_FORMAT_D32_SFLOAT;
    depthattachement.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depthattachement.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depthattachement.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthattachement.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthattachement.samples = MSAASampleCount;
    depthattachement.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthattachement.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    VkAttachmentDescription colorresolveattachemnt{};
    colorresolveattachemnt.format = SwapChainImageFormat;
    colorresolveattachemnt.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    colorresolveattachemnt.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorresolveattachemnt.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorresolveattachemnt.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorresolveattachemnt.samples = VK_SAMPLE_COUNT_1_BIT;
    colorresolveattachemnt.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorresolveattachemnt.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;


    std::array<VkAttachmentDescription,3> attachments = {colorattachment,depthattachement,colorresolveattachemnt};
    VkAttachmentReference colorattachment_ref{};
    colorattachment_ref.attachment = 0;
    colorattachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    VkAttachmentReference depthattachement_ref{};
    depthattachement_ref.attachment = 1;
    depthattachement_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    VkAttachmentReference resolveattachment_ref{};
    resolveattachment_ref.attachment = 2;
    resolveattachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    std::array<VkSubpassDescription,1> subpasses{};
    subpasses[0].colorAttachmentCount = 1;
    subpasses[0].pColorAttachments = &colorattachment_ref;
    subpasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpasses[0].pDepthStencilAttachment  = &depthattachement_ref;
    subpasses[0].pResolveAttachments = &resolveattachment_ref;
    
    VkRenderPassCreateInfo createinfo{};
    createinfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    createinfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    createinfo.pAttachments = attachments.data();
    createinfo.subpassCount = static_cast<uint32_t>(subpasses.size());
    createinfo.pSubpasses = subpasses.data();
    
    if(vkCreateRenderPass(LDevice,&createinfo,Allocator,&GraphicRenderPass)!=VK_SUCCESS){
        throw std::runtime_error("failed to create graphic renderpass!");
    }
}
void Renderer::CreateGraphicPipelineLayout()
{
    VkPipelineLayoutCreateInfo createinfo{};
    createinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    createinfo.pSetLayouts = &GraphicDescriptorSetLayout;
    createinfo.setLayoutCount = 1;
    if(vkCreatePipelineLayout(LDevice,&createinfo,Allocator,&GraphicPipelineLayout)!=VK_SUCCESS){
        throw std::runtime_error("failed to create graphic pipeline layout!");
    }
}
void Renderer::CreateGraphicPipeline()
{
    auto vertshadermodule = MakeShaderModule("shaders/spv/vertshader.spv");
    auto fragshadermodule = MakeShaderModule("shaders/spv/fragshader.spv");
    VkPipelineShaderStageCreateInfo vertshader{};
    vertshader.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertshader.module = vertshadermodule;
    vertshader.pName = "main";
    vertshader.stage = VK_SHADER_STAGE_VERTEX_BIT;
    VkPipelineShaderStageCreateInfo fragshader{};
    fragshader.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragshader.module = fragshadermodule;
    fragshader.pName = "main";
    fragshader.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    std::array<VkPipelineShaderStageCreateInfo,2> shaderstages = {vertshader,fragshader};

    std::array<VkDynamicState,2> dynamicstates = {VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicstate{};
    dynamicstate.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicstate.dynamicStateCount = static_cast<uint32_t>(dynamicstates.size());
    dynamicstate.pDynamicStates = dynamicstates.data();

    VkPipelineVertexInputStateCreateInfo vertexinput{};
    vertexinput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    auto vertexinputbinding = Vertex::GetBinding();
    auto vertexinputattributs = Vertex::GetAttributes();
    vertexinput.vertexBindingDescriptionCount = 1;
    vertexinput.pVertexBindingDescriptions = &vertexinputbinding;
    vertexinput.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexinputattributs.size());
    vertexinput.pVertexAttributeDescriptions = vertexinputattributs.data();
    
    VkPipelineInputAssemblyStateCreateInfo inputassmbly{};
    inputassmbly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputassmbly.primitiveRestartEnable = VK_FALSE;
    inputassmbly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    
    VkPipelineViewportStateCreateInfo viewport{};
    viewport.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport.viewportCount = 1;
    viewport.scissorCount = 1;
    
    VkPipelineMultisampleStateCreateInfo multisample{};
    multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample.rasterizationSamples = MSAASampleCount;

    VkPipelineRasterizationStateCreateInfo rasterization{};
    rasterization.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterization.depthClampEnable = VK_FALSE;
    rasterization.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterization.lineWidth = 1.0f;
    rasterization.polygonMode = VK_POLYGON_MODE_FILL;

    VkPipelineDepthStencilStateCreateInfo depthstencil{};
    depthstencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthstencil.depthTestEnable = VK_TRUE;
    depthstencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthstencil.depthWriteEnable = VK_TRUE;
    depthstencil.maxDepthBounds = 1.0f;
    depthstencil.minDepthBounds = 0.0f;
  
    VkPipelineColorBlendAttachmentState colorblendattachment{};
    colorblendattachment.blendEnable = VK_FALSE;
    colorblendattachment.colorWriteMask = VK_COLOR_COMPONENT_A_BIT|VK_COLOR_COMPONENT_R_BIT|
                                          VK_COLOR_COMPONENT_G_BIT|VK_COLOR_COMPONENT_B_BIT;
    VkPipelineColorBlendStateCreateInfo colorblend{};
    colorblend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorblend.attachmentCount = 1;
    colorblend.logicOpEnable = VK_FALSE;
    colorblend.pAttachments = &colorblendattachment;

   
    VkGraphicsPipelineCreateInfo createinfo{};
    createinfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    createinfo.layout = GraphicPipelineLayout;
    createinfo.pColorBlendState = &colorblend;
    createinfo.pDepthStencilState = &depthstencil;
    createinfo.pDynamicState = &dynamicstate;
    createinfo.pInputAssemblyState = &inputassmbly;
    createinfo.pMultisampleState = &multisample;
    createinfo.pRasterizationState = &rasterization;
    createinfo.pStages = shaderstages.data();
    createinfo.stageCount = static_cast<uint32_t>(shaderstages.size());
    createinfo.pVertexInputState = &vertexinput;
    createinfo.pViewportState = &viewport;
    createinfo.renderPass = GraphicRenderPass;
    createinfo.subpass = 0;
    
    if(vkCreateGraphicsPipelines(LDevice,VK_NULL_HANDLE,1,&createinfo,Allocator,&GraphicPipeline)!=VK_SUCCESS){
        throw std::runtime_error("failed to create graphic pipeline!");
    }   

    vkDestroyShaderModule(LDevice,vertshadermodule,Allocator);
    vkDestroyShaderModule(LDevice,fragshadermodule,Allocator);
}
void Renderer::CreateComputePipelineLayout()
{
    VkPipelineLayoutCreateInfo createinfo{};
    createinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    createinfo.pSetLayouts = &ComputeDescriptorSetLayout;
    createinfo.setLayoutCount = 1;
    if(vkCreatePipelineLayout(LDevice,&createinfo,Allocator,&ComputePipelineLayout)!=VK_SUCCESS){
        throw std::runtime_error("failed to create graphic pipeline layout!");
    }
}
void Renderer::CreateComputePipeline()
{
    auto computershadermodule = MakeShaderModule("shaders/spv/compshader.spv");
    VkPipelineShaderStageCreateInfo stageinfo{};
    stageinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageinfo.pName = "main";
    stageinfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stageinfo.module = computershadermodule;
    VkComputePipelineCreateInfo createinfo{};
    createinfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    createinfo.layout = ComputePipelineLayout;
    createinfo.stage = stageinfo;
    if(vkCreateComputePipelines(LDevice,VK_NULL_HANDLE,1,&createinfo,Allocator,&ComputePipeline)!=VK_SUCCESS){
        throw std::runtime_error("failed to create compute pipeline!");
    }
    vkDestroyShaderModule(LDevice,computershadermodule,Allocator);
    
}
void Renderer::CreateFramebuffers()
{
    SwapChainFramebuffers.resize(SwapChainImages.size());
    for(uint32_t i=0;i<SwapChainFramebuffers.size();++i){
        VkFramebufferCreateInfo framebufferinfo{};
        std::array<VkImageView,3> attachments = {MSAAImageView[i],DepthImageView[i],SwapChainImageViews[i]};
        framebufferinfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferinfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferinfo.pAttachments = attachments.data();
        int w,h;
        glfwGetFramebufferSize(Window,&w,&h);
        framebufferinfo.width = w;
        framebufferinfo.height = h;
        framebufferinfo.layers = 1;
        framebufferinfo.renderPass = GraphicRenderPass;
        
        if(vkCreateFramebuffer(LDevice,&framebufferinfo,Allocator,&SwapChainFramebuffers[i])!=VK_SUCCESS){
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}
void Renderer::GetRequestInstaceExts(std::vector<const char *> &exts)
{
    const char** glfwexts;
    uint32_t glfwext_count;
    glfwexts = glfwGetRequiredInstanceExtensions(&glfwext_count);
    exts.assign(glfwexts,glfwexts+glfwext_count);
    if(bEnableValidation){
        exts.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
}
void Renderer::GetRequestInstanceLayers(std::vector<const char *>& layers)
{
    layers.resize(0);
    if(bEnableValidation){
        layers.push_back("VK_LAYER_KHRONOS_validation");
    }
}
void Renderer::MakeMessengerInfo(VkDebugUtilsMessengerCreateInfoEXT &createinfo)
{
    createinfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createinfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT 
                            | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT
                            | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    createinfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT|
                                VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
    createinfo.pUserData = nullptr;
    createinfo.pfnUserCallback = Renderer::DebugCallback;
}
QueuefamliyIndices Renderer::GetPhysicalDeviceQueueFamilyIndices(VkPhysicalDevice pdevice)
{
    QueuefamliyIndices indices;
    uint32_t queuefamily_count;
    vkGetPhysicalDeviceQueueFamilyProperties(pdevice,&queuefamily_count,nullptr);
    std::vector<VkQueueFamilyProperties> queuefamilies(queuefamily_count);
    vkGetPhysicalDeviceQueueFamilyProperties(pdevice,&queuefamily_count,queuefamilies.data());
    for(uint32_t i=0;i<queuefamilies.size();++i){
        auto& qf = queuefamilies[i];
        if((qf.queueFlags&VK_QUEUE_GRAPHICS_BIT)&&(qf.queueFlags&VK_QUEUE_COMPUTE_BIT)){
            if(!indices.graphicNcompute.has_value()){
                indices.graphicNcompute = i;
            }
        }
        if(!indices.present.has_value()){
            VkBool32 surpport = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(pdevice,i,Surface,&surpport);
            if(surpport){
                indices.present = i;
            }
        }
        if(indices.IsCompleted()){
            break;
        }
    }
    return indices;
}
bool Renderer::IsPhysicalDeviceSuitable(VkPhysicalDevice pdevice)
{
    auto indices = GetPhysicalDeviceQueueFamilyIndices(pdevice);
    if(!indices.IsCompleted()) return false;
    std::vector<const char*> exts;
    GetRequestDeviceExts(exts);
    uint32_t avext_count;
    vkEnumerateDeviceExtensionProperties(pdevice,"",&avext_count,nullptr);
    std::vector<VkExtensionProperties> avexts(avext_count);
    vkEnumerateDeviceExtensionProperties(pdevice,"",&avext_count,avexts.data());
    for(auto ext:exts){
        bool surpport = false;
        for(auto avext:avexts){
            if(strcmp(avext.extensionName,ext)==0){
                surpport = true;
                break;
            }
        }
        if(!surpport) return false;
    }
    uint32_t surfaceformat_count;
    uint32_t surfacepresentmode_count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(pdevice,Surface,&surfacepresentmode_count,nullptr);
    if(surfacepresentmode_count == 0) return false;
    vkGetPhysicalDeviceSurfaceFormatsKHR(pdevice,Surface,&surfaceformat_count,nullptr);
    if(surfaceformat_count == 0) return false;
    
    return true;
}
void Renderer::GetRequestDeviceExts(std::vector<const char *>& exts)
{
    exts.resize(0);
    exts.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
}
void Renderer::GetRequestDeviceFeature(VkPhysicalDeviceFeatures& features)
{
    features = VkPhysicalDeviceFeatures();
    features.samplerAnisotropy = VK_TRUE;
    
}
SurfaceDetails Renderer::GetSurfaceDetails()
{
    SurfaceDetails details{};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(PDevice,Surface,&details.capabilities);
    uint32_t format_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(PDevice,Surface,&format_count,nullptr);
    details.formats.resize(format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(PDevice,Surface,&format_count,details.formats.data());
    uint32_t presentmode_count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(PDevice,Surface,&presentmode_count,nullptr);
    details.presentmode.resize(presentmode_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(PDevice,Surface,&presentmode_count,details.presentmode.data());
    return details;
}
VkSurfaceFormatKHR Renderer::ChooseSwapChainImageFormat(std::vector<VkSurfaceFormatKHR> &formats)
{
    for(auto& format:formats){
        if(format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR){
            return format;
        }
    }
    return formats[0];
}
VkPresentModeKHR Renderer::ChooseSwapChainImagePresentMode(std::vector<VkPresentModeKHR> &presentmodes)
{
    for(auto& presentmode:presentmodes){
        if(presentmode == VK_PRESENT_MODE_MAILBOX_KHR)
            return presentmode;
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}
VkExtent2D Renderer::ChooseSwapChainImageExtents(VkSurfaceCapabilitiesKHR &capabilities)
{
    if(capabilities.currentExtent.height != std::numeric_limits<uint32_t>::max()){
        return capabilities.currentExtent;
    }
    VkExtent2D extent = capabilities.minImageExtent;
    extent.height = std::min(extent.height,capabilities.maxImageExtent.height);
    extent.width = std::min(extent.width,capabilities.maxImageExtent.width);
    return extent;
}
TickResult Renderer::Tick(float DeltaTime)
{
    if(glfwWindowShouldClose(Window)) return TickResult::EXIT;
    glfwPollEvents();
    int w,h;
    glfwGetFramebufferSize(Window,&w,&h);
    if(w*h!=0){
        Draw();
    }
    return TickResult::NONE;
}
void Renderer::Draw()
{
    uint64_t notimeout = UINT32_MAX;
    vkWaitForFences(LDevice,1,&InFlightFences[CurrentFlight],VK_TRUE,notimeout);
    uint32_t imageindex;
    auto result = vkAcquireNextImageKHR(LDevice,SwapChain,notimeout,ImageAvaliable[CurrentFlight],VK_NULL_HANDLE,&imageindex);
    if(result==VK_ERROR_OUT_OF_DATE_KHR||result == VK_SUBOPTIMAL_KHR || bFramebufferResized){
        bFramebufferResized = false;
        RecreateSwapChain();
        return;
    }
    else if(result!=VK_SUCCESS){
        throw std::runtime_error("failed to acquire image!");
    }
    vkResetFences(LDevice,1,&InFlightFences[CurrentFlight]);
    auto cb = CreateCommandBuffer();
    std::array<VkClearValue,3> clearvalues{};
    clearvalues[0].color = {{0,0,0,1}};
    clearvalues[1].depthStencil = {1};
    clearvalues[2].color = {{0,0,0,1}};
    VkRenderPassBeginInfo renderpassbegininfo{};
    renderpassbegininfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderpassbegininfo.renderArea.offset = {0,0};
    renderpassbegininfo.renderArea.extent = SwapChainImageExtent;
    renderpassbegininfo.clearValueCount = static_cast<uint32_t>(clearvalues.size());
    renderpassbegininfo.pClearValues = clearvalues.data();
    renderpassbegininfo.renderPass = GraphicRenderPass;
    renderpassbegininfo.framebuffer = SwapChainFramebuffers[imageindex];

    vkCmdBeginRenderPass(cb,&renderpassbegininfo,VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cb,VK_PIPELINE_BIND_POINT_GRAPHICS,GraphicPipeline);
    VkViewport viewport{};
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    viewport.width = SwapChainImageExtent.width;
    viewport.height = SwapChainImageExtent.height;
    viewport.x = viewport.y = 0.0f;
    vkCmdSetViewport(cb,0,1,&viewport);
    VkRect2D scissor{};
    scissor.extent = SwapChainImageExtent;
    scissor.offset = {0,0};
    vkCmdSetScissor(cb,0,1,&scissor);
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cb,0,1,&VertexBuffer,&offset);
    vkCmdBindIndexBuffer(cb,IndexBuffer,0,VK_INDEX_TYPE_UINT32);
    vkCmdBindDescriptorSets(cb,VK_PIPELINE_BIND_POINT_GRAPHICS,GraphicPipelineLayout,0,1,&GraphicDescriptorSet,0,nullptr);
    vkCmdDrawIndexed(cb,indexs.size(),1,0,0,0);
    vkCmdEndRenderPass(cb);

    std::vector<VkSemaphore> signalsems = {RenderingFinish[CurrentFlight]};
    std::vector<VkSemaphore> waitsems = {ImageAvaliable[CurrentFlight]};
    std::vector<VkPipelineStageFlags> waitstages = {VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT};
    VkSubmitInfo submitinfo{};
    submitinfo.signalSemaphoreCount = static_cast<uint32_t>(signalsems.size());
    submitinfo.pSignalSemaphores = signalsems.data();
    submitinfo.waitSemaphoreCount = static_cast<uint32_t>(waitsems.size());
    submitinfo.pWaitSemaphores = waitsems.data();
    submitinfo.pWaitDstStageMask = waitstages.data();
    SubmitCommandBuffer(cb,submitinfo,InFlightFences[CurrentFlight]);

    VkPresentInfoKHR presentinfo{};
    presentinfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentinfo.swapchainCount = 1;
    presentinfo.pSwapchains = &SwapChain;
    presentinfo.pImageIndices = &imageindex;
    presentinfo.waitSemaphoreCount = 1;
    presentinfo.pWaitSemaphores = &RenderingFinish[CurrentFlight];
    result = vkQueuePresentKHR(PresentQueue,&presentinfo);
    if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || bFramebufferResized){
        bFramebufferResized = false;
        RecreateSwapChain();
        return;
    } 
    else if(result!=VK_SUCCESS){
        throw std::runtime_error("failed to present image!");
    }
    CurrentFlight = (CurrentFlight + 1)%MAXInFlightRendering;
}
void Renderer::SetVertices(const std::vector<Vertex> &vs,const std::vector<uint32_t>& is)
{
    vkDeviceWaitIdle(LDevice);
    vertices.assign(vs.begin(),vs.end());
    indexs.assign(is.begin(),is.end());
    CleanupBuffer(VertexBuffer,VertexBufferMemory,false);
    CleanupBuffer(IndexBuffer,IndexBufferMemory,false);
    CreateVertexBuffer();
    CreateIndexBuffer();
}
