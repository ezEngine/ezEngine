
#pragma once

#include <Foundation/Types/Bitflags.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererVulkan/Device/DispatchContext.h>
#include <RendererVulkan/MemoryAllocator/MemoryAllocatorVulkan.h>
#include <RendererVulkan/RendererVulkanDLL.h>

#include <vulkan/vulkan.hpp>

EZ_DEFINE_AS_POD_TYPE(vk::Format);

class ezGALCommandEncoderImplVulkan;
class ezFenceQueueVulkan;

struct ezGALFormatLookupEntryVulkan
{
  ezGALFormatLookupEntryVulkan() = default;
  ezGALFormatLookupEntryVulkan(vk::Format format)
  {
    m_format = format;
    m_readback = format;
  }

  ezGALFormatLookupEntryVulkan(vk::Format format, ezArrayPtr<vk::Format> mutableFormats)
  {
    m_format = format;
    m_readback = format;
    m_mutableFormats = mutableFormats;
  }

  inline ezGALFormatLookupEntryVulkan& R(vk::Format readbackType)
  {
    m_readback = readbackType;
    return *this;
  }

  vk::Format m_format = vk::Format::eUndefined;
  vk::Format m_readback = vk::Format::eUndefined;
  ezHybridArray<vk::Format, 6> m_mutableFormats;
};

using ezGALFormatLookupTableVulkan = ezGALFormatLookupTable<ezGALFormatLookupEntryVulkan>;

class ezGALBufferVulkan;
class ezGALTextureVulkan;
class ezGALPassVulkan;
class ezPipelineBarrierVulkan;
class ezCommandBufferPoolVulkan;
class ezStagingBufferPoolVulkan;
class ezQueryPoolVulkan;
class ezInitContextVulkan;

/// \brief The Vulkan device implementation of the graphics abstraction layer.
class EZ_RENDERERVULKAN_DLL ezGALDeviceVulkan : public ezGALDevice
{
private:
  friend ezInternal::NewInstance<ezGALDevice> CreateVulkanDevice(ezAllocator* pAllocator, const ezGALDeviceCreationDescription& Description);
  ezGALDeviceVulkan(const ezGALDeviceCreationDescription& Description);

public:
  virtual ~ezGALDeviceVulkan();

public:
  struct PendingDeletionFlags
  {
    using StorageType = ezUInt32;

    enum Enum
    {
      UsesExternalMemory = EZ_BIT(0),
      IsFileDescriptor = EZ_BIT(1),
      Default = 0
    };

    struct Bits
    {
      StorageType UsesExternalMemory : 1;
      StorageType IsFileDescriptor : 1;
    };
  };

  struct PendingDeletion
  {
    EZ_DECLARE_POD_TYPE();
    vk::ObjectType m_type;
    ezBitflags<PendingDeletionFlags> m_flags;
    void* m_pObject;
    union
    {
      ezVulkanAllocation m_allocation;
      void* m_pContext;
    };
  };

  struct ReclaimResource
  {
    EZ_DECLARE_POD_TYPE();
    vk::ObjectType m_type;
    void* m_pObject;
    void* m_pContext = nullptr;
  };

  struct Extensions
  {
    bool m_bSurface = false;
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    bool m_bWin32Surface = false;
#elif EZ_ENABLED(EZ_SUPPORTS_GLFW)
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
    bool m_bAndroidSurface = false;
#else
#  error "Vulkan Platform not supported"
#endif

#if EZ_ENABLED(EZ_PLATFORM_LINUX)
    bool m_bSurfaceXcb = false;
#endif

    bool m_bDebugUtils = false;
    bool m_bDebugUtilsMarkers = false;
    PFN_vkCreateDebugUtilsMessengerEXT pfn_vkCreateDebugUtilsMessengerEXT = nullptr;
    PFN_vkDestroyDebugUtilsMessengerEXT pfn_vkDestroyDebugUtilsMessengerEXT = nullptr;
    PFN_vkSetDebugUtilsObjectNameEXT pfn_vkSetDebugUtilsObjectNameEXT = nullptr;

    bool m_bDeviceSwapChain = false;
    bool m_bShaderViewportIndexLayer = false;

    vk::PhysicalDeviceCustomBorderColorFeaturesEXT m_borderColorEXT;
    bool m_bBorderColorFloat = false;

    bool m_bImageFormatList = false;
    vk::PhysicalDeviceTimelineSemaphoreFeatures m_timelineSemaphoresEXT;
    bool m_bTimelineSemaphore = false;

    bool m_bExternalMemoryCapabilities = false;
    bool m_bExternalSemaphoreCapabilities = false;

    bool m_bExternalMemory = false;
    bool m_bExternalSemaphore = false;

    bool m_bExternalMemoryFd = false;
    bool m_bExternalSemaphoreFd = false;

    bool m_bExternalMemoryWin32 = false;
    bool m_bExternalSemaphoreWin32 = false;
  };

  struct Queue
  {
    vk::Queue m_queue;
    ezUInt32 m_uiQueueFamily = -1;
    ezUInt32 m_uiQueueIndex = 0;
  };

  vk::Instance GetVulkanInstance() const;
  vk::Device GetVulkanDevice() const;
  const Queue& GetGraphicsQueue() const;
  const Queue& GetTransferQueue() const;

  vk::PhysicalDevice GetVulkanPhysicalDevice() const;
  const vk::PhysicalDeviceProperties& GetPhysicalDeviceProperties() const { return m_properties; }
  const Extensions& GetExtensions() const { return m_extensions; }
  const ezVulkanDispatchContext& GetDispatchContext() const { return m_dispatchContext; }
  vk::PipelineStageFlags GetSupportedStages() const;

  vk::CommandBuffer& GetCurrentCommandBuffer();
  ezPipelineBarrierVulkan& GetCurrentPipelineBarrier();
  ezQueryPoolVulkan& GetQueryPool() const;
  ezFenceQueueVulkan& GetFenceQueue() const;
  ezStagingBufferPoolVulkan& GetStagingBufferPool() const;
  ezInitContextVulkan& GetInitContext() const;

  ezGALTextureHandle CreateTextureInternal(const ezGALTextureCreationDescription& Description, ezArrayPtr<ezGALSystemMemoryDescription> pInitialData);
  ezGALBufferHandle CreateBufferInternal(const ezGALBufferCreationDescription& Description, ezArrayPtr<const ezUInt8> pInitialData);

  const ezGALFormatLookupTableVulkan& GetFormatLookupTable() const;

  ezInt32 GetMemoryIndex(vk::MemoryPropertyFlags properties, const vk::MemoryRequirements& requirements) const;

  vk::Fence Submit(bool bAddSignalSemaphore = true);

  void DeleteLaterImpl(const PendingDeletion& deletion);

  void DeleteLater(vk::Image& image, vk::DeviceMemory& externalMemory)
  {
    if (image)
    {
      PendingDeletion del = {vk::ObjectType::eImage, {PendingDeletionFlags::UsesExternalMemory}, (void*)image, nullptr};
      del.m_pContext = (void*)externalMemory;
      DeleteLaterImpl(del);
    }
    image = nullptr;
    externalMemory = nullptr;
  }

  template <typename T>
  void DeleteLater(T& object, ezVulkanAllocation& allocation)
  {
    if (object)
    {
      DeleteLaterImpl({object.objectType, {}, (void*)object, allocation});
    }
    object = nullptr;
    allocation = nullptr;
  }

  template <typename T>
  void DeleteLater(T& object, void* pContext)
  {
    if (object)
    {
      PendingDeletion del = {object.objectType, {}, (void*)object, nullptr};
      del.m_pContext = pContext;
      DeleteLaterImpl(static_cast<const PendingDeletion&>(del));
    }
    object = nullptr;
  }

  template <typename T>
  void DeleteLater(T& object)
  {
    if (object)
    {
      DeleteLaterImpl({object.objectType, {}, (void*)object, nullptr});
    }
    object = nullptr;
  }

  void ReclaimLater(const ReclaimResource& reclaim);

  template <typename T>
  void ReclaimLater(T& object, void* pContext = nullptr)
  {
    ReclaimLater({object.objectType, (void*)object, pContext});
    object = nullptr;
  }

  void SetDebugName(const vk::DebugUtilsObjectNameInfoEXT& info, ezVulkanAllocation allocation = nullptr);

  template <typename T>
  void SetDebugName(const char* szName, T& object, ezVulkanAllocation allocation = nullptr)
  {
#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
    if (object)
    {
      vk::DebugUtilsObjectNameInfoEXT nameInfo;
      nameInfo.objectType = object.objectType;
      nameInfo.objectHandle = (uint64_t) static_cast<typename T::NativeType>(object);
      nameInfo.pObjectName = szName;

      SetDebugName(nameInfo, allocation);
    }
#endif
  }

  void ReportLiveGpuObjects();

  static void UploadBufferStaging(ezStagingBufferPoolVulkan* pStagingBufferPool, ezPipelineBarrierVulkan* pPipelineBarrier, vk::CommandBuffer commandBuffer, const ezGALBufferVulkan* pBuffer, ezArrayPtr<const ezUInt8> pInitialData, vk::DeviceSize dstOffset = 0);
  static void UploadTextureStaging(ezStagingBufferPoolVulkan* pStagingBufferPool, ezPipelineBarrierVulkan* pPipelineBarrier, vk::CommandBuffer commandBuffer, const ezGALTextureVulkan* pTexture, const vk::ImageSubresourceLayers& subResource, const ezGALSystemMemoryDescription& data);

  struct OnBeforeImageDestroyedData
  {
    vk::Image image;
    ezGALDeviceVulkan& GALDeviceVulkan;
  };
  ezEvent<OnBeforeImageDestroyedData> OnBeforeImageDestroyed;

  virtual const ezGALSharedTexture* GetSharedTexture(ezGALTextureHandle hTexture) const override;

  struct SemaphoreInfo
  {
    static SemaphoreInfo MakeWaitSemaphore(vk::Semaphore semaphore, vk::PipelineStageFlagBits waitStage = vk::PipelineStageFlagBits::eAllCommands, vk::SemaphoreType type = vk::SemaphoreType::eBinary, ezUInt64 uiValue = 0)
    {
      return SemaphoreInfo{semaphore, type, waitStage, uiValue};
    }

    static SemaphoreInfo MakeSignalSemaphore(vk::Semaphore semaphore, vk::SemaphoreType type = vk::SemaphoreType::eBinary, ezUInt64 uiValue = 0)
    {
      return SemaphoreInfo{semaphore, type, vk::PipelineStageFlagBits::eNone, uiValue};
    }

    vk::Semaphore m_semaphore;
    vk::SemaphoreType m_type = vk::SemaphoreType::eBinary;
    vk::PipelineStageFlagBits m_waitStage = vk::PipelineStageFlagBits::eAllCommands;
    ezUInt64 m_uiValue = 0;
  };
  void AddWaitSemaphore(const SemaphoreInfo& waitSemaphore);
  void AddSignalSemaphore(const SemaphoreInfo& signalSemaphore);

  // These functions need to be implemented by a render API abstraction
protected:
  // Init & shutdown functions

  vk::Result SelectInstanceExtensions(ezHybridArray<const char*, 6>& extensions);
  vk::Result SelectDeviceExtensions(vk::DeviceCreateInfo& deviceCreateInfo, ezHybridArray<const char*, 6>& extensions);

  virtual ezStringView GetRendererPlatform() override;
  virtual ezResult InitPlatform() override;
  virtual ezResult ShutdownPlatform() override;

  // Command encoder functions

  virtual ezGALCommandEncoder* BeginCommandsPlatform(const char* szName) override;
  virtual void EndCommandsPlatform(ezGALCommandEncoder* pPass) override;

  virtual void FlushPlatform() override;


  // State creation functions

  virtual ezGALBlendState* CreateBlendStatePlatform(const ezGALBlendStateCreationDescription& Description) override;
  virtual void DestroyBlendStatePlatform(ezGALBlendState* pBlendState) override;

  virtual ezGALDepthStencilState* CreateDepthStencilStatePlatform(const ezGALDepthStencilStateCreationDescription& Description) override;
  virtual void DestroyDepthStencilStatePlatform(ezGALDepthStencilState* pDepthStencilState) override;

  virtual ezGALRasterizerState* CreateRasterizerStatePlatform(const ezGALRasterizerStateCreationDescription& Description) override;
  virtual void DestroyRasterizerStatePlatform(ezGALRasterizerState* pRasterizerState) override;

  virtual ezGALSamplerState* CreateSamplerStatePlatform(const ezGALSamplerStateCreationDescription& Description) override;
  virtual void DestroySamplerStatePlatform(ezGALSamplerState* pSamplerState) override;


  // Resource creation functions

  virtual ezGALShader* CreateShaderPlatform(const ezGALShaderCreationDescription& Description) override;
  virtual void DestroyShaderPlatform(ezGALShader* pShader) override;

  virtual ezGALBuffer* CreateBufferPlatform(const ezGALBufferCreationDescription& Description, ezArrayPtr<const ezUInt8> pInitialData) override;
  virtual void DestroyBufferPlatform(ezGALBuffer* pBuffer) override;

  virtual ezGALTexture* CreateTexturePlatform(const ezGALTextureCreationDescription& Description, ezArrayPtr<ezGALSystemMemoryDescription> pInitialData) override;
  virtual void DestroyTexturePlatform(ezGALTexture* pTexture) override;

  virtual ezGALTexture* CreateSharedTexturePlatform(const ezGALTextureCreationDescription& Description, ezArrayPtr<ezGALSystemMemoryDescription> pInitialData, ezEnum<ezGALSharedTextureType> sharedType, ezGALPlatformSharedHandle handle) override;
  virtual void DestroySharedTexturePlatform(ezGALTexture* pTexture) override;

  virtual ezGALReadbackBuffer* CreateReadbackBufferPlatform(const ezGALBufferCreationDescription& Description) override;
  virtual void DestroyReadbackBufferPlatform(ezGALReadbackBuffer* pReadbackBuffer) override;

  virtual ezGALReadbackTexture* CreateReadbackTexturePlatform(const ezGALTextureCreationDescription& Description) override;
  virtual void DestroyReadbackTexturePlatform(ezGALReadbackTexture* pReadbackTexture) override;

  virtual ezGALTextureResourceView* CreateResourceViewPlatform(ezGALTexture* pResource, const ezGALTextureResourceViewCreationDescription& Description) override;
  virtual void DestroyResourceViewPlatform(ezGALTextureResourceView* pResourceView) override;

  virtual ezGALBufferResourceView* CreateResourceViewPlatform(ezGALBuffer* pResource, const ezGALBufferResourceViewCreationDescription& Description) override;
  virtual void DestroyResourceViewPlatform(ezGALBufferResourceView* pResourceView) override;

  virtual ezGALRenderTargetView* CreateRenderTargetViewPlatform(ezGALTexture* pTexture, const ezGALRenderTargetViewCreationDescription& Description) override;
  virtual void DestroyRenderTargetViewPlatform(ezGALRenderTargetView* pRenderTargetView) override;

  ezGALTextureUnorderedAccessView* CreateUnorderedAccessViewPlatform(ezGALTexture* pResource, const ezGALTextureUnorderedAccessViewCreationDescription& Description) override;
  virtual void DestroyUnorderedAccessViewPlatform(ezGALTextureUnorderedAccessView* pUnorderedAccessView) override;

  ezGALBufferUnorderedAccessView* CreateUnorderedAccessViewPlatform(ezGALBuffer* pResource, const ezGALBufferUnorderedAccessViewCreationDescription& Description) override;
  virtual void DestroyUnorderedAccessViewPlatform(ezGALBufferUnorderedAccessView* pUnorderedAccessView) override;

  virtual ezGALVertexDeclaration* CreateVertexDeclarationPlatform(const ezGALVertexDeclarationCreationDescription& Description) override;
  virtual void DestroyVertexDeclarationPlatform(ezGALVertexDeclaration* pVertexDeclaration) override;

  // GPU -> CPU query functions

  virtual ezEnum<ezGALAsyncResult> GetTimestampResultPlatform(ezGALTimestampHandle hTimestamp, ezTime& out_result) override;
  virtual ezEnum<ezGALAsyncResult> GetOcclusionResultPlatform(ezGALOcclusionHandle hOcclusion, ezUInt64& out_uiResult) override;
  virtual ezEnum<ezGALAsyncResult> GetFenceResultPlatform(ezGALFenceHandle hFence, ezTime timeout) override;
  virtual ezResult LockBufferPlatform(const ezGALReadbackBuffer* pBuffer, ezArrayPtr<const ezUInt8>& out_Memory) const override;
  virtual void UnlockBufferPlatform(const ezGALReadbackBuffer* pBuffer) const override;
  virtual ezResult LockTexturePlatform(const ezGALReadbackTexture* pTexture, const ezArrayPtr<const ezGALTextureSubresource>& subResources, ezDynamicArray<ezGALSystemMemoryDescription>& out_Memory) const override;
  virtual void UnlockTexturePlatform(const ezGALReadbackTexture* pTexture, const ezArrayPtr<const ezGALTextureSubresource>& subResources) const override;

  // Misc functions

  virtual void BeginFramePlatform(ezArrayPtr<ezGALSwapChain*> swapchains, const ezUInt64 uiAppFrame) override;
  virtual void EndFramePlatform(ezArrayPtr<ezGALSwapChain*> swapchains) override;
  virtual ezUInt64 GetCurrentFramePlatform() const override;
  virtual ezUInt64 GetSafeFramePlatform() const override;

  virtual void FillCapabilitiesPlatform() override;

  virtual void WaitIdlePlatform() override;

  /// \endcond

private:
  struct PerFrameData
  {
    /// \brief These are all fences passed into submit calls. For some reason waiting for the fence of the last submit is not enough. At least I can't get it to work (neither semaphores nor barriers make it past the validation layer).
    ezHybridArray<vk::Fence, 2> m_CommandBufferFences;

    vk::CommandBuffer m_currentCommandBuffer;
    ezUInt64 m_uiFrame = -1;

    ezMutex m_pendingDeletionsMutex;
    ezDeque<PendingDeletion> m_pendingDeletions;
    ezDeque<PendingDeletion> m_pendingDeletionsPrevious;

    ezMutex m_reclaimResourcesMutex;
    ezDeque<ReclaimResource> m_reclaimResources;
    ezDeque<ReclaimResource> m_reclaimResourcesPrevious;
  };

  void DeletePendingResources(ezDeque<PendingDeletion>& pendingDeletions);
  void ReclaimResources(ezDeque<ReclaimResource>& resources);

  void FillFormatLookupTable();

  static constexpr ezUInt32 FRAMES = 4;

  ezUInt64 m_uiFrameCounter = 1; ///< We start at 1 so m_uiFrameCounter and m_uiSafeFrame are not equal at the start.
  ezUInt64 m_uiSafeFrame = 0;
  ezUInt8 m_uiCurrentPerFrameData = m_uiFrameCounter % FRAMES;

  vk::Instance m_instance;
  vk::PhysicalDevice m_physicalDevice;
  vk::PhysicalDeviceProperties m_properties;
  vk::Device m_device;
  Queue m_graphicsQueue;
  Queue m_transferQueue;

  ezGALFormatLookupTableVulkan m_FormatLookupTable;
  vk::PipelineStageFlags m_supportedStages;
  vk::PhysicalDeviceMemoryProperties m_memoryProperties;

  ezUniquePtr<ezGALCommandEncoderImplVulkan> m_pCommandEncoderImpl;
  ezUniquePtr<ezGALCommandEncoder> m_pCommandEncoder;

  ezUniquePtr<ezPipelineBarrierVulkan> m_pPipelineBarrier;
  ezUniquePtr<ezCommandBufferPoolVulkan> m_pCommandBufferPool;
  ezUniquePtr<ezStagingBufferPoolVulkan> m_pStagingBufferPool;
  ezUniquePtr<ezQueryPoolVulkan> m_pQueryPool;
  ezUniquePtr<ezFenceQueueVulkan> m_pFenceQueue;
  ezUniquePtr<ezInitContextVulkan> m_pInitContext;

  // We daisy-chain all command buffers in a frame in sequential order via this semaphore for now.
  vk::Semaphore m_lastCommandBufferFinished;

  PerFrameData m_PerFrameData[FRAMES];

#if EZ_ENABLED(EZ_USE_PROFILING)
  struct GPUTimingScope* m_pFrameTimingScope = nullptr;
  struct GPUTimingScope* m_pPipelineTimingScope = nullptr;
  struct GPUTimingScope* m_pPassTimingScope = nullptr;
#endif

  Extensions m_extensions;
  ezVulkanDispatchContext m_dispatchContext;
#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
#endif
  ezHybridArray<SemaphoreInfo, 3> m_waitSemaphores;
  ezHybridArray<SemaphoreInfo, 3> m_signalSemaphores;
};

#include <RendererVulkan/Device/Implementation/DeviceVulkan_inl.h>
