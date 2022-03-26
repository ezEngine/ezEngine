#include <RendererVulkan/RendererVulkanPCH.h>

#include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Profiling/Profiling.h>
#include <RendererFoundation/Device/DeviceFactory.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <RendererVulkan/Cache/ResourceCacheVulkan.h>
#include <RendererVulkan/CommandEncoder/CommandEncoderImplVulkan.h>
#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Device/PassVulkan.h>
#include <RendererVulkan/Device/SwapChainVulkan.h>
#include <RendererVulkan/Pools/CommandBufferPoolVulkan.h>
#include <RendererVulkan/Pools/FencePoolVulkan.h>
#include <RendererVulkan/Pools/SemaphorePoolVulkan.h>
#include <RendererVulkan/Resources/BufferVulkan.h>
#include <RendererVulkan/Resources/QueryVulkan.h>
#include <RendererVulkan/Resources/RenderTargetViewVulkan.h>
#include <RendererVulkan/Resources/ResourceViewVulkan.h>
#include <RendererVulkan/Resources/TextureVulkan.h>
#include <RendererVulkan/Resources/UnorderedAccessViewVulkan.h>
#include <RendererVulkan/Shader/ShaderVulkan.h>
#include <RendererVulkan/Shader/VertexDeclarationVulkan.h>
#include <RendererVulkan/State/StateVulkan.h>

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
  switch (messageSeverity)
  {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
      ezLog::Debug("VK Verbose: {}", pCallbackData->pMessage);
      break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
      ezLog::Info("VK Info: {}", pCallbackData->pMessage);
      break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
      ezLog::Warning("VK warning: {}", pCallbackData->pMessage);
      break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
      ezLog::Error("VK error: {}", pCallbackData->pMessage);
      break;
    default:
      break;
  }
  // Only layers are allowed to return true here.
  return VK_FALSE;
}

ezInt32 getGraphicsAndComputeQueue(ezArrayPtr<vk::QueueFamilyProperties> queueFamilyProperties)
{
  for (ezUInt32 i = 0; i < queueFamilyProperties.GetCount(); ++i)
  {
    const vk::QueueFamilyProperties& queueFamilyProperty = queueFamilyProperties[i];
    if (queueFamilyProperty.queueCount > 0 &&
        (queueFamilyProperty.queueFlags & vk::QueueFlagBits::eGraphics) &&
        (queueFamilyProperty.queueFlags & vk::QueueFlagBits::eCompute))
    {
      return i;
    }
  }
  return -1;
}

// Need to implement these extension functions so vulkan hpp can call them.
// They're basically just adapters calling the function pointer retrieved previously.

PFN_vkCmdDebugMarkerBeginEXT vkCmdDebugMarkerBeginEXTFunc;
PFN_vkCmdDebugMarkerEndEXT vkCmdDebugMarkerEndEXTFunc;
PFN_vkCmdDebugMarkerInsertEXT vkCmdDebugMarkerInsertEXTFunc;
PFN_vkDebugMarkerSetObjectNameEXT vkDebugMarkerSetObjectNameEXTFunc;
PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXTFunc;

void vkCmdDebugMarkerBeginEXT(VkCommandBuffer commandBuffer, const VkDebugMarkerMarkerInfoEXT* pMarkerInfo)
{
  vkCmdDebugMarkerBeginEXTFunc(commandBuffer, pMarkerInfo);
}

void vkCmdDebugMarkerEndEXT(VkCommandBuffer commandBuffer)
{
  vkCmdDebugMarkerEndEXTFunc(commandBuffer);
}

void vkCmdDebugMarkerInsertEXT(VkCommandBuffer commandBuffer, const VkDebugMarkerMarkerInfoEXT* pMarkerInfo)
{
  vkCmdDebugMarkerInsertEXTFunc(commandBuffer, pMarkerInfo);
}

VkResult vkDebugMarkerSetObjectNameEXT(VkDevice device, const VkDebugMarkerObjectNameInfoEXT* pNameInfo)
{
  return vkDebugMarkerSetObjectNameEXTFunc(device, pNameInfo);
}

VkResult vkSetDebugUtilsObjectNameEXT(VkDevice device, const VkDebugUtilsObjectNameInfoEXT* pObjectName)
{
  return vkSetDebugUtilsObjectNameEXTFunc(device, pObjectName);
}

ezInternal::NewInstance<ezGALDevice> CreateVulkanDevice(ezAllocatorBase* pAllocator, const ezGALDeviceCreationDescription& Description)
{
  return EZ_NEW(pAllocator, ezGALDeviceVulkan, Description);
}

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(RendererVulkan, DeviceFactory)

ON_CORESYSTEMS_STARTUP
{
  ezGALDeviceFactory::RegisterCreatorFunc("Vulkan", &CreateVulkanDevice, "VULKAN", "ezShaderCompilerDXC");
}

ON_CORESYSTEMS_SHUTDOWN
{
  ezGALDeviceFactory::UnregisterCreatorFunc("Vulkan");
}

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

ezGALDeviceVulkan::ezGALDeviceVulkan(const ezGALDeviceCreationDescription& Description)
  : ezGALDevice(Description)
  , m_device(nullptr)
  //, m_pDebug(nullptr)
  , m_uiFrameCounter(0)
{
}

ezGALDeviceVulkan::~ezGALDeviceVulkan() = default;

// Init & shutdown functions


vk::Result ezGALDeviceVulkan::SelectInstanceExtensions(ezHybridArray<const char*, 6>& extensions)
{
  // Fetch the list of extensions supported by the runtime.
  ezUInt32 extensionCount;
  VK_SUCCEED_OR_RETURN_LOG(vk::enumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr));
  ezDynamicArray<vk::ExtensionProperties> extensionProperties;
  extensionProperties.SetCount(extensionCount);
  VK_SUCCEED_OR_RETURN_LOG(vk::enumerateInstanceExtensionProperties(nullptr, &extensionCount, extensionProperties.GetData()));

  EZ_LOG_BLOCK("InstanceExtensions");
  for (auto& ext : extensionProperties)
  {
    ezLog::Info("{}", ext.extensionName.data());
  }

  // Add a specific extension to the list of extensions to be enabled, if it is supported.
  auto AddExtIfSupported = [&](const char* extensionName, bool& enableFlag) -> vk::Result {
    auto it = std::find_if(begin(extensionProperties), end(extensionProperties), [&](const vk::ExtensionProperties& prop) { return ezStringUtils::IsEqual(prop.extensionName.data(), extensionName); });
    if (it != end(extensionProperties))
    {
      extensions.PushBack(extensionName);
      enableFlag = true;
      return vk::Result::eSuccess;
    }
    enableFlag = false;
    return vk::Result::eErrorExtensionNotPresent;
  };

  VK_SUCCEED_OR_RETURN_LOG(AddExtIfSupported(VK_KHR_SURFACE_EXTENSION_NAME, m_extensions.m_bSurface));
  VK_SUCCEED_OR_RETURN_LOG(AddExtIfSupported(VK_KHR_WIN32_SURFACE_EXTENSION_NAME, m_extensions.m_bWin32Surface));
  VK_SUCCEED_OR_RETURN_LOG(AddExtIfSupported(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, m_extensions.m_bDebugUtils));

  return vk::Result::eSuccess;
}


vk::Result ezGALDeviceVulkan::SelectDeviceExtensions(ezHybridArray<const char*, 6>& extensions)
{
  // Fetch the list of extensions supported by the runtime.
  ezUInt32 extensionCount;
  VK_SUCCEED_OR_RETURN_LOG(m_physicalDevice.enumerateDeviceExtensionProperties(nullptr, &extensionCount, nullptr));
  ezDynamicArray<vk::ExtensionProperties> extensionProperties;
  extensionProperties.SetCount(extensionCount);
  VK_SUCCEED_OR_RETURN_LOG(m_physicalDevice.enumerateDeviceExtensionProperties(nullptr, &extensionCount, extensionProperties.GetData()));

  EZ_LOG_BLOCK("DeviceExtensions");
  for (auto& ext : extensionProperties)
  {
    ezLog::Info("{}", ext.extensionName.data());
  }

  // Add a specific extension to the list of extensions to be enabled, if it is supported.
  auto AddExtIfSupported = [&](const char* extensionName, bool& enableFlag) -> vk::Result {
    auto it = std::find_if(begin(extensionProperties), end(extensionProperties), [&](const vk::ExtensionProperties& prop) { return ezStringUtils::IsEqual(prop.extensionName.data(), extensionName); });
    if (it != end(extensionProperties))
    {
      extensions.PushBack(extensionName);
      enableFlag = true;
      return vk::Result::eSuccess;
    }
    enableFlag = false;
    return vk::Result::eErrorExtensionNotPresent;
  };

  VK_SUCCEED_OR_RETURN_LOG(AddExtIfSupported(VK_KHR_SWAPCHAIN_EXTENSION_NAME, m_extensions.m_bDeviceSwapChain));

  return vk::Result::eSuccess;
}

#define EZ_GET_INSTANCE_PROC_ADDR(name) m_extensions.pfn_##name = reinterpret_cast<PFN_##name>(vkGetInstanceProcAddr(m_instance, #name));

ezResult ezGALDeviceVulkan::InitPlatform()
{
  EZ_LOG_BLOCK("ezGALDeviceVulkan::InitPlatform");

  const char* layers[] = {"VK_LAYER_KHRONOS_validation"};
  {
    // Create instance
    vk::ApplicationInfo applicationInfo = {};
    applicationInfo.apiVersion = VK_API_VERSION_1_0;
    applicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0); // TODO put ezEngine version here
    applicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);      // TODO put ezEngine version here
    applicationInfo.pApplicationName = "ezEngine";
    applicationInfo.pEngineName = "ezEngine";

    ezHybridArray<const char*, 6> instanceExtensions;
    VK_SUCCEED_OR_RETURN_EZ_FAILURE(SelectInstanceExtensions(instanceExtensions));

    vk::InstanceCreateInfo instanceCreateInfo;
    // enabling support for win32 surfaces
    instanceCreateInfo.pApplicationInfo = &applicationInfo;

    instanceCreateInfo.enabledExtensionCount = instanceExtensions.GetCount();
    instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.GetData();

    instanceCreateInfo.enabledLayerCount = 0;
#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugCreateInfo.messageSeverity = /*VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |*/ VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debugCreateInfo.pfnUserCallback = debugCallback;
    debugCreateInfo.pUserData = nullptr;

    instanceCreateInfo.enabledLayerCount = EZ_ARRAY_SIZE(layers);
    instanceCreateInfo.ppEnabledLayerNames = layers;
    instanceCreateInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
#endif

    m_instance = vk::createInstance(instanceCreateInfo);
#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
    EZ_GET_INSTANCE_PROC_ADDR(vkCreateDebugUtilsMessengerEXT);
    EZ_GET_INSTANCE_PROC_ADDR(vkDestroyDebugUtilsMessengerEXT);
    EZ_GET_INSTANCE_PROC_ADDR(vkSetDebugUtilsObjectNameEXT);
    VK_SUCCEED_OR_RETURN_EZ_FAILURE(m_extensions.pfn_vkCreateDebugUtilsMessengerEXT(m_instance, &debugCreateInfo, nullptr, &m_debugMessenger));
#endif

    if (!m_instance)
    {
      ezLog::Error("Failed to create Vulkan instance!");
      return EZ_FAILURE;
    }
  }

  {
    // physical device
    ezUInt32 physicalDeviceCount = 0;
    ezHybridArray<vk::PhysicalDevice, 2> physicalDevices;
    VK_SUCCEED_OR_RETURN_EZ_FAILURE(m_instance.enumeratePhysicalDevices(&physicalDeviceCount, nullptr));
    if (physicalDeviceCount == 0)
    {
      ezLog::Error("No available physical device to create a Vulkan device on!");
      return EZ_FAILURE;
    }

    physicalDevices.SetCount(physicalDeviceCount);
    VK_SUCCEED_OR_RETURN_EZ_FAILURE(m_instance.enumeratePhysicalDevices(&physicalDeviceCount, physicalDevices.GetData()));

    // TODO choosable physical device?
    // TODO making sure we have a hardware device?
    m_physicalDevice = physicalDevices[0];
    ezLog::Info("Selected physical device \"{}\" for device creation.", m_physicalDevice.getProperties().deviceName);
  }

  ezInt32 graphicsQueueIndex = 0;
  {
    // Device
    ezUInt32 queueFamilyPropertyCount = 0;
    ezHybridArray<vk::QueueFamilyProperties, 2> queueFamilyPoperties;
    m_physicalDevice.getQueueFamilyProperties(&queueFamilyPropertyCount, nullptr);
    if (queueFamilyPropertyCount == 0)
    {
      ezLog::Error("No available device queues on physical device!");
      return EZ_FAILURE;
    }
    queueFamilyPoperties.SetCount(queueFamilyPropertyCount);
    m_physicalDevice.getQueueFamilyProperties(&queueFamilyPropertyCount, queueFamilyPoperties.GetData());

    m_queueFamilyIndices.Clear();
    graphicsQueueIndex = getGraphicsAndComputeQueue(queueFamilyPoperties);
    m_queueFamilyIndices.PushBack(graphicsQueueIndex);

    constexpr float queuePriority = 0.f;
    vk::DeviceQueueCreateInfo deviceQueueCreateInfo = {};
    deviceQueueCreateInfo.pQueuePriorities = &queuePriority;
    deviceQueueCreateInfo.queueCount = 1;
    deviceQueueCreateInfo.queueFamilyIndex = graphicsQueueIndex;

    //#TODO_VULKAN test that this returns the same as 'layers' passed into the instance.
    ezUInt32 uiLayers;
    VK_SUCCEED_OR_RETURN_EZ_FAILURE(m_physicalDevice.enumerateDeviceLayerProperties(&uiLayers, nullptr));
    ezDynamicArray<vk::LayerProperties> deviceLayers;
    deviceLayers.SetCount(uiLayers);
    VK_SUCCEED_OR_RETURN_EZ_FAILURE(m_physicalDevice.enumerateDeviceLayerProperties(&uiLayers, deviceLayers.GetData()));

    ezHybridArray<const char*, 6> deviceExtensions;
    VK_SUCCEED_OR_RETURN_EZ_FAILURE(SelectDeviceExtensions(deviceExtensions));

    vk::DeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.enabledExtensionCount = deviceExtensions.GetCount();
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.GetData();
    // Device layers are deprecated but provided (same as in instance) for backwards compatibility.
    deviceCreateInfo.enabledLayerCount = EZ_ARRAY_SIZE(layers);
    deviceCreateInfo.ppEnabledLayerNames = layers;

    vk::PhysicalDeviceFeatures physicalDeviceFeatures = m_physicalDevice.getFeatures();
    deviceCreateInfo.pEnabledFeatures = &physicalDeviceFeatures; // Enabling all available features for now
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &deviceQueueCreateInfo;
    m_device = m_physicalDevice.createDevice(deviceCreateInfo);
  }

  VK_SUCCEED_OR_RETURN_EZ_FAILURE(ezMemoryAllocatorVulkan::Initialize(m_physicalDevice, m_device, m_instance));

  m_queue = m_device.getQueue(m_queueFamilyIndices[0], 0);

  vkCmdDebugMarkerBeginEXTFunc = (PFN_vkCmdDebugMarkerBeginEXT)m_device.getProcAddr("vkCmdDebugMarkerBeginEXT");
  vkCmdDebugMarkerEndEXTFunc = (PFN_vkCmdDebugMarkerEndEXT)m_device.getProcAddr("vkCmdDebugMarkerEndEXT");
  vkCmdDebugMarkerInsertEXTFunc = (PFN_vkCmdDebugMarkerInsertEXT)m_device.getProcAddr("vkCmdDebugMarkerInsertEXT");
  vkDebugMarkerSetObjectNameEXTFunc = (PFN_vkDebugMarkerSetObjectNameEXT)m_device.getProcAddr("vkDebugMarkerSetObjectNameEXT");
  vkSetDebugUtilsObjectNameEXTFunc = (PFN_vkSetDebugUtilsObjectNameEXT)m_device.getProcAddr("vkSetDebugUtilsObjectNameEXT");

  m_memoryProperties = m_physicalDevice.getMemoryProperties();

  // Fill lookup table
  FillFormatLookupTable();

  ezClipSpaceDepthRange::Default = ezClipSpaceDepthRange::ZeroToOne;

  // Per frame data & timer data
  for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(m_PerFrameData); ++i)
  {
    auto& perFrameData = m_PerFrameData[i];

    //if (FAILED(m_pDevice->CreateQuery(&disjointQueryDesc, &perFrameData.m_pDisjointTimerQuery)))
    //{
    //  ezLog::Error("Creation of native DirectX query for disjoint query has failed!");
    //  return EZ_FAILURE;
    //}
  }

  m_Timestamps.SetCountUninitialized(1024);
  for (ezUInt32 i = 0; i < m_Timestamps.GetCount(); ++i)
  {
    //if (FAILED(m_pDevice->CreateQuery(&timerQueryDesc, &m_Timestamps[i])))
    //{
    //  ezLog::Error("Creation of native DirectX query for timestamp has failed!");
    //  return EZ_FAILURE;
    //}
  }

  m_SyncTimeDiff.SetZero();

  m_pDefaultPass = EZ_NEW(&m_Allocator, ezGALPassVulkan, *this);

  ezCommandBufferPoolVulkan::Initialize(m_device, graphicsQueueIndex);
  ezSemaphorePoolVulkan::Initialize(m_device);
  ezFencePoolVulkan::Initialize(m_device);
  ezResourceCacheVulkan::Initialize(this, m_device);

  ezGALWindowSwapChain::SetFactoryMethod([this](const ezGALWindowSwapChainCreationDescription& desc) -> ezGALSwapChainHandle { return CreateSwapChain([this, &desc](ezAllocatorBase* pAllocator) -> ezGALSwapChain* { return EZ_NEW(pAllocator, ezGALSwapChainVulkan, desc); }); });

  return EZ_SUCCESS;
}

void ezGALDeviceVulkan::ReportLiveGpuObjects()
{
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
  // not implemented
  return;

#else

  // TODO how to do this under vulkan?

  OutputDebugStringW(L" +++++ Live Vulkan Objects: +++++\n");



  OutputDebugStringW(L" ----- Live Vulkan Objects: -----\n");

#endif
}

ezResult ezGALDeviceVulkan::ShutdownPlatform()
{
  ezGALWindowSwapChain::SetFactoryMethod({});

  auto& pCommandEncoder = m_pDefaultPass->m_pCommandEncoderImpl;

  for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(m_PerFrameData); ++i)
  {
    // First, we wait for all fences for all submit calls. This is necessary to make sure no resources of the frame are still in use by the GPU.
    auto& perFrameData = m_PerFrameData[i];
    for (vk::Fence fence : perFrameData.m_CommandBufferFences)
    {
      vk::Result fenceStatus = m_device.getFenceStatus(fence);
      if (fenceStatus == vk::Result::eNotReady)
      {
        m_device.waitForFences(1, &fence, true, 1000000000ui64);
      }
    }
    perFrameData.m_CommandBufferFences.Clear();
  }

  for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(m_PerFrameData); ++i)
  {
    {
      EZ_LOCK(m_PerFrameData[i].m_pendingDeletionsMutex);
      DeletePendingResources(m_PerFrameData[i].m_pendingDeletions);
    }
    {
      EZ_LOCK(m_PerFrameData[i].m_reclaimResourcesMutex);
      ReclaimResources(m_PerFrameData[i].m_reclaimResources);
    }
  }

  ezResourceCacheVulkan::DeInitialize();
  ezCommandBufferPoolVulkan::DeInitialize();
  ezSemaphorePoolVulkan::DeInitialize();
  ezFencePoolVulkan::DeInitialize();


  for (ezUInt32 type = 0; type < TempResourceType::ENUM_COUNT; ++type)
  {
    for (auto it = m_FreeTempResources[type].GetIterator(); it.IsValid(); ++it)
    {
      ezDynamicArray<VkResource*>& resources = it.Value();
      for (auto pResource : resources)
      {
        EZ_GAL_VULKAN_RELEASE(pResource);
      }
    }
    m_FreeTempResources[type].Clear();

    for (auto& tempResource : m_UsedTempResources[type])
    {
      EZ_GAL_VULKAN_RELEASE(tempResource.m_pResource);
    }
    m_UsedTempResources[type].Clear();
  }

  for (auto& timestamp : m_Timestamps)
  {
    EZ_GAL_VULKAN_RELEASE(timestamp);
  }
  m_Timestamps.Clear();

  for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(m_PerFrameData); ++i)
  {
    auto& perFrameData = m_PerFrameData[i];

    //EZ_GAL_VULKAN_RELEASE(perFrameData.m_pDisjointTimerQuery);
  }

  m_pDefaultPass = nullptr;

  ezMemoryAllocatorVulkan::DeInitialize();

  m_device.destroy();

  m_extensions.pfn_vkDestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);

  m_instance.destroy();
  ReportLiveGpuObjects();

  return EZ_SUCCESS;
}

// Pipeline & Pass functions

void ezGALDeviceVulkan::BeginPipelinePlatform(const char* szName, ezGALSwapChain* pSwapChain)
{
  EZ_PROFILE_SCOPE("BeginPipelinePlatform");

  if (!m_PerFrameData[m_uiCurrentPerFrameData].m_currentCommandBuffer)
  {
    // Restart new command buffer if none is active already.
    m_PerFrameData[m_uiCurrentPerFrameData].m_currentCommandBuffer = ezCommandBufferPoolVulkan::RequestCommandBuffer();
    vk::CommandBufferBeginInfo beginInfo;
    m_PerFrameData[m_uiCurrentPerFrameData].m_currentCommandBuffer.begin(&beginInfo);
  }

  if (pSwapChain)
  {
    pSwapChain->AcquireNextImage(this);
  }
}

void ezGALDeviceVulkan::EndPipelinePlatform(ezGALSwapChain* pSwapChain)
{
  EZ_PROFILE_SCOPE("EndPipelinePlatform");

  if (pSwapChain)
  {
    pSwapChain->PresentNextImage(this);
  }

  // Restart new command buffer
  m_PerFrameData[m_uiCurrentPerFrameData].m_currentCommandBuffer = ezCommandBufferPoolVulkan::RequestCommandBuffer();
  vk::CommandBufferBeginInfo beginInfo;
  m_PerFrameData[m_uiCurrentPerFrameData].m_currentCommandBuffer.begin(&beginInfo);
}

vk::Fence ezGALDeviceVulkan::Submit(vk::Semaphore waitSemaphore, vk::PipelineStageFlags waitStage, vk::Semaphore signalSemaphore)
{
  m_PerFrameData[m_uiCurrentPerFrameData].m_currentCommandBuffer.end();

  vk::SubmitInfo submitInfo = {};
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &m_PerFrameData[m_uiCurrentPerFrameData].m_currentCommandBuffer;

  vk::Fence renderFence = ezFencePoolVulkan::RequestFence();

  ezHybridArray<vk::Semaphore, 2> waitSemaphores;
  ezHybridArray<vk::PipelineStageFlags, 2> waitStages;
  ezHybridArray<vk::Semaphore, 2> signalSemaphores;

  if (m_lastCommandBufferFinished)
  {
    waitSemaphores.PushBack(m_lastCommandBufferFinished);
    waitStages.PushBack(vk::PipelineStageFlagBits::eAllCommands);
    ReclaimLater(m_lastCommandBufferFinished);
  }

  if (waitSemaphore)
  {
    waitSemaphores.PushBack(waitSemaphore);
    waitStages.PushBack(waitStage);
  }

  if (signalSemaphore)
  {
    signalSemaphores.PushBack(signalSemaphore);
  }
  m_lastCommandBufferFinished = ezSemaphorePoolVulkan::RequestSemaphore();
  signalSemaphores.PushBack(m_lastCommandBufferFinished);

  submitInfo.waitSemaphoreCount = waitSemaphores.GetCount();
  submitInfo.pWaitSemaphores = waitSemaphores.GetData();
  submitInfo.pWaitDstStageMask = waitStages.GetData();
  submitInfo.signalSemaphoreCount = signalSemaphores.GetCount();
  submitInfo.pSignalSemaphores = signalSemaphores.GetData();

  {
    m_PerFrameData[m_uiCurrentPerFrameData].m_CommandBufferFences.PushBack(renderFence);
    m_queue.submit(1, &submitInfo, renderFence);
  }

  ReclaimLater(renderFence);
  ReclaimLater(m_PerFrameData[m_uiCurrentPerFrameData].m_currentCommandBuffer);

  return renderFence;
}

ezGALPass* ezGALDeviceVulkan::BeginPassPlatform(const char* szName)
{
  return m_pDefaultPass.Borrow();
}

void ezGALDeviceVulkan::EndPassPlatform(ezGALPass* pPass)
{
}

// State creation functions

ezGALBlendState* ezGALDeviceVulkan::CreateBlendStatePlatform(const ezGALBlendStateCreationDescription& Description)
{
  ezGALBlendStateVulkan* pState = EZ_NEW(&m_Allocator, ezGALBlendStateVulkan, Description);

  if (pState->InitPlatform(this).Succeeded())
  {
    return pState;
  }
  else
  {
    EZ_DELETE(&m_Allocator, pState);
    return nullptr;
  }
}

void ezGALDeviceVulkan::DestroyBlendStatePlatform(ezGALBlendState* pBlendState)
{
  ezGALBlendStateVulkan* pState = static_cast<ezGALBlendStateVulkan*>(pBlendState);
  pState->DeInitPlatform(this).IgnoreResult();
  EZ_DELETE(&m_Allocator, pState);
}

ezGALDepthStencilState* ezGALDeviceVulkan::CreateDepthStencilStatePlatform(const ezGALDepthStencilStateCreationDescription& Description)
{
  ezGALDepthStencilStateVulkan* pVulkanDepthStencilState = EZ_NEW(&m_Allocator, ezGALDepthStencilStateVulkan, Description);

  if (pVulkanDepthStencilState->InitPlatform(this).Succeeded())
  {
    return pVulkanDepthStencilState;
  }
  else
  {
    EZ_DELETE(&m_Allocator, pVulkanDepthStencilState);
    return nullptr;
  }
}

void ezGALDeviceVulkan::DestroyDepthStencilStatePlatform(ezGALDepthStencilState* pDepthStencilState)
{
  ezGALDepthStencilStateVulkan* pVulkanDepthStencilState = static_cast<ezGALDepthStencilStateVulkan*>(pDepthStencilState);
  pVulkanDepthStencilState->DeInitPlatform(this).IgnoreResult();
  EZ_DELETE(&m_Allocator, pVulkanDepthStencilState);
}

ezGALRasterizerState* ezGALDeviceVulkan::CreateRasterizerStatePlatform(const ezGALRasterizerStateCreationDescription& Description)
{
  ezGALRasterizerStateVulkan* pVulkanRasterizerState = EZ_NEW(&m_Allocator, ezGALRasterizerStateVulkan, Description);

  if (pVulkanRasterizerState->InitPlatform(this).Succeeded())
  {
    return pVulkanRasterizerState;
  }
  else
  {
    EZ_DELETE(&m_Allocator, pVulkanRasterizerState);
    return nullptr;
  }
}

void ezGALDeviceVulkan::DestroyRasterizerStatePlatform(ezGALRasterizerState* pRasterizerState)
{
  ezGALRasterizerStateVulkan* pVulkanRasterizerState = static_cast<ezGALRasterizerStateVulkan*>(pRasterizerState);
  pVulkanRasterizerState->DeInitPlatform(this).IgnoreResult();
  EZ_DELETE(&m_Allocator, pVulkanRasterizerState);
}

ezGALSamplerState* ezGALDeviceVulkan::CreateSamplerStatePlatform(const ezGALSamplerStateCreationDescription& Description)
{
  ezGALSamplerStateVulkan* pVulkanSamplerState = EZ_NEW(&m_Allocator, ezGALSamplerStateVulkan, Description);

  if (pVulkanSamplerState->InitPlatform(this).Succeeded())
  {
    return pVulkanSamplerState;
  }
  else
  {
    EZ_DELETE(&m_Allocator, pVulkanSamplerState);
    return nullptr;
  }
}

void ezGALDeviceVulkan::DestroySamplerStatePlatform(ezGALSamplerState* pSamplerState)
{
  ezGALSamplerStateVulkan* pVulkanSamplerState = static_cast<ezGALSamplerStateVulkan*>(pSamplerState);
  pVulkanSamplerState->DeInitPlatform(this).IgnoreResult();
  EZ_DELETE(&m_Allocator, pVulkanSamplerState);
}


// Resource creation functions

ezGALShader* ezGALDeviceVulkan::CreateShaderPlatform(const ezGALShaderCreationDescription& Description)
{
  ezGALShaderVulkan* pShader = EZ_NEW(&m_Allocator, ezGALShaderVulkan, Description);

  if (!pShader->InitPlatform(this).Succeeded())
  {
    EZ_DELETE(&m_Allocator, pShader);
    return nullptr;
  }

  return pShader;
}

void ezGALDeviceVulkan::DestroyShaderPlatform(ezGALShader* pShader)
{
  ezGALShaderVulkan* pVulkanShader = static_cast<ezGALShaderVulkan*>(pShader);
  pVulkanShader->DeInitPlatform(this).IgnoreResult();
  EZ_DELETE(&m_Allocator, pVulkanShader);
}

ezGALBuffer* ezGALDeviceVulkan::CreateBufferPlatform(
  const ezGALBufferCreationDescription& Description, ezArrayPtr<const ezUInt8> pInitialData)
{
  ezGALBufferVulkan* pBuffer = EZ_NEW(&m_Allocator, ezGALBufferVulkan, Description);

  if (!pBuffer->InitPlatform(this, pInitialData).Succeeded())
  {
    EZ_DELETE(&m_Allocator, pBuffer);
    return nullptr;
  }

  return pBuffer;
}

void ezGALDeviceVulkan::DestroyBufferPlatform(ezGALBuffer* pBuffer)
{
  ezGALBufferVulkan* pVulkanBuffer = static_cast<ezGALBufferVulkan*>(pBuffer);
  pVulkanBuffer->DeInitPlatform(this).IgnoreResult();
  EZ_DELETE(&m_Allocator, pVulkanBuffer);
}

ezGALTexture* ezGALDeviceVulkan::CreateTexturePlatform(
  const ezGALTextureCreationDescription& Description, ezArrayPtr<ezGALSystemMemoryDescription> pInitialData)
{
  ezGALTextureVulkan* pTexture = EZ_NEW(&m_Allocator, ezGALTextureVulkan, Description);

  if (!pTexture->InitPlatform(this, pInitialData).Succeeded())
  {
    EZ_DELETE(&m_Allocator, pTexture);
    return nullptr;
  }

  return pTexture;
}

void ezGALDeviceVulkan::DestroyTexturePlatform(ezGALTexture* pTexture)
{
  ezGALTextureVulkan* pVulkanTexture = static_cast<ezGALTextureVulkan*>(pTexture);
  pVulkanTexture->DeInitPlatform(this).IgnoreResult();
  EZ_DELETE(&m_Allocator, pVulkanTexture);
}

ezGALResourceView* ezGALDeviceVulkan::CreateResourceViewPlatform(
  ezGALResourceBase* pResource, const ezGALResourceViewCreationDescription& Description)
{
  ezGALResourceViewVulkan* pResourceView = EZ_NEW(&m_Allocator, ezGALResourceViewVulkan, pResource, Description);

  if (!pResourceView->InitPlatform(this).Succeeded())
  {
    EZ_DELETE(&m_Allocator, pResourceView);
    return nullptr;
  }

  return pResourceView;
}

void ezGALDeviceVulkan::DestroyResourceViewPlatform(ezGALResourceView* pResourceView)
{
  ezGALResourceViewVulkan* pVulkanResourceView = static_cast<ezGALResourceViewVulkan*>(pResourceView);
  pVulkanResourceView->DeInitPlatform(this).IgnoreResult();
  EZ_DELETE(&m_Allocator, pVulkanResourceView);
}

ezGALRenderTargetView* ezGALDeviceVulkan::CreateRenderTargetViewPlatform(
  ezGALTexture* pTexture, const ezGALRenderTargetViewCreationDescription& Description)
{
  ezGALRenderTargetViewVulkan* pRTView = EZ_NEW(&m_Allocator, ezGALRenderTargetViewVulkan, pTexture, Description);

  if (!pRTView->InitPlatform(this).Succeeded())
  {
    EZ_DELETE(&m_Allocator, pRTView);
    return nullptr;
  }

  return pRTView;
}

void ezGALDeviceVulkan::DestroyRenderTargetViewPlatform(ezGALRenderTargetView* pRenderTargetView)
{
  ezGALRenderTargetViewVulkan* pVulkanRenderTargetView = static_cast<ezGALRenderTargetViewVulkan*>(pRenderTargetView);
  pVulkanRenderTargetView->DeInitPlatform(this).IgnoreResult();
  EZ_DELETE(&m_Allocator, pVulkanRenderTargetView);
}

ezGALUnorderedAccessView* ezGALDeviceVulkan::CreateUnorderedAccessViewPlatform(
  ezGALResourceBase* pTextureOfBuffer, const ezGALUnorderedAccessViewCreationDescription& Description)
{
  ezGALUnorderedAccessViewVulkan* pUnorderedAccessView = EZ_NEW(&m_Allocator, ezGALUnorderedAccessViewVulkan, pTextureOfBuffer, Description);

  if (!pUnorderedAccessView->InitPlatform(this).Succeeded())
  {
    EZ_DELETE(&m_Allocator, pUnorderedAccessView);
    return nullptr;
  }

  return pUnorderedAccessView;
}

void ezGALDeviceVulkan::DestroyUnorderedAccessViewPlatform(ezGALUnorderedAccessView* pUnorderedAccessView)
{
  ezGALUnorderedAccessViewVulkan* pUnorderedAccessViewVulkan = static_cast<ezGALUnorderedAccessViewVulkan*>(pUnorderedAccessView);
  pUnorderedAccessViewVulkan->DeInitPlatform(this).IgnoreResult();
  EZ_DELETE(&m_Allocator, pUnorderedAccessViewVulkan);
}



// Other rendering creation functions
ezGALQuery* ezGALDeviceVulkan::CreateQueryPlatform(const ezGALQueryCreationDescription& Description)
{
  ezGALQueryVulkan* pQuery = EZ_NEW(&m_Allocator, ezGALQueryVulkan, Description);

  if (!pQuery->InitPlatform(this).Succeeded())
  {
    EZ_DELETE(&m_Allocator, pQuery);
    return nullptr;
  }

  return pQuery;
}

void ezGALDeviceVulkan::DestroyQueryPlatform(ezGALQuery* pQuery)
{
  ezGALQueryVulkan* pQueryVulkan = static_cast<ezGALQueryVulkan*>(pQuery);
  pQueryVulkan->DeInitPlatform(this).IgnoreResult();
  EZ_DELETE(&m_Allocator, pQueryVulkan);
}

ezGALVertexDeclaration* ezGALDeviceVulkan::CreateVertexDeclarationPlatform(const ezGALVertexDeclarationCreationDescription& Description)
{
  ezGALVertexDeclarationVulkan* pVertexDeclaration = EZ_NEW(&m_Allocator, ezGALVertexDeclarationVulkan, Description);

  if (pVertexDeclaration->InitPlatform(this).Succeeded())
  {
    return pVertexDeclaration;
  }
  else
  {
    EZ_DELETE(&m_Allocator, pVertexDeclaration);
    return nullptr;
  }
}

void ezGALDeviceVulkan::DestroyVertexDeclarationPlatform(ezGALVertexDeclaration* pVertexDeclaration)
{
  ezGALVertexDeclarationVulkan* pVertexDeclarationVulkan = static_cast<ezGALVertexDeclarationVulkan*>(pVertexDeclaration);
  pVertexDeclarationVulkan->DeInitPlatform(this).IgnoreResult();
  EZ_DELETE(&m_Allocator, pVertexDeclarationVulkan);
}

ezGALTimestampHandle ezGALDeviceVulkan::GetTimestampPlatform()
{
  ezUInt32 uiIndex = m_uiNextTimestamp;
  m_uiNextTimestamp = (m_uiNextTimestamp + 1) % m_Timestamps.GetCount();
  return {uiIndex, m_uiFrameCounter};
}

ezResult ezGALDeviceVulkan::GetTimestampResultPlatform(ezGALTimestampHandle hTimestamp, ezTime& result)
{
  // Check whether frequency and sync timer are already available for the frame of the timestamp
  ezUInt64 uiFrameCounter = hTimestamp.m_uiFrameCounter;

  PerFrameData* pPerFrameData = nullptr;
  for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(m_PerFrameData); ++i)
  {
    if (m_PerFrameData[i].m_uiFrame == uiFrameCounter && m_PerFrameData[i].m_fInvTicksPerSecond >= 0.0)
    {
      pPerFrameData = &m_PerFrameData[i];
      break;
    }
  }

  if (pPerFrameData == nullptr)
  {
    return EZ_FAILURE;
  }

#if 0 // TODO port
  ID3D11Query* pQuery = GetTimestamp(hTimestamp);

  ezUInt64 uiTimestamp;
  if (FAILED(pContext->GetDXContext()->GetData(pQuery, &uiTimestamp, sizeof(uiTimestamp), D3D11_ASYNC_GETDATA_DONOTFLUSH)))
  {
    return EZ_FAILURE;
  }

  if (pPerFrameData->m_fInvTicksPerSecond == 0.0)
  {
    result.SetZero();
  }
  else
  {
    result = ezTime::Seconds(double(uiTimestamp) * pPerFrameData->m_fInvTicksPerSecond) + m_SyncTimeDiff;
  }
#endif
  return EZ_SUCCESS;
}

// Misc functions

void ezGALDeviceVulkan::BeginFramePlatform(const ezUInt64 uiRenderFrame)
{
  auto& pCommandEncoder = m_pDefaultPass->m_pCommandEncoderImpl;

  // check if fence is reached and wait if the disjoint timer is about to be re-used
  {
    auto& perFrameData = m_PerFrameData[m_uiCurrentPerFrameData];
    for (vk::Fence fence : perFrameData.m_CommandBufferFences)
    {
      vk::Result fenceStatus = m_device.getFenceStatus(fence);
      if (fenceStatus == vk::Result::eNotReady)
      {
        m_device.waitForFences(1, &fence, true, 1000000000ui64);
      }
    }
    perFrameData.m_CommandBufferFences.Clear();

    {
      EZ_LOCK(m_PerFrameData[m_uiCurrentPerFrameData].m_pendingDeletionsMutex);
      DeletePendingResources(m_PerFrameData[m_uiCurrentPerFrameData].m_pendingDeletions);
    }
    {
      EZ_LOCK(m_PerFrameData[m_uiCurrentPerFrameData].m_reclaimResourcesMutex);
      ReclaimResources(m_PerFrameData[m_uiCurrentPerFrameData].m_reclaimResources);
    }
  }
  {
    auto& perFrameData = m_PerFrameData[m_uiNextPerFrameData];
    // TODO
    // pContext->GetDXContext()->Begin(perFrameData.m_pDisjointTimerQuery);

    perFrameData.m_fInvTicksPerSecond = -1.0f;
  }

  m_PerFrameData[m_uiCurrentPerFrameData].m_uiFrame = m_uiFrameCounter;
  m_PerFrameData[m_uiCurrentPerFrameData].m_currentCommandBuffer = ezCommandBufferPoolVulkan::RequestCommandBuffer();
  vk::CommandBufferBeginInfo beginInfo;
  m_PerFrameData[m_uiCurrentPerFrameData].m_currentCommandBuffer.begin(&beginInfo);
}

void ezGALDeviceVulkan::EndFramePlatform()
{
  {
    m_PerFrameData[m_uiCurrentPerFrameData].m_currentCommandBuffer.end();

    ezHybridArray<vk::Semaphore, 2> waitSemaphores;
    ezHybridArray<vk::PipelineStageFlags, 2> waitStages;
    if (m_lastCommandBufferFinished)
    {
      waitSemaphores.PushBack(m_lastCommandBufferFinished);
      waitStages.PushBack(vk::PipelineStageFlagBits::eAllCommands);
      ReclaimLater(m_lastCommandBufferFinished);
    }

    vk::SubmitInfo submitInfo;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_PerFrameData[m_uiCurrentPerFrameData].m_currentCommandBuffer;
    submitInfo.waitSemaphoreCount = waitSemaphores.GetCount();
    submitInfo.pWaitSemaphores = waitSemaphores.GetData();
    submitInfo.pWaitDstStageMask = waitStages.GetData();

    vk::Fence renderFence = ezFencePoolVulkan::RequestFence();

    m_PerFrameData[m_uiCurrentPerFrameData].m_CommandBufferFences.PushBack(renderFence);
    m_queue.submit(1, &submitInfo, renderFence);
    ReclaimLater(renderFence);
  }

  // end disjoint query
  {
    auto& perFrameData = m_PerFrameData[m_uiNextPerFrameData];
    //m_pImmediateContext->End(perFrameData.m_pDisjointTimerQuery);
  }

  /*
  // check if fence is reached and update per frame data
  {
    auto& perFrameData = m_PerFrameData[m_uiCurrentPerFrameData];
    if (perFrameData.m_uiFrame != ((ezUInt64)-1))
    {
      if (pCommandEncoder->IsFenceReachedPlatform(perFrameData.m_pFence))
      {
        FreeTempResources(perFrameData.m_uiFrame);
     
        D3D11_QUERY_DATA_TIMESTAMP_DISJOINT data;
        if (FAILED(m_pImmediateContext->GetData(perFrameData.m_pDisjointTimerQuery, &data, sizeof(data), D3D11_ASYNC_GETDATA_DONOTFLUSH)) || data.Disjoint)
        {
          perFrameData.m_fInvTicksPerSecond = 0.0f;
        }
        else
        {
          perFrameData.m_fInvTicksPerSecond = 1.0 / (double)data.Frequency;

          if (m_bSyncTimeNeeded)
          {
            ezGALTimestampHandle hTimestamp = m_pDefaultPass->m_pRenderCommandEncoder->InsertTimestamp();
            ID3D11Query* pQuery = GetTimestamp(hTimestamp);

            ezUInt64 uiTimestamp;
            while (m_pImmediateContext->GetData(pQuery, &uiTimestamp, sizeof(uiTimestamp), 0) != S_OK)
            {
              ezThreadUtils::YieldTimeSlice();
            }

            m_SyncTimeDiff = ezTime::Now() - ezTime::Seconds(double(uiTimestamp) * perFrameData.m_fInvTicksPerSecond);
            m_bSyncTimeNeeded = false;
          }
        }

        m_uiCurrentPerFrameData = (m_uiCurrentPerFrameData + 1) % EZ_ARRAY_SIZE(m_PerFrameData);
      }
    }
  }
*/
  m_uiCurrentPerFrameData = (m_uiCurrentPerFrameData + 1) % EZ_ARRAY_SIZE(m_PerFrameData);
  m_uiNextPerFrameData = (m_uiCurrentPerFrameData + 1) % EZ_ARRAY_SIZE(m_PerFrameData);
  ++m_uiFrameCounter;
}

void ezGALDeviceVulkan::FillCapabilitiesPlatform()
{
  vk::PhysicalDeviceProperties properties = m_physicalDevice.getProperties();
  vk::PhysicalDeviceMemoryProperties memProperties = m_physicalDevice.getMemoryProperties();
  vk::PhysicalDeviceFeatures features = m_physicalDevice.getFeatures();

  ezUInt64 dedicatedMemory = 0;
  ezUInt64 systemMemory = 0;
  for (uint32_t i = 0; i < memProperties.memoryHeapCount; ++i)
  {
    if (memProperties.memoryHeaps[i].flags & vk::MemoryHeapFlagBits::eDeviceLocal)
    {
      dedicatedMemory += memProperties.memoryHeaps[i].size;
    }
    else
    {
      systemMemory += memProperties.memoryHeaps[i].size;
    }
  }

  {
    m_Capabilities.m_sAdapterName = ezStringUtf8(properties.deviceName).GetData();
    m_Capabilities.m_uiDedicatedVRAM = static_cast<ezUInt64>(dedicatedMemory);
    m_Capabilities.m_uiDedicatedSystemRAM = static_cast<ezUInt64>(systemMemory);
    m_Capabilities.m_uiSharedSystemRAM = static_cast<ezUInt64>(0); // TODO
    m_Capabilities.m_bHardwareAccelerated = properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu;
  }

  m_Capabilities.m_bMultithreadedResourceCreation = true;

  m_Capabilities.m_bB5G6R5Textures = true;          // TODO how to check
  m_Capabilities.m_bNoOverwriteBufferUpdate = true; // TODO how to check

  m_Capabilities.m_bShaderStageSupported[ezGALShaderStage::VertexShader] = true;
  m_Capabilities.m_bShaderStageSupported[ezGALShaderStage::HullShader] = features.tessellationShader;
  m_Capabilities.m_bShaderStageSupported[ezGALShaderStage::DomainShader] = features.tessellationShader;
  m_Capabilities.m_bShaderStageSupported[ezGALShaderStage::GeometryShader] = features.geometryShader;
  m_Capabilities.m_bShaderStageSupported[ezGALShaderStage::PixelShader] = true;
  m_Capabilities.m_bShaderStageSupported[ezGALShaderStage::ComputeShader] = true; // we check this when creating the queue, always has to be supported
  m_Capabilities.m_bInstancing = true;
  m_Capabilities.m_b32BitIndices = true;
  m_Capabilities.m_bIndirectDraw = true;
  m_Capabilities.m_bStreamOut = true;
  m_Capabilities.m_uiMaxConstantBuffers = properties.limits.maxDescriptorSetUniformBuffers;
  m_Capabilities.m_bTextureArrays = true;
  m_Capabilities.m_bCubemapArrays = true;
  m_Capabilities.m_uiMaxTextureDimension = properties.limits.maxImageDimension1D;
  m_Capabilities.m_uiMaxCubemapDimension = properties.limits.maxImageDimensionCube;
  m_Capabilities.m_uiMax3DTextureDimension = properties.limits.maxImageDimension3D;
  m_Capabilities.m_uiMaxAnisotropy = static_cast<ezUInt16>(properties.limits.maxSamplerAnisotropy);
  m_Capabilities.m_uiMaxRendertargets = properties.limits.maxColorAttachments;
  m_Capabilities.m_uiUAVCount = ezMath::Min(properties.limits.maxDescriptorSetStorageBuffers, properties.limits.maxDescriptorSetStorageImages);
  m_Capabilities.m_bAlphaToCoverage = true;

  m_Capabilities.m_bConservativeRasterization = false; // need to query for VK_EXT_CONSERVATIVE_RASTERIZATION
}

#if 0

vk::Buffer ezGALDeviceVulkan::FindTempBuffer(ezUInt32 uiSize)
{
  const ezUInt32 uiExpGrowthLimit = 16 * 1024 * 1024;

  uiSize = ezMath::Max(uiSize, 256U);
  if (uiSize < uiExpGrowthLimit)
  {
    uiSize = ezMath::PowerOfTwo_Ceil(uiSize);
  }
  else
  {
    uiSize = ezMemoryUtils::AlignSize(uiSize, uiExpGrowthLimit);
  }

  ID3D11Resource* pResource = nullptr;
  auto it = m_FreeTempResources[TempResourceType::Buffer].Find(uiSize);
  if (it.IsValid())
  {
    ezDynamicArray<ID3D11Resource*>& resources = it.Value();
    if (!resources.IsEmpty())
    {
      pResource = resources[0];
      resources.RemoveAtAndSwap(0);
    }
  }

  if (pResource == nullptr)
  {
    D3D11_BUFFER_DESC desc;
    desc.ByteWidth = uiSize;
    desc.Usage = D3D11_USAGE_STAGING;
    desc.BindFlags = 0;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    desc.MiscFlags = 0;
    desc.StructureByteStride = 0;

    ID3D11Buffer* pBuffer = nullptr;
    if (!SUCCEEDED(m_pDevice->CreateBuffer(&desc, nullptr, &pBuffer)))
    {
      return nullptr;
    }

    pResource = pBuffer;
  }

  auto& tempResource = m_UsedTempResources[TempResourceType::Buffer].ExpandAndGetRef();
  tempResource.m_pResource = pResource;
  tempResource.m_uiFrame = m_uiFrameCounter;
  tempResource.m_uiHash = uiSize;

  return pResource;
}

ID3D11Resource* ezGALDeviceVulkan::FindTempTexture(ezUInt32 uiWidth, ezUInt32 uiHeight, ezUInt32 uiDepth, ezGALResourceFormat::Enum format)
{
  ezUInt32 data[] = {uiWidth, uiHeight, uiDepth, (ezUInt32)format};
  ezUInt32 uiHash = ezHashingUtils::xxHash32(data, sizeof(data));

  ID3D11Resource* pResource = nullptr;
  auto it = m_FreeTempResources[TempResourceType::Texture].Find(uiHash);
  if (it.IsValid())
  {
    ezDynamicArray<ID3D11Resource*>& resources = it.Value();
    if (!resources.IsEmpty())
    {
      pResource = resources[0];
      resources.RemoveAtAndSwap(0);
    }
  }

  if (pResource == nullptr)
  {
    if (uiDepth == 1)
    {
      D3D11_TEXTURE2D_DESC desc;
      desc.Width = uiWidth;
      desc.Height = uiHeight;
      desc.MipLevels = 1;
      desc.ArraySize = 1;
      desc.Format = GetFormatLookupTable().GetFormatInfo(format).m_eStorage;
      desc.SampleDesc.Count = 1;
      desc.SampleDesc.Quality = 0;
      desc.Usage = D3D11_USAGE_STAGING;
      desc.BindFlags = 0;
      desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
      desc.MiscFlags = 0;

      ID3D11Texture2D* pTexture = nullptr;
      if (!SUCCEEDED(m_pDevice->CreateTexture2D(&desc, nullptr, &pTexture)))
      {
        return nullptr;
      }

      pResource = pTexture;
    }
    else
    {
      EZ_ASSERT_NOT_IMPLEMENTED;
      return nullptr;
    }
  }

  auto& tempResource = m_UsedTempResources[TempResourceType::Texture].ExpandAndGetRef();
  tempResource.m_pResource = pResource;
  tempResource.m_uiFrame = m_uiFrameCounter;
  tempResource.m_uiHash = uiHash;

  return pResource;
}

#endif

ezInt32 ezGALDeviceVulkan::GetMemoryIndex(vk::MemoryPropertyFlags properties, const vk::MemoryRequirements& requirements) const
{

  for (ezUInt32 i = 0; i < m_memoryProperties.memoryTypeCount; ++i)
  {
    const vk::MemoryType& type = m_memoryProperties.memoryTypes[i];
    if (requirements.memoryTypeBits & (1 << i) && (type.propertyFlags & properties))
    {
      return i;
    }
  }

  return -1;
}

void ezGALDeviceVulkan::DeleteLater(const PendingDeletion& deletion)
{
  EZ_LOCK(m_PerFrameData[m_uiCurrentPerFrameData].m_pendingDeletionsMutex);
  m_PerFrameData[m_uiCurrentPerFrameData].m_pendingDeletions.PushBack(deletion);
}

void ezGALDeviceVulkan::ReclaimLater(const ReclaimResource& reclaim)
{
  EZ_LOCK(m_PerFrameData[m_uiCurrentPerFrameData].m_reclaimResourcesMutex);
  m_PerFrameData[m_uiCurrentPerFrameData].m_reclaimResources.PushBack(reclaim);
}

void ezGALDeviceVulkan::DeletePendingResources(ezDeque<PendingDeletion>& pendingDeletions)
{
  for (PendingDeletion& deletion : pendingDeletions)
  {
    switch (deletion.m_type)
    {
      case vk::ObjectType::eImageView:
        m_device.destroyImageView(reinterpret_cast<vk::ImageView&>(deletion.m_pObject));
        break;
      case vk::ObjectType::eImage:
        ezMemoryAllocatorVulkan::DestroyImage(reinterpret_cast<vk::Image&>(deletion.m_pObject), deletion.m_allocation);
        break;
      case vk::ObjectType::eBuffer:
        ezMemoryAllocatorVulkan::DestroyBuffer(reinterpret_cast<vk::Buffer&>(deletion.m_pObject), deletion.m_allocation);
        break;
      case vk::ObjectType::eFramebuffer:
        m_device.destroyFramebuffer(reinterpret_cast<vk::Framebuffer&>(deletion.m_pObject));
        break;
      case vk::ObjectType::eRenderPass:
        m_device.destroyRenderPass(reinterpret_cast<vk::RenderPass&>(deletion.m_pObject));
        break;
      case vk::ObjectType::eSampler:
        m_device.destroySampler(reinterpret_cast<vk::Sampler&>(deletion.m_pObject));
        break;
      case vk::ObjectType::eSwapchainKHR:
        m_device.destroySwapchainKHR(reinterpret_cast<vk::SwapchainKHR&>(deletion.m_pObject));
        break;
      case vk::ObjectType::eSurfaceKHR:
        m_instance.destroySurfaceKHR(reinterpret_cast<vk::SurfaceKHR&>(deletion.m_pObject));
        break;
      default:
        EZ_REPORT_FAILURE("This object type is not implemented");
        break;
    }
  }
  pendingDeletions.Clear();
}

void ezGALDeviceVulkan::ReclaimResources(ezDeque<ReclaimResource>& resources)
{
  for (ReclaimResource& resource : resources)
  {
    switch (resource.m_type)
    {
      case vk::ObjectType::eSemaphore:
        ezSemaphorePoolVulkan::ReclaimSemaphore(reinterpret_cast<vk::Semaphore&>(resource.m_pObject));
        break;
      case vk::ObjectType::eFence:
        ezFencePoolVulkan::ReclaimFence(reinterpret_cast<vk::Fence&>(resource.m_pObject));
        break;
      case vk::ObjectType::eCommandBuffer:
        ezCommandBufferPoolVulkan::ReclaimCommandBuffer(reinterpret_cast<vk::CommandBuffer&>(resource.m_pObject));
        break;
      default:
        EZ_REPORT_FAILURE("This object type is not implemented");
        break;
    }
  }
  resources.Clear();
}

void ezGALDeviceVulkan::FreeTempResources(ezUInt64 uiFrame)
{
  for (ezUInt32 type = 0; type < TempResourceType::ENUM_COUNT; ++type)
  {
    while (!m_UsedTempResources[type].IsEmpty())
    {
      auto& usedTempResource = m_UsedTempResources[type].PeekFront();
      if (usedTempResource.m_uiFrame == uiFrame)
      {
        auto it = m_FreeTempResources[type].Find(usedTempResource.m_uiHash);
        if (!it.IsValid())
        {
          it = m_FreeTempResources[type].Insert(usedTempResource.m_uiHash, ezDynamicArray<VkResource*>(&m_Allocator));
        }

        it.Value().PushBack(usedTempResource.m_pResource);
        m_UsedTempResources[type].PopFront();
      }
      else
      {
        break;
      }
    }
  }
}

void ezGALDeviceVulkan::FillFormatLookupTable()
{
  ///       The list below is in the same order as the ezGALResourceFormat enum. No format should be missing except the ones that are just
  ///       different names for the same enum value.

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RGBAFloat, ezGALFormatLookupEntryVulkan(vk::Format::eR32G32B32A32Sfloat)
                                                                      .RT(vk::Format::eR32G32B32A32Sfloat)
                                                                      .VA(vk::Format::eR32G32B32A32Sfloat)
                                                                      .RV(vk::Format::eR32G32B32A32Sfloat));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RGBAUInt, ezGALFormatLookupEntryVulkan(vk::Format::eR32G32B32A32Uint)
                                                                     .RT(vk::Format::eR32G32B32A32Uint)
                                                                     .VA(vk::Format::eR32G32B32A32Uint)
                                                                     .RV(vk::Format::eR32G32B32A32Uint));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RGBAInt, ezGALFormatLookupEntryVulkan(vk::Format::eR32G32B32A32Sint)
                                                                    .RT(vk::Format::eR32G32B32A32Sint)
                                                                    .VA(vk::Format::eR32G32B32A32Sint)
                                                                    .RV(vk::Format::eR32G32B32A32Sint));

  // TODO 3-channel formats are not really supported under vulkan judging by experience
  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RGBFloat, ezGALFormatLookupEntryVulkan(vk::Format::eR32G32B32Sfloat)
                                                                     .RT(vk::Format::eR32G32B32Sfloat)
                                                                     .VA(vk::Format::eR32G32B32Sfloat)
                                                                     .RV(vk::Format::eR32G32B32Sfloat));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RGBUInt, ezGALFormatLookupEntryVulkan(vk::Format::eR32G32B32Uint)
                                                                    .RT(vk::Format::eR32G32B32Uint)
                                                                    .VA(vk::Format::eR32G32B32Uint)
                                                                    .RV(vk::Format::eR32G32B32Uint));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RGBInt, ezGALFormatLookupEntryVulkan(vk::Format::eR32G32B32Sint)
                                                                   .RT(vk::Format::eR32G32B32Sint)
                                                                   .VA(vk::Format::eR32G32B32Sint)
                                                                   .RV(vk::Format::eR32G32B32Sint));

  // TODO dunno if these are actually supported for the respective Vulkan device
  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::B5G6R5UNormalized, ezGALFormatLookupEntryVulkan(vk::Format::eB5G6R5UnormPack16)
                                                                              .RT(vk::Format::eB5G6R5UnormPack16)
                                                                              .VA(vk::Format::eB5G6R5UnormPack16)
                                                                              .RV(vk::Format::eB5G6R5UnormPack16));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::BGRAUByteNormalized, ezGALFormatLookupEntryVulkan(vk::Format::eB8G8R8A8Unorm)
                                                                                .RT(vk::Format::eB8G8R8A8Unorm)
                                                                                .VA(vk::Format::eB8G8R8A8Unorm)
                                                                                .RV(vk::Format::eB8G8R8A8Unorm));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::BGRAUByteNormalizedsRGB,
    ezGALFormatLookupEntryVulkan(vk::Format::eB8G8R8A8Srgb).RT(vk::Format::eB8G8R8A8Srgb).RV(vk::Format::eB8G8R8A8Srgb));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RGBAHalf, ezGALFormatLookupEntryVulkan(vk::Format::eR16G16B16A16Sfloat)
                                                                     .RT(vk::Format::eR16G16B16A16Sfloat)
                                                                     .VA(vk::Format::eR16G16B16A16Sfloat)
                                                                     .RV(vk::Format::eR16G16B16A16Sfloat));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RGBAUShort, ezGALFormatLookupEntryVulkan(vk::Format::eR16G16B16A16Uint)
                                                                       .RT(vk::Format::eR16G16B16A16Uint)
                                                                       .VA(vk::Format::eR16G16B16A16Uint)
                                                                       .RV(vk::Format::eR16G16B16A16Uint));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RGBAUShortNormalized, ezGALFormatLookupEntryVulkan(vk::Format::eR16G16B16A16Unorm)
                                                                                 .RT(vk::Format::eR16G16B16A16Unorm)
                                                                                 .VA(vk::Format::eR16G16B16A16Unorm)
                                                                                 .RV(vk::Format::eR16G16B16A16Unorm));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RGBAShort, ezGALFormatLookupEntryVulkan(vk::Format::eR16G16B16A16Sint)
                                                                      .RT(vk::Format::eR16G16B16A16Sint)
                                                                      .VA(vk::Format::eR16G16B16A16Sint)
                                                                      .RV(vk::Format::eR16G16B16A16Sint));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RGBAShortNormalized, ezGALFormatLookupEntryVulkan(vk::Format::eR16G16B16A16Snorm)
                                                                                .RT(vk::Format::eR16G16B16A16Snorm)
                                                                                .VA(vk::Format::eR16G16B16A16Snorm)
                                                                                .RV(vk::Format::eR16G16B16A16Snorm));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RGFloat, ezGALFormatLookupEntryVulkan(vk::Format::eR32G32Sfloat)
                                                                    .RT(vk::Format::eR32G32Sfloat)
                                                                    .VA(vk::Format::eR32G32Sfloat)
                                                                    .RV(vk::Format::eR32G32Sfloat));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RGUInt, ezGALFormatLookupEntryVulkan(vk::Format::eR32G32Uint)
                                                                   .RT(vk::Format::eR32G32Uint)
                                                                   .VA(vk::Format::eR32G32Uint)
                                                                   .RV(vk::Format::eR32G32Uint));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RGInt, ezGALFormatLookupEntryVulkan(vk::Format::eR32G32Sint)
                                                                  .RT(vk::Format::eR32G32Sint)
                                                                  .VA(vk::Format::eR32G32Sint)
                                                                  .RV(vk::Format::eR32G32Sint));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RGB10A2UInt, ezGALFormatLookupEntryVulkan(vk::Format::eA2B10G10R10UintPack32)
                                                                        .RT(vk::Format::eA2B10G10R10UintPack32)
                                                                        .VA(vk::Format::eA2B10G10R10UintPack32)
                                                                        .RV(vk::Format::eA2B10G10R10UintPack32));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RGB10A2UIntNormalized, ezGALFormatLookupEntryVulkan(vk::Format::eA2B10G10R10UnormPack32)
                                                                                  .RT(vk::Format::eA2B10G10R10UnormPack32)
                                                                                  .VA(vk::Format::eA2B10G10R10UnormPack32)
                                                                                  .RV(vk::Format::eA2B10G10R10UnormPack32));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RG11B10Float, ezGALFormatLookupEntryVulkan(vk::Format::eB10G11R11UfloatPack32)
                                                                         .RT(vk::Format::eB10G11R11UfloatPack32)
                                                                         .VA(vk::Format::eB10G11R11UfloatPack32)
                                                                         .RV(vk::Format::eB10G11R11UfloatPack32));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RGBAUByteNormalized, ezGALFormatLookupEntryVulkan(vk::Format::eR8G8B8A8Unorm)
                                                                                .RT(vk::Format::eR8G8B8A8Unorm)
                                                                                .VA(vk::Format::eR8G8B8A8Unorm)
                                                                                .RV(vk::Format::eR8G8B8A8Unorm));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RGBAUByteNormalizedsRGB,
    ezGALFormatLookupEntryVulkan(vk::Format::eR8G8B8A8Srgb).RT(vk::Format::eR8G8B8A8Srgb).RV(vk::Format::eR8G8B8A8Srgb));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RGBAUByte, ezGALFormatLookupEntryVulkan(vk::Format::eR8G8B8A8Uint)
                                                                      .RT(vk::Format::eR8G8B8A8Uint)
                                                                      .VA(vk::Format::eR8G8B8A8Uint)
                                                                      .RV(vk::Format::eR8G8B8A8Uint));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RGBAByteNormalized, ezGALFormatLookupEntryVulkan(vk::Format::eR8G8B8A8Snorm)
                                                                               .RT(vk::Format::eR8G8B8A8Snorm)
                                                                               .VA(vk::Format::eR8G8B8A8Snorm)
                                                                               .RV(vk::Format::eR8G8B8A8Snorm));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RGBAByte, ezGALFormatLookupEntryVulkan(vk::Format::eR8G8B8A8Sint)
                                                                     .RT(vk::Format::eR8G8B8A8Sint)
                                                                     .VA(vk::Format::eR8G8B8A8Sint)
                                                                     .RV(vk::Format::eR8G8B8A8Sint));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RGHalf, ezGALFormatLookupEntryVulkan(vk::Format::eR16G16Sfloat)
                                                                   .RT(vk::Format::eR16G16Sfloat)
                                                                   .VA(vk::Format::eR16G16Sfloat)
                                                                   .RV(vk::Format::eR16G16Sfloat));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RGUShort, ezGALFormatLookupEntryVulkan(vk::Format::eR16G16Uint)
                                                                     .RT(vk::Format::eR16G16Uint)
                                                                     .VA(vk::Format::eR16G16Uint)
                                                                     .RV(vk::Format::eR16G16Uint));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RGUShortNormalized, ezGALFormatLookupEntryVulkan(vk::Format::eR16G16Unorm)
                                                                               .RT(vk::Format::eR16G16Unorm)
                                                                               .VA(vk::Format::eR16G16Unorm)
                                                                               .RV(vk::Format::eR16G16Unorm));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RGShort, ezGALFormatLookupEntryVulkan(vk::Format::eR16G16Sint)
                                                                    .RT(vk::Format::eR16G16Sint)
                                                                    .VA(vk::Format::eR16G16Sint)
                                                                    .RV(vk::Format::eR16G16Sint));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RGShortNormalized, ezGALFormatLookupEntryVulkan(vk::Format::eR16G16Snorm)
                                                                              .RT(vk::Format::eR16G16Snorm)
                                                                              .VA(vk::Format::eR16G16Snorm)
                                                                              .RV(vk::Format::eR16G16Snorm));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RGUByte,
    ezGALFormatLookupEntryVulkan(vk::Format::eR8G8Uint).RT(vk::Format::eR8G8Uint).VA(vk::Format::eR8G8Uint).RV(vk::Format::eR8G8Uint));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RGUByteNormalized,
    ezGALFormatLookupEntryVulkan(vk::Format::eR8G8Unorm).RT(vk::Format::eR8G8Unorm).VA(vk::Format::eR8G8Unorm).RV(vk::Format::eR8G8Unorm));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RGByte,
    ezGALFormatLookupEntryVulkan(vk::Format::eR8G8Sint).RT(vk::Format::eR8G8Sint).VA(vk::Format::eR8G8Sint).RV(vk::Format::eR8G8Sint));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RGByteNormalized,
    ezGALFormatLookupEntryVulkan(vk::Format::eR8G8Snorm).RT(vk::Format::eR8G8Snorm).VA(vk::Format::eR8G8Snorm).RV(vk::Format::eR8G8Snorm));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RFloat,
    ezGALFormatLookupEntryVulkan(vk::Format::eR32Sfloat).RT(vk::Format::eR32Sfloat).VA(vk::Format::eR32Sfloat).RV(vk::Format::eR32Sfloat));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RUInt,
    ezGALFormatLookupEntryVulkan(vk::Format::eR32Uint).RT(vk::Format::eR32Uint).VA(vk::Format::eR32Uint).RV(vk::Format::eR32Uint));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RInt,
    ezGALFormatLookupEntryVulkan(vk::Format::eR32Sint).RT(vk::Format::eR32Sint).VA(vk::Format::eR32Sint).RV(vk::Format::eR32Sint));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RHalf,
    ezGALFormatLookupEntryVulkan(vk::Format::eR16Sfloat).RT(vk::Format::eR16Sfloat).VA(vk::Format::eR16Sfloat).RV(vk::Format::eR16Sfloat));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RUShort,
    ezGALFormatLookupEntryVulkan(vk::Format::eR16Uint).RT(vk::Format::eR16Uint).VA(vk::Format::eR16Uint).RV(vk::Format::eR16Uint));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RUShortNormalized,
    ezGALFormatLookupEntryVulkan(vk::Format::eR16Unorm).RT(vk::Format::eR16Unorm).VA(vk::Format::eR16Unorm).RV(vk::Format::eR16Unorm));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RShort,
    ezGALFormatLookupEntryVulkan(vk::Format::eR16Sint).RT(vk::Format::eR16Sint).VA(vk::Format::eR16Sint).RV(vk::Format::eR16Sint));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RShortNormalized,
    ezGALFormatLookupEntryVulkan(vk::Format::eR16Snorm).RT(vk::Format::eR16Snorm).VA(vk::Format::eR16Snorm).RV(vk::Format::eR16Snorm));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RUByte,
    ezGALFormatLookupEntryVulkan(vk::Format::eR8Uint).RT(vk::Format::eR8Uint).VA(vk::Format::eR8Uint).RV(vk::Format::eR8Uint));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RUByteNormalized,
    ezGALFormatLookupEntryVulkan(vk::Format::eR8Unorm).RT(vk::Format::eR8Unorm).VA(vk::Format::eR8Unorm).RV(vk::Format::eR8Unorm));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RByte,
    ezGALFormatLookupEntryVulkan(vk::Format::eR8Sint).RT(vk::Format::eR8Sint).VA(vk::Format::eR8Sint).RV(vk::Format::eR8Sint));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RByteNormalized,
    ezGALFormatLookupEntryVulkan(vk::Format::eR8Snorm).RT(vk::Format::eR8Snorm).VA(vk::Format::eR8Snorm).RV(vk::Format::eR8Snorm));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::AUByteNormalized,
    ezGALFormatLookupEntryVulkan(vk::Format::eR8Unorm).RT(vk::Format::eR8Unorm).VA(vk::Format::eR8Unorm).RV(vk::Format::eR8Unorm));

  auto SelectDepthFormat = [&](const std::vector<vk::Format>& list) -> vk::Format {
    for (auto& format : list)
    {
      vk::FormatProperties formatProperties;
      m_physicalDevice.getFormatProperties(format, &formatProperties);
      if (formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment)
        return format;
    }
    return vk::Format::eUndefined;
  };

  // Select smallest available depth format.
  vk::Format depthFormat = SelectDepthFormat({vk::Format::eD16Unorm, vk::Format::eD24UnormS8Uint, vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint});
  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::D16,
    ezGALFormatLookupEntryVulkan(depthFormat).RT(depthFormat).RV(depthFormat).DS(depthFormat).D(depthFormat));

  // Select closest depth stencil format.
  depthFormat = SelectDepthFormat({vk::Format::eD24UnormS8Uint, vk::Format::eD32SfloatS8Uint, vk::Format::eD16UnormS8Uint});
  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::D24S8, ezGALFormatLookupEntryVulkan(depthFormat)
                                                                  .RT(depthFormat)
                                                                  .RV(depthFormat)
                                                                  .DS(depthFormat)
                                                                  .D(depthFormat)
                                                                  .S(depthFormat));

  // Select biggest depth format.
  depthFormat = SelectDepthFormat({vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint, vk::Format::eD16Unorm});
  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::DFloat,
    ezGALFormatLookupEntryVulkan(depthFormat).RT(depthFormat).RV(depthFormat).D(depthFormat).DS(depthFormat));

  // TODO is BC1 the rgba or the rgb format?
  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::BC1, ezGALFormatLookupEntryVulkan(vk::Format::eBc1RgbaUnormBlock).RV(vk::Format::eBc1RgbaUnormBlock));

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::BC1sRGB, ezGALFormatLookupEntryVulkan(vk::Format::eBc1RgbaSrgbBlock).RV(vk::Format::eBc1RgbaSrgbBlock));

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::BC2, ezGALFormatLookupEntryVulkan(vk::Format::eBc2UnormBlock).RV(vk::Format::eBc2UnormBlock));

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::BC2sRGB, ezGALFormatLookupEntryVulkan(vk::Format::eBc2SrgbBlock).RV(vk::Format::eBc2SrgbBlock));

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::BC3, ezGALFormatLookupEntryVulkan(vk::Format::eBc3UnormBlock).RV(vk::Format::eBc3UnormBlock));

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::BC3sRGB, ezGALFormatLookupEntryVulkan(vk::Format::eBc3SrgbBlock).RV(vk::Format::eBc3SrgbBlock));

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::BC4UNormalized, ezGALFormatLookupEntryVulkan(vk::Format::eBc4UnormBlock).RV(vk::Format::eBc4UnormBlock));

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::BC4Normalized, ezGALFormatLookupEntryVulkan(vk::Format::eBc4SnormBlock).RV(vk::Format::eBc4SnormBlock));

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::BC5UNormalized, ezGALFormatLookupEntryVulkan(vk::Format::eBc5UnormBlock).RV(vk::Format::eBc5UnormBlock));

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::BC5Normalized, ezGALFormatLookupEntryVulkan(vk::Format::eBc5SnormBlock).RV(vk::Format::eBc5SnormBlock));

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::BC6UFloat, ezGALFormatLookupEntryVulkan(vk::Format::eBc6HUfloatBlock).RV(vk::Format::eBc6HUfloatBlock));

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::BC6Float, ezGALFormatLookupEntryVulkan(vk::Format::eBc6HSfloatBlock).RV(vk::Format::eBc6HSfloatBlock));

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::BC7UNormalized, ezGALFormatLookupEntryVulkan(vk::Format::eBc7UnormBlock).RV(vk::Format::eBc7UnormBlock));

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::BC7UNormalizedsRGB, ezGALFormatLookupEntryVulkan(vk::Format::eBc7SrgbBlock).RV(vk::Format::eBc7SrgbBlock));
}



EZ_STATICLINK_FILE(RendererVulkan, RendererVulkan_Device_Implementation_DeviceVulkan);
