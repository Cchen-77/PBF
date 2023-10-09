#include"renderer.h"
#include"extensionfuncs.h"
#include"helperfuncs.h"

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
void Renderer::SubmitCommandBuffer(VkCommandBuffer& cb,VkSubmitInfo submitinfo,VkFence fence,VkQueue queue)
{
    submitinfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitinfo.commandBufferCount = 1;
    submitinfo.pCommandBuffers = &cb;
    if(vkEndCommandBuffer(cb)!=VK_SUCCESS){
        throw std::runtime_error("failed to end command buffer!");
    }
    if(vkQueueSubmit(queue,1,&submitinfo,fence)!=VK_SUCCESS){
        throw std::runtime_error("failed to submit command buffer!");
    }
    vkDeviceWaitIdle(LDevice);
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
    else if(oldlayout == VK_IMAGE_LAYOUT_UNDEFINED && newlayout == VK_IMAGE_LAYOUT_GENERAL){
        srcstage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        barrier.srcAccessMask = 0;
        dststage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT|VK_ACCESS_SHADER_READ_BIT;
    }
    else{
        throw std::runtime_error("bad oldlayout to newlayout!");
    }   
    auto cb = CreateCommandBuffer();
    vkCmdPipelineBarrier(cb,srcstage,dststage,0,0,nullptr,0,nullptr,1,&barrier);
    VkSubmitInfo submitinfo{};
    SubmitCommandBuffer(cb,submitinfo,VK_NULL_HANDLE,GraphicNComputeQueue);
}
void Renderer::UpdateDescriptorSet()
{

    for(uint32_t i=0;i<PostprocessDescriptorSets.size();++i){
        VkDescriptorImageInfo thickimageinfo{};
        VkDescriptorImageInfo depthimageinfo{};
        VkDescriptorImageInfo dstimageinfo{};
        thickimageinfo.imageView = ThickImageView;
        thickimageinfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        thickimageinfo.sampler = ThickImageSampler;
        depthimageinfo.imageView = FilteredDepthImageView;
        depthimageinfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        depthimageinfo.sampler = FilteredDepthImageSampler;
        dstimageinfo.imageView = SwapChainImageViews[i];
        dstimageinfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        std::array<VkWriteDescriptorSet,3> writes{};
        writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[0].descriptorCount = 1;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[0].dstArrayElement = 0;
        writes[0].dstBinding = 1;
        writes[0].dstSet = PostprocessDescriptorSets[i];
        writes[0].pImageInfo = &depthimageinfo;

        writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[1].descriptorCount = 1;
        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[1].dstArrayElement = 0;
        writes[1].dstBinding = 2;
        writes[1].dstSet = PostprocessDescriptorSets[i];
        writes[1].pImageInfo = &thickimageinfo;
        
        writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[2].descriptorCount = 1;
        writes[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        writes[2].dstArrayElement = 0;
        writes[2].dstBinding = 3;
        writes[2].dstSet = PostprocessDescriptorSets[i];
        writes[2].pImageInfo = &dstimageinfo;

        vkUpdateDescriptorSets(LDevice,static_cast<uint32_t>(writes.size()),writes.data(),0,nullptr);
    }
    {
        VkDescriptorImageInfo customdepthimageinfo{};
        VkDescriptorImageInfo filtereddepthimageinfo{};
        customdepthimageinfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        customdepthimageinfo.imageView = CustomDepthImageView;
        customdepthimageinfo.sampler = CustomDepthImageSampler;
        filtereddepthimageinfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        filtereddepthimageinfo.imageView = FilteredDepthImageView;
        std::array<VkWriteDescriptorSet,2> writes{};
        writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[0].descriptorCount = 1;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[0].dstArrayElement = 0;
        writes[0].dstBinding = 0;
        writes[0].dstSet = FilterDescriptorSet;
        writes[0].pImageInfo = &customdepthimageinfo;
        writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[1].descriptorCount = 1;
        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        writes[1].dstArrayElement = 0;
        writes[1].dstBinding = 1;
        writes[1].dstSet = FilterDescriptorSet;
        writes[1].pImageInfo = &filtereddepthimageinfo;
        vkUpdateDescriptorSets(LDevice,static_cast<uint32_t>(writes.size()),writes.data(),0,nullptr);
    }
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

void Renderer::SetRenderingObj(const UniformRenderingObject &robj)
{
    if(Initialized){
        vkDeviceWaitIdle(LDevice);
        memcpy(MappedRenderingBuffer,&robj,sizeof(UniformRenderingObject));
        vkDeviceWaitIdle(LDevice);
    }
    else{
        renderingobj = robj;
    }

}
void Renderer::SetSimulatingObj(const UniformSimulatingObject &sobj)
{
    if(Initialized){
        vkDeviceWaitIdle(LDevice);
        auto pObj = reinterpret_cast<UniformSimulatingObject*>(MappedSimulatingBuffer);
        pObj->dt = sobj.dt;
        pObj->accumulated_t = sobj.accumulated_t;
        vkDeviceWaitIdle(LDevice);
    }
    else{
        simulatingobj = sobj;
    }
}

void Renderer::SetNSObj(const UniformNSObject &nobj)
{
    if(Initialized){
        vkDeviceWaitIdle(LDevice);
        memcpy(MappedNSBuffer,&nobj,sizeof(UniformNSObject));
        vkDeviceWaitIdle(LDevice);
    }
    else{
        nsobject = nobj;
    }
}

void Renderer::SetParticles(const std::vector<Particle> &ps)
{
    if(Initialized){
        throw std::runtime_error("you should not set particles after vulkan initialized!");
    }
    else if(ps.size()>=ONE_GROUP_INVOCATION_COUNT*ONE_GROUP_INVOCATION_COUNT){
       throw std::runtime_error("num of particles is too big!");
    }
    else{
        particles.assign(ps.begin(),ps.end());
    }
}
Renderer::Renderer(uint32_t w, uint32_t h, bool validation)
{
    Width = w;
    Height = h;   
    bEnableValidation = validation;

}
Renderer::~Renderer()
{

}
void Renderer::Init()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API,GLFW_NO_API);
    Window = glfwCreateWindow(Width,Height,"jason's renderer",nullptr,nullptr);
    glfwSetWindowUserPointer(Window,this);
    glfwSetFramebufferSizeCallback(Window,&Renderer::WindowResizeCallback);
    CreateInstance();
    CreateDebugMessenger();
    CreateSurface();
    PickPhysicalDevice();
    CreateLogicalDevice();

    CreateSupportObjects();
    CreateCommandPool();

    CreateParticleBuffer();
    CreateParticleNgbrBuffer();
 
    CreateRadixsortedIndexBuffer();
    CreateRSGlobalBucketBuffer();
    CreateCellinfoBuffer();
    CreateLocalPrefixBuffer();
    
    CreateUniformNSBuffer();
    CreateUniformRenderingBuffer();
    CreateUniformSimulatingBuffer();

    CreateSwapChain();
    CreateDepthResources();
    CreateThickResources();


    CreateDescriptorSetLayout();
    CreateDescriptorPool();
    CreateDescriptorSet();
    CreateRenderPass();
    CreateGraphicPipelineLayout();
    CreateGraphicPipeline();
  
    CreateComputePipelineLayout();
    CreateComputePipeline();
   
    CreateFramebuffers(); 

    RecordSimulatingCommandBuffers();

    Initialized = true;
}
void Renderer::Cleanup()
{
    vkDeviceWaitIdle(LDevice);

    vkFreeCommandBuffers(LDevice,CommandPool,MAXInFlightRendering,SimulatingCommandBuffers.data());
    
    vkDestroyPipeline(LDevice,NSPipeline_CalcellHash,Allocator);
    vkDestroyPipeline(LDevice,NSPipeline_Radixsort1,Allocator);
    vkDestroyPipeline(LDevice,NSPipeline_Radixsort2,Allocator);
    vkDestroyPipeline(LDevice,NSPipeline_Radixsort3,Allocator);
    vkDestroyPipeline(LDevice,NSPipeline_FixcellBuffer,Allocator);
    vkDestroyPipeline(LDevice,NSPipeline_GetNgbrs,Allocator);


    vkDestroyPipeline(LDevice,SimulatePipeline_Euler,Allocator);
    vkDestroyPipeline(LDevice,SimulatePipeline_Lambda,Allocator);
    vkDestroyPipeline(LDevice,SimulatePipeline_DeltaPosition,Allocator);
    vkDestroyPipeline(LDevice,SimulatePipeline_PositionUpd,Allocator);
    vkDestroyPipeline(LDevice,SimulatePipeline_VelocityUpd,Allocator);
    vkDestroyPipeline(LDevice,SimulatePipeline_VelocityCache,Allocator);
    vkDestroyPipeline(LDevice,SimulatePipeline_ViscosityCorr,Allocator);
    vkDestroyPipeline(LDevice,SimulatePipeline_VorticityCorr,Allocator);
    vkDestroyPipelineLayout(LDevice,SimulatePipelineLayout,Allocator);



    vkDestroyPipeline(LDevice,PostprocessPipeline,Allocator);
    vkDestroyPipelineLayout(LDevice,PostprocessPipelineLayout,Allocator);

    vkDestroyPipeline(LDevice,FilterPipeline,Allocator);
    vkDestroyPipelineLayout(LDevice,FilterPipelineLayout,Allocator);
    
    vkDestroyPipeline(LDevice,GraphicPipeline,Allocator);
    vkDestroyPipelineLayout(LDevice,GraphicPipelineLayout,Allocator);
    vkDestroyRenderPass(LDevice,GraphicRenderPass,Allocator);

    vkDestroyPipelineLayout(LDevice,NSPipelineLayout,Allocator);

    vkDestroyDescriptorPool(LDevice,DescriptorPool,Allocator);

    vkDestroyDescriptorSetLayout(LDevice,GraphicDescriptorSetLayout,Allocator); 
    vkDestroyDescriptorSetLayout(LDevice,SimulateDescriptorSetLayout,Allocator);
    vkDestroyDescriptorSetLayout(LDevice,FilterDecsriptorSetLayout,Allocator);
    vkDestroyDescriptorSetLayout(LDevice,PostprocessDescriptorSetLayout,Allocator);
    vkDestroyDescriptorSetLayout(LDevice,NSDescriptorSetLayout,Allocator);
    
    vkDestroyFramebuffer(LDevice,Framebuffer,Allocator);

    vkDestroySampler(LDevice,CustomDepthImageSampler,Allocator);
    vkDestroyImageView(LDevice,DepthImageView,Allocator);
    vkDestroyImage(LDevice,DepthImage,Allocator);
    vkFreeMemory(LDevice,DepthImageMemory,Allocator);  
    
    vkDestroyImageView(LDevice,CustomDepthImageView,Allocator);
    vkDestroyImage(LDevice,CustomDepthImage,Allocator);
    vkFreeMemory(LDevice,CustomDepthImageMemory,Allocator);
  
    vkDestroySampler(LDevice,FilteredDepthImageSampler,Allocator);
    vkDestroyImageView(LDevice,FilteredDepthImageView,Allocator);
    vkDestroyImage(LDevice,FilteredDepthImage,Allocator);
    vkFreeMemory(LDevice,FilteredDepthImageMemory,Allocator);
    
    vkDestroySampler(LDevice,ThickImageSampler,Allocator);
    vkDestroyImageView(LDevice,ThickImageView,Allocator);
    vkDestroyImage(LDevice,ThickImage,Allocator);
    vkFreeMemory(LDevice,ThickImageMemory,Allocator);

    CleanupSwapChain();


    for(uint32_t i=0;i<MAXInFlightRendering;++i){
        CleanupBuffer(ParticleBuffers[i],ParticleBufferMemory[i],false);
    }
    CleanupBuffer(ParticleNgbrBuffer,ParticleNgbrBufferMemory,false);
    CleanupBuffer(UniformRenderingBuffer,UniformRenderingBufferMemory,true);
    CleanupBuffer(UniformSimulatingBuffer,UniformSimulatingBufferMemory,true);
    CleanupBuffer(UniformNSBuffer,UniformNSBufferMemory,true);
    for(uint32_t i=0;i<2;++i){
        CleanupBuffer(RadixsortedIndexBuffer[i],RadixsortedIndexBufferMemory[i],false);
    }
    CleanupBuffer(RSGlobalBucketBuffer,RSGlobalBucketBufferMemory,false);
    CleanupBuffer(CellinfoBuffer,CellinfoBufferMemory,false);
    CleanupBuffer(LocalPrefixBuffer,LocalPrefixBufferMemory,false);

    vkDestroyCommandPool(LDevice,CommandPool,Allocator);
    CleanupSupportObjects();

    vkDestroyDevice(LDevice,Allocator);
    vkDestroySurfaceKHR(Instance,Surface,Allocator);
    ExtensionFuncs::vkDestroyDebugUtilsMessengerEXT(Instance,Messenger,Allocator);
    vkDestroyInstance(Instance,Allocator);

    glfwDestroyWindow(Window);
    glfwTerminate();
}
void Renderer::CreateInstance()
{
    VkApplicationInfo appinfo{};
    appinfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appinfo.pApplicationName = "Jason's Renderer";
    appinfo.apiVersion = VK_MAKE_API_VERSION(0,1,3,0);
    
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
void Renderer::CreateSupportObjects(){

    VkSemaphoreCreateInfo seminfo{};
    seminfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    seminfo.flags = VK_SEMAPHORE_TYPE_BINARY;
    if(vkCreateSemaphore(LDevice,&seminfo,Allocator,&ImageAvaliable)!=VK_SUCCESS){
        throw std::runtime_error("failed to create sem:imageavaliable!");
    }
    if(vkCreateSemaphore(LDevice,&seminfo,Allocator,&RenderingFinish)!=VK_SUCCESS){
        throw std::runtime_error("failed to create sem:renderingfinish!");
    }
    if(vkCreateSemaphore(LDevice,&seminfo,Allocator,&FilteringFinish)!=VK_SUCCESS){
        throw std::runtime_error("failed to create sem:filteringfinish!");
    }
    if(vkCreateSemaphore(LDevice,&seminfo,Allocator,&DepthNThickAvaliable)!=VK_SUCCESS){
        throw std::runtime_error("failed to create sem:depthnthickavaliable!");
    }
    if(vkCreateSemaphore(LDevice,&seminfo,Allocator,&SimulatingFinish)!=VK_SUCCESS){
        throw std::runtime_error("failed to create sem:simulatingfinsh!");
    }
    VkFenceCreateInfo fenceinfo{};
    fenceinfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceinfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    if(vkCreateFence(LDevice,&fenceinfo,Allocator,&DrawingFence)!=VK_SUCCESS){
        throw std::runtime_error("failed to create fence:inflight!");
    }

}
void Renderer::CleanupSupportObjects()
{
    vkDestroySemaphore(LDevice,ImageAvaliable,Allocator);
    vkDestroySemaphore(LDevice,RenderingFinish,Allocator);
    vkDestroySemaphore(LDevice,FilteringFinish,Allocator);
    vkDestroySemaphore(LDevice,DepthNThickAvaliable,Allocator);
    vkDestroySemaphore(LDevice,SimulatingFinish,Allocator);
    vkDestroyFence(LDevice,DrawingFence,Allocator);
    
}
void Renderer::CreateDebugMessenger()
{
    VkDebugUtilsMessengerCreateInfoEXT createinfo{};
    MakeMessengerInfo(createinfo);
    if(ExtensionFuncs::vkCreateDebugUtilsMessengerEXT(Instance,&createinfo,Allocator,&Messenger)!=VK_SUCCESS){
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

    createinfo.queueCreateInfoCount = static_cast<uint32_t>(queueinfos.size());
    createinfo.pQueueCreateInfos = queueinfos.data();
    
    VkPhysicalDeviceFeatures features;
    GetRequestDeviceFeature(features);

    VkPhysicalDeviceShaderObjectFeaturesEXT shaderobjectFeatures{};
    shaderobjectFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_OBJECT_FEATURES_EXT;
    shaderobjectFeatures.shaderObject = VK_TRUE;

    VkPhysicalDeviceExtendedDynamicState3FeaturesEXT extendedDynamicState3Features{};
    extendedDynamicState3Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_3_FEATURES_EXT;
    extendedDynamicState3Features.extendedDynamicState3ColorBlendEnable = VK_TRUE;
    extendedDynamicState3Features.pNext = &shaderobjectFeatures;

    VkPhysicalDeviceExtendedDynamicStateFeaturesEXT extendedDynamicStateFeatures{};
    extendedDynamicStateFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT;
    extendedDynamicStateFeatures.extendedDynamicState = VK_TRUE;
    extendedDynamicStateFeatures.pNext = &extendedDynamicState3Features;
    

    VkPhysicalDeviceFeatures2 features2{};
    features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    features2.features = features;
    features2.pNext = &extendedDynamicStateFeatures;
    createinfo.pNext = &features2;
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
void Renderer::CreateParticleBuffer()
{
    if(particles.size()%ONE_GROUP_INVOCATION_COUNT != 0){
        WORK_GROUP_COUNT = particles.size()/ONE_GROUP_INVOCATION_COUNT + 1;
    }
    else{
        WORK_GROUP_COUNT = particles.size()/ONE_GROUP_INVOCATION_COUNT;
    }
    nsobject.numParticles = particles.size();
    nsobject.workgroup_count = WORK_GROUP_COUNT;
    nsobject.hashsize = particles.size()*2;

    simulatingobj.numParticles = particles.size();

    ParticleBufferMemory.resize(MAXInFlightRendering);
    ParticleBuffers.resize(MAXInFlightRendering);
    VkDeviceSize size = particles.size()*sizeof(Particle);

    for(uint32_t i=0;i<MAXInFlightRendering;++i){
        VkBuffer stagingbuffer;
        VkDeviceMemory stagingmemory;
        CreateBuffer(stagingbuffer,stagingmemory,size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        CreateBuffer(ParticleBuffers[i],ParticleBufferMemory[i],size,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT|VK_BUFFER_USAGE_STORAGE_BUFFER_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        void* data;
        vkMapMemory(LDevice,stagingmemory,0,size,0,&data);
        memcpy(data,particles.data(),size);
        
        auto cb = CreateCommandBuffer();
        VkBufferCopy region{};
        region.size = size;
        region.dstOffset = region.srcOffset = 0;
        vkCmdCopyBuffer(cb,stagingbuffer,ParticleBuffers[i],1,&region);
        VkSubmitInfo submitinfo{};
        SubmitCommandBuffer(cb,submitinfo,VK_NULL_HANDLE,GraphicNComputeQueue);
        
        vkDeviceWaitIdle(LDevice);
        CleanupBuffer(stagingbuffer,stagingmemory,true);
    }
}

void Renderer::CreateParticleNgbrBuffer()
{
    VkDeviceSize size = MAX_NGBR_NUM*particles.size()*sizeof(uint32_t);
    CreateBuffer(ParticleNgbrBuffer,ParticleNgbrBufferMemory,size,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

void Renderer::CreateUniformRenderingBuffer()
{
    VkDeviceSize size = sizeof(UniformRenderingObject);
    CreateBuffer(UniformRenderingBuffer,UniformRenderingBufferMemory,size,
    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    vkMapMemory(LDevice,UniformRenderingBufferMemory,0,size,0,&MappedRenderingBuffer);
    memcpy(MappedRenderingBuffer,&renderingobj,size);
}

void Renderer::CreateUniformSimulatingBuffer()
{
    VkDeviceSize size = sizeof(UniformSimulatingObject);
    CreateBuffer(UniformSimulatingBuffer,UniformSimulatingBufferMemory,size,
    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    vkMapMemory(LDevice,UniformSimulatingBufferMemory,0,size,0,&MappedSimulatingBuffer);
    memcpy(MappedSimulatingBuffer,&simulatingobj,size);
}

void Renderer::CreateUniformNSBuffer()
{
    VkDeviceSize size = sizeof(UniformNSObject);
    CreateBuffer(UniformNSBuffer,UniformNSBufferMemory,size,
    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    vkMapMemory(LDevice,UniformNSBufferMemory,0,size,0,&MappedNSBuffer);
    memcpy(MappedNSBuffer,&nsobject,size);
}

void Renderer::CreateRadixsortedIndexBuffer()
{
    VkDeviceSize size = sizeof(uint32_t)*particles.size();
    for(uint32_t i=0;i<2;++i){
        VkBuffer stagingbuffer;
        VkDeviceMemory stagingmemory;
        CreateBuffer(stagingbuffer,stagingmemory,size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        CreateBuffer(RadixsortedIndexBuffer[i],RadixsortedIndexBufferMemory[i],size,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        void* data;
        vkMapMemory(LDevice,stagingmemory,0,size,0,&data);
        int* intdata = reinterpret_cast<int*>(data);
        for(uint32_t j=0;j<size/4;++j)
        {
            intdata[j] = j;
        }
        
        auto cb = CreateCommandBuffer();
        VkBufferCopy region{};
        region.size = size;
        region.dstOffset = region.srcOffset = 0;
        vkCmdCopyBuffer(cb,stagingbuffer,RadixsortedIndexBuffer[i],1,&region);
        VkSubmitInfo submitinfo{};
        SubmitCommandBuffer(cb,submitinfo,VK_NULL_HANDLE,GraphicNComputeQueue);
        
        vkDeviceWaitIdle(LDevice);
        CleanupBuffer(stagingbuffer,stagingmemory,true);
    }
}

void Renderer::CreateRSGlobalBucketBuffer()
{
    VkDeviceSize size = sizeof(uint32_t)*(WORK_GROUP_COUNT+1)*16;
    CreateBuffer(RSGlobalBucketBuffer,RSGlobalBucketBufferMemory,size,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

void Renderer::CreateCellinfoBuffer()
{
    VkDeviceSize size = sizeof(uint32_t)*4*particles.size();
    CreateBuffer(CellinfoBuffer,CellinfoBufferMemory,size,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

void Renderer::CreateLocalPrefixBuffer()
{
    VkDeviceSize size = sizeof(uint32_t)*16*particles.size();
    CreateBuffer(LocalPrefixBuffer,LocalPrefixBufferMemory,size,VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
}

void Renderer::CreateDepthResources()
{
    VkExtent3D extent = {SwapChainImageExtent.width,SwapChainImageExtent.height,1};
    CreateImage(DepthImage,DepthImageMemory,extent,VK_FORMAT_D32_SFLOAT,VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_IMAGE_USAGE_SAMPLED_BIT,VK_SAMPLE_COUNT_1_BIT);
    DepthImageView = CreateImageView(DepthImage,VK_FORMAT_D32_SFLOAT,VK_IMAGE_ASPECT_DEPTH_BIT);
    ImageLayoutTransition(DepthImage,VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,VK_IMAGE_ASPECT_DEPTH_BIT);

    CreateImage(CustomDepthImage,CustomDepthImageMemory,extent,VK_FORMAT_R32_SFLOAT,VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_SAMPLED_BIT,VK_SAMPLE_COUNT_1_BIT);
    CustomDepthImageView = CreateImageView(CustomDepthImage,VK_FORMAT_R32_SFLOAT,VK_IMAGE_ASPECT_COLOR_BIT);
    ImageLayoutTransition(CustomDepthImage,VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,VK_IMAGE_ASPECT_COLOR_BIT);

    CreateImage(FilteredDepthImage,FilteredDepthImageMemory,extent,VK_FORMAT_R32_SFLOAT,VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_STORAGE_BIT,VK_SAMPLE_COUNT_1_BIT);
    FilteredDepthImageView = CreateImageView(FilteredDepthImage,VK_FORMAT_R32_SFLOAT,VK_IMAGE_ASPECT_COLOR_BIT);

    VkSamplerCreateInfo samplerinfo{};
    samplerinfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerinfo.anisotropyEnable = VK_FALSE;
    samplerinfo.minFilter = VK_FILTER_LINEAR;
    samplerinfo.magFilter = VK_FILTER_LINEAR;
    vkCreateSampler(LDevice,&samplerinfo,Allocator,&CustomDepthImageSampler);
    vkCreateSampler(LDevice,&samplerinfo,Allocator,&FilteredDepthImageSampler);

}

void Renderer::CreateThickResources()
{
    VkExtent3D extent = {SwapChainImageExtent.width,SwapChainImageExtent.height,1};
    CreateImage(ThickImage,ThickImageMemory,extent,VK_FORMAT_R32_SFLOAT,VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_SAMPLED_BIT,VK_SAMPLE_COUNT_1_BIT);
    ThickImageView = CreateImageView(ThickImage,VK_FORMAT_R32_SFLOAT,VK_IMAGE_ASPECT_COLOR_BIT);
    ImageLayoutTransition(ThickImage,VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,VK_IMAGE_ASPECT_COLOR_BIT);
    
    VkSamplerCreateInfo samplerinfo{};
    samplerinfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerinfo.anisotropyEnable = VK_FALSE;
    samplerinfo.minFilter = VK_FILTER_LINEAR;
    samplerinfo.magFilter = VK_FILTER_LINEAR;
    vkCreateSampler(LDevice,&samplerinfo,Allocator,&ThickImageSampler);
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
    createinfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
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
    for(auto& imageview:SwapChainImageViews){
        vkDestroyImageView(LDevice,imageview,Allocator);
    }
    vkDestroySwapchainKHR(LDevice,SwapChain,Allocator);

}
void Renderer::RecreateSwapChain()
{
    vkDeviceWaitIdle(LDevice);
    bFramebufferResized = false;
    vkDestroyFramebuffer(LDevice,Framebuffer,Allocator);

    vkDestroyImageView(LDevice,DepthImageView,Allocator);
    vkDestroyImage(LDevice,DepthImage,Allocator);
    vkFreeMemory(LDevice,DepthImageMemory,Allocator);

    vkDestroySampler(LDevice,ThickImageSampler,Allocator);
    vkDestroyImageView(LDevice,ThickImageView,Allocator);
    vkDestroyImage(LDevice,ThickImage,Allocator);
    vkFreeMemory(LDevice,ThickImageMemory,Allocator);

    vkDestroySampler(LDevice,CustomDepthImageSampler,Allocator);
    vkDestroyImageView(LDevice,CustomDepthImageView,Allocator);
    vkDestroyImage(LDevice,CustomDepthImage,Allocator);
    vkFreeMemory(LDevice,CustomDepthImageMemory,Allocator);

    vkDestroySampler(LDevice,FilteredDepthImageSampler,Allocator);
    vkDestroyImageView(LDevice,FilteredDepthImageView,Allocator);
    vkDestroyImage(LDevice,FilteredDepthImage,Allocator);
    vkFreeMemory(LDevice,FilteredDepthImageMemory,Allocator);

    CleanupSwapChain();
    CreateSwapChain();
    CreateDepthResources();
    CreateThickResources();
    CreateFramebuffers();

    UpdateDescriptorSet();

}
void Renderer::CreateDescriptorSetLayout()
{
    {
        std::array<VkDescriptorSetLayoutBinding,1> bindings{};
        bindings[0].binding = 0;
        bindings[0].descriptorCount = 1;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT|VK_SHADER_STAGE_FRAGMENT_BIT;

    
        VkDescriptorSetLayoutCreateInfo createinfo{};
        createinfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        createinfo.bindingCount = static_cast<uint32_t>(bindings.size());
        createinfo.pBindings = bindings.data();
        
        if(vkCreateDescriptorSetLayout(LDevice,&createinfo,Allocator,&GraphicDescriptorSetLayout)!=VK_SUCCESS){
            throw std::runtime_error("failed to create graphic descriptor set layout!");
        }
    }

    {
        std::array<VkDescriptorSetLayoutBinding,4> bindings{};
        bindings[0].binding = 0;
        bindings[0].descriptorCount = 1;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        bindings[1].binding = 1;
        bindings[1].descriptorCount = 1;
        bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        bindings[2].binding = 2;
        bindings[2].descriptorCount = 1;
        bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        bindings[3].binding = 3;
        bindings[3].descriptorCount = 1;
        bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[3].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        VkDescriptorSetLayoutCreateInfo createinfo{};
        createinfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        createinfo.bindingCount = static_cast<uint32_t>(bindings.size());
        createinfo.pBindings = bindings.data();

        if(vkCreateDescriptorSetLayout(LDevice,&createinfo,Allocator,&SimulateDescriptorSetLayout)!=VK_SUCCESS){
            throw std::runtime_error("failed to create simulate descriptor set layout!");
        }
    }
    {
        std::array<VkDescriptorSetLayoutBinding,4> bindings{};

        bindings[0].binding = 0;
        bindings[0].descriptorCount = 1;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;    

        bindings[1].binding = 1;
        bindings[1].descriptorCount = 1;
        bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        bindings[2].binding = 2;
        bindings[2].descriptorCount = 1;
        bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        bindings[3].binding = 3;
        bindings[3].descriptorCount = 1;
        bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        bindings[3].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;   

        VkDescriptorSetLayoutCreateInfo createinfo{};
        createinfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        createinfo.bindingCount = static_cast<uint32_t>(bindings.size());
        createinfo.pBindings = bindings.data();
        if(vkCreateDescriptorSetLayout(LDevice,&createinfo,Allocator,&PostprocessDescriptorSetLayout)!=VK_SUCCESS){
            throw std::runtime_error("failed to create postprocess descriptor set layout!");
        }
    }
    {
        std::array<VkDescriptorSetLayoutBinding,2> bindings{};
        bindings[0].binding = 0;
        bindings[0].descriptorCount = 1;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        bindings[1].binding = 1;
        bindings[1].descriptorCount = 1;
        bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        VkDescriptorSetLayoutCreateInfo createinfo{};
        createinfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        createinfo.bindingCount = static_cast<uint32_t>(bindings.size());
        createinfo.pBindings = bindings.data();
        if(vkCreateDescriptorSetLayout(LDevice,&createinfo,Allocator,&FilterDecsriptorSetLayout)!=VK_SUCCESS){
            throw std::runtime_error("failed to create filter descriptor set layout!");
        }
    }
    {
        std::array<VkDescriptorSetLayoutBinding,8> bindings{};
        bindings[0].binding = 0;
        bindings[0].descriptorCount = 1;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        bindings[1].binding = 1;
        bindings[1].descriptorCount = 1;
        bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        bindings[2].binding = 2;
        bindings[2].descriptorCount = 1;
        bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        bindings[3].binding = 3;
        bindings[3].descriptorCount = 1;
        bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[3].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        bindings[4].binding = 4;
        bindings[4].descriptorCount = 1;
        bindings[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[4].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        bindings[5].binding = 5;
        bindings[5].descriptorCount = 1;
        bindings[5].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[5].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        bindings[6].binding = 6;
        bindings[6].descriptorCount = 1;
        bindings[6].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[6].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        bindings[7].binding = 7;
        bindings[7].descriptorCount = 1;
        bindings[7].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[7].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;


        VkDescriptorSetLayoutCreateInfo createinfo{};
        createinfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        createinfo.bindingCount = static_cast<uint32_t>(bindings.size());
        createinfo.pBindings = bindings.data();
        if(vkCreateDescriptorSetLayout(LDevice,&createinfo,Allocator,&NSDescriptorSetLayout)!=VK_SUCCESS){
            throw std::runtime_error("failed to create neighborhood searcher descriptor set layout!");
        }

    }
}
void Renderer::CreateDescriptorPool()
{
    std::array<VkDescriptorPoolSize,4> poolsizes{};
    poolsizes[0].descriptorCount = 8;
    poolsizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolsizes[1].descriptorCount = 8;
    poolsizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolsizes[2].descriptorCount = 16;
    poolsizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolsizes[3].descriptorCount = 8;
    poolsizes[3].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;


    VkDescriptorPoolCreateInfo createinfo{};
    createinfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    createinfo.maxSets = 16;
    createinfo.poolSizeCount = static_cast<uint32_t>(poolsizes.size());
    createinfo.pPoolSizes = poolsizes.data();

    if(vkCreateDescriptorPool(LDevice,&createinfo,Allocator,&DescriptorPool)!=VK_SUCCESS){
        throw std::runtime_error("failed to create descriptor pool!");
    }
}
void Renderer::CreateDescriptorSet()
{
    {
        VkDescriptorSetAllocateInfo allocateinfo{};
        allocateinfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocateinfo.descriptorPool = DescriptorPool;
        allocateinfo.descriptorSetCount = 1;
        allocateinfo.pSetLayouts = &GraphicDescriptorSetLayout;
        if(vkAllocateDescriptorSets(LDevice,&allocateinfo,&GraphicDescriptorSet)!=VK_SUCCESS){
            throw std::runtime_error("failed to allocate graphic descriptor set!");
        }
        std::array<VkWriteDescriptorSet,1> writes{};

        VkDescriptorBufferInfo renderingbufferinfo{};
        renderingbufferinfo.buffer = UniformRenderingBuffer;
        renderingbufferinfo.offset = 0;
        renderingbufferinfo.range = sizeof(UniformRenderingObject);

        writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[0].descriptorCount = 1;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writes[0].dstArrayElement = 0;
        writes[0].dstBinding = 0;
        writes[0].dstSet = GraphicDescriptorSet;
        writes[0].pBufferInfo = &renderingbufferinfo;

        vkUpdateDescriptorSets(LDevice,writes.size(),writes.data(),0,nullptr);
    }
    {
        SimulateDescriptorSet.resize(MAXInFlightRendering);
        VkDescriptorSetAllocateInfo allocateinfo{};
        allocateinfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocateinfo.descriptorPool = DescriptorPool;
        allocateinfo.descriptorSetCount = 1;
        allocateinfo.pSetLayouts = &SimulateDescriptorSetLayout;
        for(uint32_t i=0;i<MAXInFlightRendering;++i){
            if(vkAllocateDescriptorSets(LDevice,&allocateinfo,&SimulateDescriptorSet[i])!=VK_SUCCESS){
                throw std::runtime_error("failed to allocate simulate descriptor set!");
            }
        }
        std::array<VkWriteDescriptorSet,4> writes{};

        VkDescriptorBufferInfo simulatingbufferinfo{};
        simulatingbufferinfo.buffer = UniformSimulatingBuffer;
        simulatingbufferinfo.offset = 0;
        simulatingbufferinfo.range = sizeof(UniformSimulatingObject);

        VkDescriptorBufferInfo ngbrbufferinfo{};
        ngbrbufferinfo.buffer = ParticleNgbrBuffer;
        ngbrbufferinfo.offset = 0;
        ngbrbufferinfo.range = MAX_NGBR_NUM*particles.size()*sizeof(uint32_t);
        
        writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[0].descriptorCount = 1;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writes[0].dstArrayElement = 0;
        writes[0].dstBinding = 0;
        writes[0].pBufferInfo = &simulatingbufferinfo;
            
        writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[1].descriptorCount = 1;
        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[1].dstArrayElement = 0;
        writes[1].dstBinding = 1;

        writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[2].descriptorCount = 1;
        writes[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[2].dstArrayElement = 0;
        writes[2].dstBinding = 2; 

        writes[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[3].descriptorCount = 1;
        writes[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[3].dstArrayElement = 0;
        writes[3].dstBinding = 3;
        writes[3].pBufferInfo = &ngbrbufferinfo;
        

        for(uint32_t i=0;i<MAXInFlightRendering;++i){
           
            VkDescriptorBufferInfo particlebufferinfo_thisframe{};
            particlebufferinfo_thisframe.buffer = ParticleBuffers[i];
            particlebufferinfo_thisframe.offset = 0;
            particlebufferinfo_thisframe.range = sizeof(Particle)*particles.size();
            VkDescriptorBufferInfo particlebufferinfo_lastframe{};
            particlebufferinfo_lastframe.buffer = ParticleBuffers[(i-1)%MAXInFlightRendering];
            particlebufferinfo_lastframe.offset = 0;
            particlebufferinfo_lastframe.range = sizeof(Particle)*particles.size();
            
            writes[0].dstSet = SimulateDescriptorSet[i];
            writes[1].dstSet = SimulateDescriptorSet[i];
            writes[1].pBufferInfo = &particlebufferinfo_lastframe;
            writes[2].dstSet = SimulateDescriptorSet[i];
            writes[2].pBufferInfo =  &particlebufferinfo_thisframe;
            writes[3].dstSet = SimulateDescriptorSet[i];
            

            vkUpdateDescriptorSets(LDevice,writes.size(),writes.data(),0,nullptr);
        }
    }

    {
        for(uint32_t i=0;i<2;++i){
            NSDescriptorSets[i].resize(MAXInFlightRendering);
        }
        for(uint32_t i=0;i<2;++i){
            for(uint32_t j=0;j<MAXInFlightRendering;++j){
                VkDescriptorSetAllocateInfo allocateinfo{};
                allocateinfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
                allocateinfo.descriptorSetCount = 1;
                allocateinfo.descriptorPool = DescriptorPool;
                allocateinfo.pSetLayouts = &NSDescriptorSetLayout;
                if(vkAllocateDescriptorSets(LDevice,&allocateinfo,&NSDescriptorSets[i][j])!=VK_SUCCESS){
                    throw std::runtime_error("failed to allocate neighborhood searcher desciptorset!");
                }
            }
        }

        VkDescriptorBufferInfo nsbufferinfo{};
        nsbufferinfo.buffer = UniformNSBuffer;
        nsbufferinfo.offset = 0;
        nsbufferinfo.range = sizeof(UniformNSObject);

        VkDescriptorBufferInfo sortedidxbufferinfo[2]{};
        sortedidxbufferinfo[0].buffer = RadixsortedIndexBuffer[0];
        sortedidxbufferinfo[0].offset = 0;
        sortedidxbufferinfo[0].range = sizeof(uint32_t)*particles.size();
        sortedidxbufferinfo[1].buffer = RadixsortedIndexBuffer[1];
        sortedidxbufferinfo[1].offset = 0;
        sortedidxbufferinfo[1].range = sizeof(uint32_t)*particles.size();

        VkDescriptorBufferInfo pngbrebufferinfo{};
        pngbrebufferinfo.buffer = ParticleNgbrBuffer;
        pngbrebufferinfo.offset = 0;
        pngbrebufferinfo.range = MAX_NGBR_NUM*particles.size()*sizeof(uint32_t);

        VkDescriptorBufferInfo rsbucketbufferinfo{};
        rsbucketbufferinfo.buffer = RSGlobalBucketBuffer;
        rsbucketbufferinfo.offset = 0;
        rsbucketbufferinfo.range = sizeof(uint32_t)*16*(WORK_GROUP_COUNT+1);

        VkDescriptorBufferInfo cellinfobufferinfo{};
        cellinfobufferinfo.buffer = CellinfoBuffer;
        cellinfobufferinfo.offset = 0;
        cellinfobufferinfo.range = sizeof(uint32_t)*4*particles.size();

        VkDescriptorBufferInfo localprefixbufferinfo{};
        localprefixbufferinfo.buffer = LocalPrefixBuffer;
        localprefixbufferinfo.offset = 0;
        localprefixbufferinfo.range = sizeof(uint32_t)*16*particles.size();

        std::array<VkWriteDescriptorSet,8> writes{};
        writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[0].descriptorCount = 1;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writes[0].dstArrayElement = 0;
        writes[0].dstBinding = 0;
        writes[0].pBufferInfo = &nsbufferinfo;

        writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[1].descriptorCount = 1;
        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[1].dstArrayElement = 0;
        writes[1].dstBinding = 1;

        writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[2].descriptorCount = 1;
        writes[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[2].dstArrayElement = 0;
        writes[2].dstBinding = 2;

        writes[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[3].descriptorCount = 1;
        writes[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[3].dstArrayElement = 0;
        writes[3].dstBinding = 3;
        
        writes[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[4].descriptorCount = 1;
        writes[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[4].dstArrayElement = 0;
        writes[4].dstBinding = 4;
        writes[4].pBufferInfo = &pngbrebufferinfo;

        writes[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[5].descriptorCount = 1;
        writes[5].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[5].dstArrayElement = 0;
        writes[5].dstBinding = 5;
        writes[5].pBufferInfo = &rsbucketbufferinfo;

        writes[6].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[6].descriptorCount = 1;
        writes[6].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[6].dstArrayElement = 0;
        writes[6].dstBinding = 6;
        writes[6].pBufferInfo = &cellinfobufferinfo;

        writes[7].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[7].descriptorCount = 1;
        writes[7].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[7].dstArrayElement = 0;
        writes[7].dstBinding = 7;
        writes[7].pBufferInfo = &localprefixbufferinfo;

        for(uint32_t i=0;i<2;++i){
            writes[1].pBufferInfo = &sortedidxbufferinfo[i];
            writes[2].pBufferInfo = &sortedidxbufferinfo[i^1];
            for(uint32_t j=0;j<MAXInFlightRendering;++j){
                VkDescriptorBufferInfo particlebufferinfo{};
                particlebufferinfo.buffer = ParticleBuffers[j];
                particlebufferinfo.offset = 0;
                particlebufferinfo.range = particles.size()*sizeof(Particle);
                writes[3].pBufferInfo = &particlebufferinfo;

                writes[0].dstSet = NSDescriptorSets[i][j];
                writes[1].dstSet = NSDescriptorSets[i][j];
                writes[2].dstSet = NSDescriptorSets[i][j];
                writes[3].dstSet = NSDescriptorSets[i][j];
                writes[4].dstSet = NSDescriptorSets[i][j];
                writes[5].dstSet = NSDescriptorSets[i][j];
                writes[6].dstSet = NSDescriptorSets[i][j];
                writes[7].dstSet = NSDescriptorSets[i][j];

                vkUpdateDescriptorSets(LDevice,static_cast<uint32_t>(writes.size()),writes.data(),0,nullptr);
            }
        }
    }

    {
        PostprocessDescriptorSets.resize(SwapChainImages.size());
        VkDescriptorBufferInfo renderingbufferinfo{};
        renderingbufferinfo.buffer = UniformRenderingBuffer;
        renderingbufferinfo.offset = 0;
        renderingbufferinfo.range = sizeof(UniformRenderingObject);
        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write.dstArrayElement = 0;
        write.dstBinding = 0;
        write.pBufferInfo = &renderingbufferinfo;
        for(uint32_t i=0;i<PostprocessDescriptorSets.size ();++i){
            VkDescriptorSetAllocateInfo allocateinfo{};
            allocateinfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocateinfo.descriptorPool = DescriptorPool;
            allocateinfo.descriptorSetCount = 1;
            allocateinfo.pSetLayouts = &PostprocessDescriptorSetLayout;
            if(vkAllocateDescriptorSets(LDevice,&allocateinfo,&PostprocessDescriptorSets[i])!=VK_SUCCESS){
                throw std::runtime_error("failed to allocate descritorset:postprocess!");
            }
            write.dstSet = PostprocessDescriptorSets[i];
            vkUpdateDescriptorSets(LDevice,1,&write,0,nullptr);
        }
    }

    {
        VkDescriptorSetAllocateInfo allocateinfo{};
        allocateinfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocateinfo.descriptorSetCount = 1;
        allocateinfo.descriptorPool = DescriptorPool;
        allocateinfo.pSetLayouts = &FilterDecsriptorSetLayout;
        if(vkAllocateDescriptorSets(LDevice,&allocateinfo,&FilterDescriptorSet)!=VK_SUCCESS){
            throw std::runtime_error("failed to allocate desciptorset:filter!");
        }
    }
    UpdateDescriptorSet();
}
void Renderer::CreateRenderPass()
{
    VkAttachmentDescription depthattachement{};
    depthattachement.format = VK_FORMAT_D32_SFLOAT;
    depthattachement.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    depthattachement.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthattachement.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthattachement.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthattachement.samples = VK_SAMPLE_COUNT_1_BIT;
    depthattachement.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthattachement.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    VkAttachmentDescription thickattachment{};
    thickattachment.format = VK_FORMAT_R32_SFLOAT;
    thickattachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    thickattachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    thickattachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    thickattachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    thickattachment.samples = VK_SAMPLE_COUNT_1_BIT;
    thickattachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    thickattachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    VkAttachmentDescription customdepthattachement{};
    customdepthattachement.format = VK_FORMAT_R32_SFLOAT;
    customdepthattachement.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    customdepthattachement.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    customdepthattachement.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    customdepthattachement.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    customdepthattachement.samples = VK_SAMPLE_COUNT_1_BIT;
    customdepthattachement.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    customdepthattachement.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    std::array<VkAttachmentDescription,3> attachments = {depthattachement,thickattachment,customdepthattachement};
    VkAttachmentReference thickattachment_ref{};
    thickattachment_ref.attachment = 1;
    thickattachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    VkAttachmentReference customdepthattachment_ref{};
    customdepthattachment_ref.attachment = 2;
    customdepthattachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthattachement_ref{};
    depthattachement_ref.attachment = 0;
    depthattachement_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    std::array<VkAttachmentReference,2> colorattachment_ref = {customdepthattachment_ref,thickattachment_ref};
    std::array<VkSubpassDescription,1> subpasses{};
    subpasses[0].colorAttachmentCount = static_cast<uint32_t>(colorattachment_ref.size());
    subpasses[0].pColorAttachments = colorattachment_ref.data();
    subpasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpasses[0].pDepthStencilAttachment  = &depthattachement_ref;
    
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

    std::array<VkDynamicState,4> dynamicstates = {VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR,VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE,VK_DYNAMIC_STATE_COLOR_BLEND_ENABLE_EXT};
    VkPipelineDynamicStateCreateInfo dynamicstate{};
    dynamicstate.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicstate.dynamicStateCount = static_cast<uint32_t>(dynamicstates.size());
    dynamicstate.pDynamicStates = dynamicstates.data();

    VkPipelineVertexInputStateCreateInfo vertexinput{};
    vertexinput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    auto vertexinputbinding = Particle::GetBinding();
    auto vertexinputattributs = Particle::GetAttributes();
    vertexinput.vertexBindingDescriptionCount = 1;
    vertexinput.pVertexBindingDescriptions = &vertexinputbinding;
    vertexinput.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexinputattributs.size());
    vertexinput.pVertexAttributeDescriptions = vertexinputattributs.data();
    
    VkPipelineInputAssemblyStateCreateInfo inputassmbly{};
    inputassmbly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputassmbly.primitiveRestartEnable = VK_FALSE;
    inputassmbly.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    
    VkPipelineViewportStateCreateInfo viewport{};
    viewport.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport.viewportCount = 1;
    viewport.scissorCount = 1;
    
    VkPipelineMultisampleStateCreateInfo multisample{};
    multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineRasterizationStateCreateInfo rasterization{};
    rasterization.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization.cullMode = VK_CULL_MODE_NONE;
    rasterization.depthClampEnable = VK_FALSE;
    rasterization.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterization.lineWidth = 1.0f;
    rasterization.polygonMode = VK_POLYGON_MODE_POINT;

    VkPipelineDepthStencilStateCreateInfo depthstencil{};
    depthstencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthstencil.depthTestEnable = VK_TRUE;
    depthstencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthstencil.depthWriteEnable = VK_TRUE;
    depthstencil.maxDepthBounds = 1.0f;
    depthstencil.minDepthBounds = 0.0f;
  


    VkPipelineColorBlendAttachmentState depthblendattachment{};
    depthblendattachment.blendEnable = VK_FALSE;
    depthblendattachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT;
    depthblendattachment.colorBlendOp = VK_BLEND_OP_MIN;
    depthblendattachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    depthblendattachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;

    VkPipelineColorBlendAttachmentState thickblendattachment{};
    thickblendattachment.blendEnable = VK_FALSE;
    thickblendattachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT;
    thickblendattachment.colorBlendOp = VK_BLEND_OP_ADD;
    thickblendattachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    thickblendattachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    thickblendattachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    thickblendattachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;

    std::array<VkPipelineColorBlendAttachmentState,2> colorblendattachments = {depthblendattachment,thickblendattachment};

    
    VkPipelineColorBlendStateCreateInfo colorblend{};
    colorblend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorblend.attachmentCount = static_cast<uint32_t>(colorblendattachments.size());
    colorblend.logicOpEnable = VK_FALSE;
    colorblend.pAttachments = colorblendattachments.data();

   
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
    VkPipelineLayoutCreateInfo nscreateinfo{};
    nscreateinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    nscreateinfo.pSetLayouts = &NSDescriptorSetLayout;
    nscreateinfo.setLayoutCount = 1;
    if(vkCreatePipelineLayout(LDevice,&nscreateinfo,Allocator,&NSPipelineLayout)!=VK_SUCCESS){
        throw std::runtime_error("failed to create neighborhood searcher pipeline layout!");
    }
    VkPipelineLayoutCreateInfo simulatecreateinfo{};
    simulatecreateinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    simulatecreateinfo.pSetLayouts = &SimulateDescriptorSetLayout;
    simulatecreateinfo.setLayoutCount = 1;
    if(vkCreatePipelineLayout(LDevice,&simulatecreateinfo,Allocator,&SimulatePipelineLayout)!=VK_SUCCESS){
        throw std::runtime_error("failed to create simulate pipeline layout!");
    }
    VkPipelineLayoutCreateInfo postprocesscreateinfo{};
    postprocesscreateinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    postprocesscreateinfo.pSetLayouts = &PostprocessDescriptorSetLayout;
    postprocesscreateinfo.setLayoutCount = 1;
    if(vkCreatePipelineLayout(LDevice,&postprocesscreateinfo,Allocator,&PostprocessPipelineLayout)!=VK_SUCCESS){
        throw std::runtime_error("failed to create postprocessing compute pipeline layout!");
    }
    VkPipelineLayoutCreateInfo filtercreateinfo{};
    filtercreateinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    filtercreateinfo.pSetLayouts = &FilterDecsriptorSetLayout;
    filtercreateinfo.setLayoutCount = 1;
    if(vkCreatePipelineLayout(LDevice,&filtercreateinfo,Allocator,&FilterPipelineLayout)!=VK_SUCCESS){
        throw std::runtime_error("failed to create filtering compute pipeline layout!");
    }
}
void Renderer::CreateComputePipeline()
{
    {
        //NGBR PIPELINES
        auto computeshadermodule_calcellhash = MakeShaderModule("shaders/spv/compshader_calcellhash.spv");
        auto computeshadermodule_radixsort1 = MakeShaderModule("shaders/spv/compshader_radixsort1.spv");
        auto computeshadermodule_radixsort2 = MakeShaderModule("shaders/spv/compshader_radixsort2.spv");
        auto computeshadermodule_radixsort3 = MakeShaderModule("shaders/spv/compshader_radixsort3.spv");
        auto computeshadermodule_fixcellbuffer = MakeShaderModule("shaders/spv/compshader_fixcellbuffer.spv");
        auto computeshadermodule_getngbrs = MakeShaderModule("shaders/spv/compshader_getngbrs.spv");

        std::vector<VkShaderModule> shadermodules = {computeshadermodule_calcellhash,computeshadermodule_radixsort1,computeshadermodule_radixsort2,
        computeshadermodule_radixsort3,computeshadermodule_fixcellbuffer,computeshadermodule_getngbrs};
        std::vector<VkPipeline*> pcomputepipelines = {&NSPipeline_CalcellHash,&NSPipeline_Radixsort1,&NSPipeline_Radixsort2,
        &NSPipeline_Radixsort3,&NSPipeline_FixcellBuffer,&NSPipeline_GetNgbrs}; 
        
        for(uint32_t i=0;i<shadermodules.size();++i){
            VkPipelineShaderStageCreateInfo stageinfo{};
            stageinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            stageinfo.pName = "main";
            stageinfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
            stageinfo.module = shadermodules[i];
            VkComputePipelineCreateInfo createinfo{};
            createinfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
            createinfo.layout = NSPipelineLayout;
            createinfo.stage = stageinfo;
            if(vkCreateComputePipelines(LDevice,VK_NULL_HANDLE,1,&createinfo,Allocator,pcomputepipelines[i])!=VK_SUCCESS){
                throw std::runtime_error("failed to create ns compute pipeline!");
            }
        }

        for(auto& computershadermodule:shadermodules){
            vkDestroyShaderModule(LDevice,computershadermodule,Allocator);
        }
    }

    {
        //SIMULATING PIPELINES
        auto computershadermodule_euler = MakeShaderModule("shaders/spv/compshader_euler.spv");
        auto computershadermodule_lambda = MakeShaderModule("shaders/spv/compshader_lambda.spv");
        auto computershadermodule_deltaposition = MakeShaderModule("shaders/spv/compshader_deltaposition.spv");
        auto computershadermodule_positionupd = MakeShaderModule("shaders/spv/compshader_positionupd.spv");
        auto computershadermodule_velocityupd = MakeShaderModule("shaders/spv/compshader_velocityupd.spv");
        auto computershadermodule_velocitycache = MakeShaderModule("shaders/spv/compshader_velocitycache.spv");
        auto computershadermodule_viscositycorr = MakeShaderModule("shaders/spv/compshader_viscositycorr.spv");
        auto computershadermodule_vorticitycorr = MakeShaderModule("shaders/spv/compshader_vorticitycorr.spv");

        std::vector<VkShaderModule> shadermodules = {computershadermodule_euler,computershadermodule_lambda,computershadermodule_deltaposition,
        computershadermodule_positionupd,computershadermodule_velocityupd,computershadermodule_velocitycache,
        computershadermodule_viscositycorr,computershadermodule_vorticitycorr};
        std::vector<VkPipeline*> pcomputepipelines = {&SimulatePipeline_Euler,&SimulatePipeline_Lambda,&SimulatePipeline_DeltaPosition,
        &SimulatePipeline_PositionUpd,&SimulatePipeline_VelocityUpd, 
        &SimulatePipeline_VelocityCache,&SimulatePipeline_ViscosityCorr, &SimulatePipeline_VorticityCorr};

        for(uint32_t i=0;i<shadermodules.size();++i){
            VkPipelineShaderStageCreateInfo stageinfo{};
            stageinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            stageinfo.pName = "main";
            stageinfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
            stageinfo.module = shadermodules[i];
            VkComputePipelineCreateInfo createinfo{};
            createinfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
            createinfo.layout = SimulatePipelineLayout;
            createinfo.stage = stageinfo;
            if(vkCreateComputePipelines(LDevice,VK_NULL_HANDLE,1,&createinfo,Allocator,pcomputepipelines[i])!=VK_SUCCESS){
                throw std::runtime_error("failed to create simulating compute pipeline!");
            }
        }

        for(auto& computershadermodule:shadermodules){
            vkDestroyShaderModule(LDevice,computershadermodule,Allocator);
        }
    }
    {
        //POSTPROCESSING PIPELINES
        auto computershadermodule_postprocessing = MakeShaderModule("shaders/spv/compshader_postprocessing.spv");
        auto computershadermodule_filtering = MakeShaderModule("shaders/spv/compshader_filtering.spv");

        VkPipelineShaderStageCreateInfo postprecessing_stageinfo{};
        postprecessing_stageinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        postprecessing_stageinfo.pName = "main";
        postprecessing_stageinfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        postprecessing_stageinfo.module = computershadermodule_postprocessing;
        VkComputePipelineCreateInfo rcreateinfo{};
        rcreateinfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        rcreateinfo.layout = PostprocessPipelineLayout;
        rcreateinfo.stage = postprecessing_stageinfo;
        if(vkCreateComputePipelines(LDevice,VK_NULL_HANDLE,1,&rcreateinfo,Allocator,&PostprocessPipeline)!=VK_SUCCESS){
            throw std::runtime_error("failed to create postprocess pipeline!");
        }

        VkPipelineShaderStageCreateInfo filtering_stageinfo{};
        filtering_stageinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        filtering_stageinfo.pName = "main";
        filtering_stageinfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        filtering_stageinfo.module = computershadermodule_filtering;
        VkComputePipelineCreateInfo fcreateinfo{};
        fcreateinfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        fcreateinfo.layout = FilterPipelineLayout;
        fcreateinfo.stage = filtering_stageinfo;
        if(vkCreateComputePipelines(LDevice,VK_NULL_HANDLE,1,&fcreateinfo,Allocator,&FilterPipeline)!=VK_SUCCESS){
            throw std::runtime_error("failed to create filter pipeline!");
        }
        
        vkDestroyShaderModule(LDevice,computershadermodule_postprocessing,Allocator);
        vkDestroyShaderModule(LDevice,computershadermodule_filtering,Allocator);
    }
}
void Renderer::CreateFramebuffers()
{
    VkFramebufferCreateInfo framebufferinfo{};
    std::array<VkImageView,3> attachments = {DepthImageView,ThickImageView,CustomDepthImageView};
    framebufferinfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferinfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    framebufferinfo.pAttachments = attachments.data();
    int w,h;
    glfwGetFramebufferSize(Window,&w,&h);
    framebufferinfo.width = w;
    framebufferinfo.height = h;
    framebufferinfo.layers = 1;
    framebufferinfo.renderPass = GraphicRenderPass;
    
    if(vkCreateFramebuffer(LDevice,&framebufferinfo,Allocator,&Framebuffer)!=VK_SUCCESS){
        throw std::runtime_error("failed to create framebuffer!");
    }

}
void Renderer::RecordSimulatingCommandBuffers()
{
    SimulatingCommandBuffers.resize(MAXInFlightRendering);
    for(uint32_t i=0;i<MAXInFlightRendering;++i){
        VkCommandBufferAllocateInfo allocateinfo{};
        allocateinfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocateinfo.commandPool = CommandPool;
        allocateinfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocateinfo.commandBufferCount = 1;
        if(vkAllocateCommandBuffers(LDevice,&allocateinfo,&SimulatingCommandBuffers[i])!=VK_SUCCESS){
            throw std::runtime_error("failed to allocate simulating command buffer!");
        }
        VkCommandBufferBeginInfo begininfo{};
        begininfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begininfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

        if(vkBeginCommandBuffer(SimulatingCommandBuffers[i],&begininfo)!=VK_SUCCESS){
            throw std::runtime_error("failed to begin simulating command buffer!");
        }
        VkMemoryBarrier memorybarrier{};
        memorybarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        memorybarrier.srcAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
        memorybarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        vkCmdPipelineBarrier(SimulatingCommandBuffers[i],VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,0,1,&memorybarrier,0,nullptr,0,nullptr);
        memorybarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        memorybarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT|VK_ACCESS_SHADER_WRITE_BIT;

        
        vkCmdBindDescriptorSets(SimulatingCommandBuffers[i],VK_PIPELINE_BIND_POINT_COMPUTE,SimulatePipelineLayout,0,1,&SimulateDescriptorSet[i],0,nullptr);
        vkCmdPipelineBarrier(SimulatingCommandBuffers[i],VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,0,1,&memorybarrier,0,nullptr,0,nullptr);
        vkCmdBindPipeline(SimulatingCommandBuffers[i],VK_PIPELINE_BIND_POINT_COMPUTE,SimulatePipeline_Euler);
        vkCmdDispatch(SimulatingCommandBuffers[i],WORK_GROUP_COUNT,1,1);
        
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        //                  SEARCHING NEIGHBORS
        ////////////////////////////////////////////////////////////////////////////////////////////////////
        vkCmdBindPipeline(SimulatingCommandBuffers[i],VK_PIPELINE_BIND_POINT_COMPUTE,NSPipeline_CalcellHash);
        vkCmdBindDescriptorSets(SimulatingCommandBuffers[i],VK_PIPELINE_BIND_POINT_COMPUTE,NSPipelineLayout,0,1,&NSDescriptorSets[0][i],0,nullptr);
        vkCmdPipelineBarrier(SimulatingCommandBuffers[i],VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,0,1,&memorybarrier,0,nullptr,0,nullptr);
        vkCmdDispatch(SimulatingCommandBuffers[i],WORK_GROUP_COUNT,1,1);
        
        for(uint32_t iter=0;iter<8;++iter){
            vkCmdBindDescriptorSets(SimulatingCommandBuffers[i],VK_PIPELINE_BIND_POINT_COMPUTE,NSPipelineLayout,0,1,&NSDescriptorSets[iter%2][i],0,nullptr);

            vkCmdBindPipeline(SimulatingCommandBuffers[i],VK_PIPELINE_BIND_POINT_COMPUTE,NSPipeline_Radixsort1);
            vkCmdPipelineBarrier(SimulatingCommandBuffers[i],VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,0,1,&memorybarrier,0,nullptr,0,nullptr);
            vkCmdDispatch(SimulatingCommandBuffers[i],WORK_GROUP_COUNT,1,1);

            vkCmdBindPipeline(SimulatingCommandBuffers[i],VK_PIPELINE_BIND_POINT_COMPUTE,NSPipeline_Radixsort2);
            vkCmdPipelineBarrier(SimulatingCommandBuffers[i],VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,0,1,&memorybarrier,0,nullptr,0,nullptr);
            vkCmdDispatch(SimulatingCommandBuffers[i],1,1,1);

            vkCmdBindPipeline(SimulatingCommandBuffers[i],VK_PIPELINE_BIND_POINT_COMPUTE,NSPipeline_Radixsort3);
            vkCmdPipelineBarrier(SimulatingCommandBuffers[i],VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,0,1,&memorybarrier,0,nullptr,0,nullptr);
            vkCmdDispatch(SimulatingCommandBuffers[i],WORK_GROUP_COUNT,1,1);

        }
        
        vkCmdBindPipeline(SimulatingCommandBuffers[i],VK_PIPELINE_BIND_POINT_COMPUTE,NSPipeline_FixcellBuffer);
        vkCmdPipelineBarrier(SimulatingCommandBuffers[i],VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,0,1,&memorybarrier,0,nullptr,0,nullptr);
        vkCmdDispatch(SimulatingCommandBuffers[i],WORK_GROUP_COUNT,1,1);

        vkCmdBindPipeline(SimulatingCommandBuffers[i],VK_PIPELINE_BIND_POINT_COMPUTE,NSPipeline_GetNgbrs);
        vkCmdPipelineBarrier(SimulatingCommandBuffers[i],VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,0,1,&memorybarrier,0,nullptr,0,nullptr);
        vkCmdDispatch(SimulatingCommandBuffers[i],WORK_GROUP_COUNT,1,1);
        ////////////////////////////////////////////////////////////////////////////////////////////////////

        vkCmdBindDescriptorSets(SimulatingCommandBuffers[i],VK_PIPELINE_BIND_POINT_COMPUTE,SimulatePipelineLayout,0,1,&SimulateDescriptorSet[i],0,nullptr);

        for(int iter=0;iter<4;++iter){
            vkCmdPipelineBarrier(SimulatingCommandBuffers[i],VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,0,1,&memorybarrier
            ,0,nullptr,0,nullptr);
            vkCmdBindPipeline(SimulatingCommandBuffers[i],VK_PIPELINE_BIND_POINT_COMPUTE,SimulatePipeline_Lambda);
            vkCmdDispatch(SimulatingCommandBuffers[i],WORK_GROUP_COUNT,1,1);

            vkCmdPipelineBarrier(SimulatingCommandBuffers[i],VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,0,1,&memorybarrier
            ,0,nullptr,0,nullptr);
            vkCmdBindPipeline(SimulatingCommandBuffers[i],VK_PIPELINE_BIND_POINT_COMPUTE,SimulatePipeline_DeltaPosition);
            vkCmdDispatch(SimulatingCommandBuffers[i],WORK_GROUP_COUNT,1,1);    

            vkCmdPipelineBarrier(SimulatingCommandBuffers[i],VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,0,1,&memorybarrier
            ,0,nullptr,0,nullptr);
            vkCmdBindPipeline(SimulatingCommandBuffers[i],VK_PIPELINE_BIND_POINT_COMPUTE,SimulatePipeline_PositionUpd);
            vkCmdDispatch(SimulatingCommandBuffers[i],WORK_GROUP_COUNT,1,1); 
        }
        vkCmdPipelineBarrier(SimulatingCommandBuffers[i],VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,0,1,&memorybarrier
        ,0,nullptr,0,nullptr);
        vkCmdBindPipeline(SimulatingCommandBuffers[i],VK_PIPELINE_BIND_POINT_COMPUTE,SimulatePipeline_VelocityUpd);
        vkCmdDispatch(SimulatingCommandBuffers[i],WORK_GROUP_COUNT,1,1);   

        vkCmdPipelineBarrier(SimulatingCommandBuffers[i],VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,0,1,&memorybarrier
        ,0,nullptr,0,nullptr);
        vkCmdBindPipeline(SimulatingCommandBuffers[i],VK_PIPELINE_BIND_POINT_COMPUTE,SimulatePipeline_VelocityCache);
        vkCmdDispatch(SimulatingCommandBuffers[i],WORK_GROUP_COUNT,1,1);  

        vkCmdPipelineBarrier(SimulatingCommandBuffers[i],VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,0,1,&memorybarrier
        ,0,nullptr,0,nullptr);
        vkCmdBindPipeline(SimulatingCommandBuffers[i],VK_PIPELINE_BIND_POINT_COMPUTE,SimulatePipeline_ViscosityCorr);
        vkCmdDispatch(SimulatingCommandBuffers[i],WORK_GROUP_COUNT,1,1);  

        vkCmdPipelineBarrier(SimulatingCommandBuffers[i],VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,0,1,&memorybarrier
        ,0,nullptr,0,nullptr);
        vkCmdBindPipeline(SimulatingCommandBuffers[i],VK_PIPELINE_BIND_POINT_COMPUTE,SimulatePipeline_VelocityCache);
        vkCmdDispatch(SimulatingCommandBuffers[i],WORK_GROUP_COUNT,1,1);  

        vkCmdPipelineBarrier(SimulatingCommandBuffers[i],VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,0,1,&memorybarrier
        ,0,nullptr,0,nullptr);
        vkCmdBindPipeline(SimulatingCommandBuffers[i],VK_PIPELINE_BIND_POINT_COMPUTE,SimulatePipeline_VorticityCorr);
        vkCmdDispatch(SimulatingCommandBuffers[i],WORK_GROUP_COUNT,1,1); 

        auto result = vkEndCommandBuffer(SimulatingCommandBuffers[i]);
        if(result != VK_SUCCESS){
            throw std::runtime_error("failed to end simulating command buffer!");
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

    VkPhysicalDeviceShaderObjectFeaturesEXT so{};
    so.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_OBJECT_FEATURES_EXT;
    VkPhysicalDeviceExtendedDynamicState3FeaturesEXT ed3{};
    ed3.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_3_FEATURES_EXT;
    ed3.pNext = &so;
    VkPhysicalDeviceExtendedDynamicStateFeaturesEXT ed{};
    ed.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT;
    ed.pNext = &ed3;
    
    
    VkPhysicalDeviceFeatures2 features2{};
    features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    features2.pNext = &ed;
    vkGetPhysicalDeviceFeatures2(pdevice,&features2);
    if(ed.extendedDynamicState != VK_TRUE) return false;
    if(ed3.extendedDynamicState3ColorBlendEnable != VK_TRUE) return false;
    if(so.shaderObject !=VK_TRUE) return false;
    if(features2.features.samplerAnisotropy != VK_TRUE) return false;
    if(features2.features.fillModeNonSolid != VK_TRUE) return false;
    return true;
}
void Renderer::GetRequestDeviceExts(std::vector<const char *>& exts)
{
    exts.resize(0);
    exts.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    exts.push_back(VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME);
    exts.push_back(VK_EXT_SHADER_OBJECT_EXTENSION_NAME);
    exts.push_back(VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME);
    exts.push_back("VK_KHR_dynamic_rendering");
    exts.push_back("VK_KHR_depth_stencil_resolve");
    exts.push_back("VK_KHR_create_renderpass2");
}
void Renderer::GetRequestDeviceFeature(VkPhysicalDeviceFeatures& features)
{
    features = VkPhysicalDeviceFeatures{};
    features.samplerAnisotropy = VK_TRUE;
    features.fillModeNonSolid = VK_TRUE;
    features.independentBlend = VK_TRUE;

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
        VkFormatProperties formatProperties;
        vkGetPhysicalDeviceFormatProperties(PDevice,format.format,&formatProperties);
        if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT)) {
           continue;
        }
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
TickWindowResult Renderer::TickWindow(float DeltaTime)
{
    if(glfwWindowShouldClose(Window)) return TickWindowResult::EXIT;
    glfwPollEvents();
    int w,h;
    glfwGetFramebufferSize(Window,&w,&h);
    if(w*h == 0){
        return TickWindowResult::HIDE;
    }
    return TickWindowResult::NONE;
}
void Renderer::Simulate()
{
    CurrentFlight = (CurrentFlight + 1)%MAXInFlightRendering; 
    
    VkSubmitInfo submitinfo{};
    submitinfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitinfo.commandBufferCount = 1;
    submitinfo.pCommandBuffers = &SimulatingCommandBuffers[CurrentFlight];
    submitinfo.signalSemaphoreCount = 1;
    submitinfo.pSignalSemaphores = &SimulatingFinish;

    if(vkQueueSubmit(GraphicNComputeQueue,1,&submitinfo,VK_NULL_HANDLE)!=VK_SUCCESS){
        throw std::runtime_error("failed to submit simulating command buffer!");
    }
}
void Renderer::Draw()
{
    uint64_t notimeout = UINT64_MAX;
    VkResult result;
    
    uint32_t image_idx;
    
    vkWaitForFences(LDevice,1,&DrawingFence,VK_TRUE,notimeout);
    vkResetFences(LDevice,1,&DrawingFence);

    result = vkAcquireNextImageKHR(LDevice,SwapChain,notimeout,ImageAvaliable,VK_NULL_HANDLE,&image_idx);

    auto cb_graphic = CreateCommandBuffer();
    VkRenderPassBeginInfo renderpass_begininfo{};
    renderpass_begininfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderpass_begininfo.framebuffer = Framebuffer;
    std::array<VkClearValue,3> clearvalues{};
    clearvalues[0].depthStencil = {{1}};
    clearvalues[1].color = {{0,0,0,0}};
    clearvalues[2].color = {{1000,0,0,0}};
    renderpass_begininfo.clearValueCount = static_cast<uint32_t>(clearvalues.size());
    renderpass_begininfo.pClearValues = clearvalues.data();
    renderpass_begininfo.renderPass = GraphicRenderPass;
    renderpass_begininfo.renderArea.extent = SwapChainImageExtent;
    renderpass_begininfo.renderArea.offset = {0,0};
    vkCmdBindDescriptorSets(cb_graphic,VK_PIPELINE_BIND_POINT_GRAPHICS,GraphicPipelineLayout,0,1,&GraphicDescriptorSet,0,nullptr);
    vkCmdBindPipeline(cb_graphic,VK_PIPELINE_BIND_POINT_GRAPHICS,GraphicPipeline);
    vkCmdBeginRenderPass(cb_graphic,&renderpass_begininfo,VK_SUBPASS_CONTENTS_INLINE);
    
    VkViewport viewport;
    viewport.height = SwapChainImageExtent.height;
    viewport.width = SwapChainImageExtent.width;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    viewport.x = viewport.y = 0;
    VkRect2D scissor;
    scissor.offset = {0,0};
    scissor.extent = SwapChainImageExtent;
    vkCmdSetViewport(cb_graphic,0,1,&viewport);
    vkCmdSetScissor(cb_graphic,0,1,&scissor);
    vkCmdSetDepthTestEnable(cb_graphic,VK_TRUE);
    std::array<VkBool32,2> blendenables = {VK_TRUE,VK_TRUE};
    ExtensionFuncs::vkCmdSetColorBlendEnableEXT(LDevice,cb_graphic,0,static_cast<uint32_t>(blendenables.size()),blendenables.data());
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cb_graphic,0,1,&ParticleBuffers[CurrentFlight],&offset);
    
    vkCmdDraw(cb_graphic,particles.size(),1,0,0);
    vkCmdEndRenderPass(cb_graphic);

    VkSubmitInfo graphic_submitinfo{};    
    graphic_submitinfo.pSignalSemaphores = &DepthNThickAvaliable;;
    graphic_submitinfo.signalSemaphoreCount = 1;
    std::array<VkPipelineStageFlags,1> waitstages = {VK_PIPELINE_STAGE_VERTEX_INPUT_BIT};
    std::array<VkSemaphore,1> waitsems = {SimulatingFinish};
    graphic_submitinfo.waitSemaphoreCount = static_cast<uint32_t>(waitsems.size());
    graphic_submitinfo.pWaitSemaphores = waitsems.data();
    graphic_submitinfo.pWaitDstStageMask = waitstages.data();

    SubmitCommandBuffer(cb_graphic,graphic_submitinfo,VK_NULL_HANDLE,GraphicNComputeQueue);

    auto cb_compute = CreateCommandBuffer();

    VkImageMemoryBarrier imagebarrier{};
    imagebarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imagebarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imagebarrier.subresourceRange.baseArrayLayer = 0;
    imagebarrier.subresourceRange.baseMipLevel = 0;
    imagebarrier.subresourceRange.layerCount = 1;
    imagebarrier.subresourceRange.levelCount = 1;
    VkMemoryBarrier memorybarrier{};
    memorybarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;

    //DEPTH TEXTURE FILTERING

    imagebarrier.image = FilteredDepthImage;
    imagebarrier.srcAccessMask = 0;
    imagebarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    imagebarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imagebarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    vkCmdPipelineBarrier(cb_compute,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,0,0,nullptr,0,nullptr,1,&imagebarrier);

    vkCmdBindPipeline(cb_compute,VK_PIPELINE_BIND_POINT_COMPUTE,FilterPipeline);
    vkCmdBindDescriptorSets(cb_compute,VK_PIPELINE_BIND_POINT_COMPUTE,FilterPipelineLayout,0,1,&FilterDescriptorSet,0,nullptr);
    vkCmdDispatch(cb_compute,SwapChainImageExtent.width/4,SwapChainImageExtent.height/4,1);
    imagebarrier.image = FilteredDepthImage;
    imagebarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    imagebarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    imagebarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
    imagebarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    vkCmdPipelineBarrier(cb_compute,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,0,0,nullptr,0,nullptr,1,&imagebarrier);
    
    vkCmdBindPipeline(cb_compute,VK_PIPELINE_BIND_POINT_COMPUTE,PostprocessPipeline);
    vkCmdBindDescriptorSets(cb_compute,VK_PIPELINE_BIND_POINT_COMPUTE,PostprocessPipelineLayout,0,1,&PostprocessDescriptorSets[image_idx],0,nullptr);
    imagebarrier.image = SwapChainImages[image_idx];
    imagebarrier.srcAccessMask = 0;
    imagebarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    imagebarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imagebarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    vkCmdPipelineBarrier(cb_compute,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,0,0,nullptr,0,nullptr,1,&imagebarrier);
    vkCmdDispatch(cb_compute,SwapChainImageExtent.width/4,SwapChainImageExtent.height/4,1);

    VkSubmitInfo compute_submitinfo{};
    std::array<VkSemaphore,2> comp_waitsems = {ImageAvaliable,DepthNThickAvaliable};
    std::array<VkPipelineStageFlags,2> comp_waitstages = {VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT};
    compute_submitinfo.waitSemaphoreCount = static_cast<uint32_t>(comp_waitsems.size());
    compute_submitinfo.pWaitSemaphores = comp_waitsems.data();
    compute_submitinfo.pWaitDstStageMask = comp_waitstages.data();
    compute_submitinfo.pSignalSemaphores = &RenderingFinish;
    compute_submitinfo.signalSemaphoreCount = 1;
    SubmitCommandBuffer(cb_compute,compute_submitinfo,DrawingFence,GraphicNComputeQueue);

    auto cb_present = CreateCommandBuffer();
    imagebarrier.image = SwapChainImages[image_idx];
    imagebarrier.srcAccessMask = 0;
    imagebarrier.dstAccessMask = 0;
    imagebarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
    imagebarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    vkCmdPipelineBarrier(cb_present,VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,0,0,nullptr,0,nullptr,1,&imagebarrier);
    VkSubmitInfo present_submitinfo{};
    SubmitCommandBuffer(cb_present,present_submitinfo,VK_NULL_HANDLE,PresentQueue);

    VkPresentInfoKHR presentinfo{};
    presentinfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentinfo.swapchainCount = 1;
    presentinfo.pSwapchains = &SwapChain;
    presentinfo.pWaitSemaphores = &RenderingFinish;
    presentinfo.waitSemaphoreCount = 1;
    presentinfo.pImageIndices = &image_idx;
    result = vkQueuePresentKHR(PresentQueue,&presentinfo);
    if(result != VK_SUCCESS||bFramebufferResized){
        RecreateSwapChain();
        return;
    }

}

void Renderer::WaitIdle()
{
    vkDeviceWaitIdle(LDevice);
}
