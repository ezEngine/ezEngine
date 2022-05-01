
#pragma once

#include <Foundation/Types/Bitflags.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererVulkan/RendererVulkanDLL.h>

#include <vulkan/vulkan.hpp>

using ezGALFormatLookupEntryVulkan = ezGALFormatLookupEntry<vk::Format, (vk::Format)0>;
using ezGALFormatLookupTableVulkan = ezGALFormatLookupTable<ezGALFormatLookupEntryVulkan>;

class ezGALBufferVulkan;
class ezGALTextureVulkan;
class ezGALPassVulkan;

/// \brief The Vulkan device implementation of the graphics abstraction layer.
class EZ_RENDERERVULKAN_DLL ezGALDeviceVulkan : public ezGALDevice
{
private:
  friend ezInternal::NewInstance<ezGALDevice> CreateVulkanDevice(ezAllocatorBase* pAllocator, const ezGALDeviceCreationDescription& Description);
  ezGALDeviceVulkan(const ezGALDeviceCreationDescription& Description);

public:
  virtual ~ezGALDeviceVulkan();

public:
  struct PendingDeletion
  {
    EZ_DECLARE_POD_TYPE();
    vk::ObjectType m_type;
    void* m_pObject;
    ezVulkanAllocation m_allocation;
  };

  struct ReclaimResource
  {
    EZ_DECLARE_POD_TYPE();
    vk::ObjectType m_type;
    void* m_pObject;
  };

  struct Extensions
  {
    bool m_bSurface = false;
    bool m_bWin32Surface = false;

    bool m_bDebugUtils = false;
    PFN_vkCreateDebugUtilsMessengerEXT pfn_vkCreateDebugUtilsMessengerEXT;
    PFN_vkDestroyDebugUtilsMessengerEXT pfn_vkDestroyDebugUtilsMessengerEXT;
    PFN_vkSetDebugUtilsObjectNameEXT pfn_vkSetDebugUtilsObjectNameEXT;

    bool m_bDeviceSwapChain = false;
  };

  struct Queue
  {
    vk::Queue m_queue;
    ezUInt32 m_uiQueueFamily = -1;
    ezUInt32 m_uiQueueIndex = 0;
  };

  ezUInt64 GetCurrentFrame() const { return m_uiFrameCounter;}
  ezUInt64 GetSafeFrame() const { return m_uiSafeFrame; }

  vk::Instance GetVulkanInstance() const;
  vk::Device GetVulkanDevice() const;
  const Queue& GetGraphicsQueue() const;
  const Queue& GetTransferQueue() const;

  vk::PhysicalDevice GetVulkanPhysicalDevice() const;
  const vk::PhysicalDeviceProperties& GetPhysicalDeviceProperties() const { return m_properties; }
  const Extensions& GetExtensions() const { return m_extensions; }
  vk::PipelineStageFlags GetSupportedStages() const;

  vk::CommandBuffer& GetCurrentCommandBuffer();

  const ezGALFormatLookupTableVulkan& GetFormatLookupTable() const;

  ezInt32 GetMemoryIndex(vk::MemoryPropertyFlags properties, const vk::MemoryRequirements& requirements) const;

  vk::Fence Submit(vk::Semaphore waitSemaphore, vk::PipelineStageFlags waitStage, vk::Semaphore signalSemaphore);

  void DeleteLater(const PendingDeletion& deletion);
  template <typename T>
  void DeleteLater(T& object, ezVulkanAllocation& allocation)
  {
    if (object)
    {
      DeleteLater({object.objectType, (void*)object, allocation});
    }
    object = nullptr;
    allocation = nullptr;
  }

  template <typename T>
  void DeleteLater(T& object)
  {
    if (object)
    {
      DeleteLater({object.objectType, (void*)object, nullptr});
    }
    object = nullptr;
  }

  void ReclaimLater(const ReclaimResource& reclaim);

  template <typename T>
  void ReclaimLater(T& object)
  {
    ReclaimLater({object.objectType, (void*)object});
    object = nullptr;
  }

  void SetDebugName(const vk::DebugUtilsObjectNameInfoEXT& info, ezVulkanAllocation allocation = nullptr);

  template <typename T>
  void SetDebugName(const char* szName, T& object, ezVulkanAllocation allocation = nullptr)
  {
    if (object)
    {
      vk::DebugUtilsObjectNameInfoEXT nameInfo;
      nameInfo.objectType = object.objectType;
      nameInfo.objectHandle = (uint64_t)static_cast<T::NativeType>(object);
      nameInfo.pObjectName = szName;

      SetDebugName(nameInfo, allocation);
    }
  }

  void ReportLiveGpuObjects();

  void UploadBufferStaging(const ezGALBufferVulkan* pBuffer, ezArrayPtr<const ezUInt8> pInitialData, vk::DeviceSize dstOffset = 0);

  // These functions need to be implemented by a render API abstraction
protected:
  // Init & shutdown functions

  vk::Result SelectInstanceExtensions(ezHybridArray<const char*, 6>& extensions);
  vk::Result SelectDeviceExtensions(ezHybridArray<const char*, 6>& extensions);

  virtual ezResult InitPlatform() override;
  virtual ezResult ShutdownPlatform() override;

  // Pipeline & Pass functions

  virtual void BeginPipelinePlatform(const char* szName, ezGALSwapChain* pSwapChain) override;
  virtual void EndPipelinePlatform(ezGALSwapChain* pSwapChain) override;

  virtual ezGALPass* BeginPassPlatform(const char* szName) override;
  virtual void EndPassPlatform(ezGALPass* pPass) override;


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

  virtual ezGALResourceView* CreateResourceViewPlatform(ezGALResourceBase* pResource, const ezGALResourceViewCreationDescription& Description) override;
  virtual void DestroyResourceViewPlatform(ezGALResourceView* pResourceView) override;

  virtual ezGALRenderTargetView* CreateRenderTargetViewPlatform(ezGALTexture* pTexture, const ezGALRenderTargetViewCreationDescription& Description) override;
  virtual void DestroyRenderTargetViewPlatform(ezGALRenderTargetView* pRenderTargetView) override;

  ezGALUnorderedAccessView* CreateUnorderedAccessViewPlatform(ezGALResourceBase* pResource, const ezGALUnorderedAccessViewCreationDescription& Description) override;
  virtual void DestroyUnorderedAccessViewPlatform(ezGALUnorderedAccessView* pResource) override;

  // Other rendering creation functions

  virtual ezGALQuery* CreateQueryPlatform(const ezGALQueryCreationDescription& Description) override;
  virtual void DestroyQueryPlatform(ezGALQuery* pQuery) override;

  virtual ezGALVertexDeclaration* CreateVertexDeclarationPlatform(const ezGALVertexDeclarationCreationDescription& Description) override;
  virtual void DestroyVertexDeclarationPlatform(ezGALVertexDeclaration* pVertexDeclaration) override;

  // Timestamp functions

  virtual ezGALTimestampHandle GetTimestampPlatform() override;
  virtual ezResult GetTimestampResultPlatform(ezGALTimestampHandle hTimestamp, ezTime& result) override;

  // Misc functions

  virtual void BeginFramePlatform(const ezUInt64 uiRenderFrame) override;
  virtual void EndFramePlatform() override;

  virtual void FillCapabilitiesPlatform() override;

  /// \endcond

private:

  struct PerFrameData
  {
    /// \brief These are all fences passed into submit calls. For some reason waiting for the fence of the last submit is not enough. At least I can't get it to work (neither semaphores nor barriers make it past the validation layer).
    ezHybridArray<vk::Fence, 2> m_CommandBufferFences;

    vk::CommandBuffer m_currentCommandBuffer;
    //ID3D11Query* m_pDisjointTimerQuery = nullptr;
    double m_fInvTicksPerSecond = -1.0;
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

  ezUInt64 m_uiFrameCounter = 0;
  ezUInt64 m_uiSafeFrame = 0;
  ezUInt8 m_uiCurrentPerFrameData = 0;
  ezUInt8 m_uiNextPerFrameData = 1;

  vk::Instance m_instance;
  vk::PhysicalDevice m_physicalDevice;
  vk::PhysicalDeviceProperties m_properties;
  vk::Device m_device;
  Queue m_graphicsQueue;
  Queue m_transferQueue;

  ezGALFormatLookupTableVulkan m_FormatLookupTable;

  vk::PhysicalDeviceMemoryProperties m_memoryProperties;

  ezUniquePtr<ezGALPassVulkan> m_pDefaultPass;

  // We daisy-chain all command buffers in a frame in sequential order via this semaphore for now.
  vk::Semaphore m_lastCommandBufferFinished;

  PerFrameData m_PerFrameData[4];

  ezTime m_SyncTimeDiff;
  bool m_bSyncTimeNeeded = true;

  Extensions m_extensions;
#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  VkDebugUtilsMessengerEXT m_debugMessenger;
#endif
};

#include <RendererVulkan/Device/Implementation/DeviceVulkan_inl.h>
