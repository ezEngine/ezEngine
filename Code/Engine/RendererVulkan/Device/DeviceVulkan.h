
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

  vk::Instance GetVulkanInstance() const;
  vk::Device GetVulkanDevice() const;
  vk::Queue GetVulkanQueue() const;
  vk::PhysicalDevice GetVulkanPhysicalDevice() const;
  const Extensions& GetExtensions() const { return m_extensions; }

  vk::CommandBuffer& GetCurrentCommandBuffer();

  ezArrayPtr<const ezUInt32> GetQueueFamilyIndices() const;
  vk::Queue GetQueue();

  const ezGALFormatLookupTableVulkan& GetFormatLookupTable() const;

  ezInt32 GetMemoryIndex(vk::MemoryPropertyFlags properties, const vk::MemoryRequirements& requirements) const;

  vk::Fence Submit(vk::Semaphore waitSemaphore, vk::PipelineStageFlags waitStage, vk::Semaphore signalSemaphore);

  void DeleteLater(const PendingDeletion& deletion);
  template <typename T>
  void DeleteLater(T& object, ezVulkanAllocation& allocation)
  {
    DeleteLater({object.objectType, (void*)object, allocation});
    object = nullptr;
    allocation = nullptr;
  }

  template <typename T>
  void DeleteLater(T& object)
  {
    DeleteLater({object.objectType, (void*)object, nullptr});
    object = nullptr;
  }

  void ReclaimLater(const ReclaimResource& reclaim);

  template <typename T>
  void ReclaimLater(T& object)
  {
    ReclaimLater({object.objectType, (void*)object});
    object = nullptr;
  }

  void ReportLiveGpuObjects();

  ezGALBufferVulkan* FindTempBuffer(ezUInt32 uiSize) { return nullptr; }                                                                           // TODO impl
  ezGALTextureVulkan* FindTempTexture(ezUInt32 uiWidth, ezUInt32 uiHeight, ezUInt32 uiDepth, ezGALResourceFormat::Enum format) { return nullptr; } // TODO impl

  // These functions need to be implemented by a render API abstraction
protected:
  // Init & shutdown functions

  /// \brief Internal version of device init that allows to modify device creation flags and graphics adapter.
  ///
  /// \param pUsedAdapter
  ///   Null means default adapter.
  //ezResult InitPlatform(DWORD flags, IDXGIAdapter* pUsedAdapter);

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
  struct TempResourceType
  {
    enum Enum
    {
      Buffer,
      Texture,

      ENUM_COUNT
    };
  };

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

    ezMutex m_reclaimResourcesMutex;
    ezDeque<ReclaimResource> m_reclaimResources;
  };

  void DeletePendingResources(ezDeque<PendingDeletion>& pendingDeletions);
  void ReclaimResources(ezDeque<ReclaimResource>& resources);

  void FreeTempResources(ezUInt64 uiFrame);

  //ID3D11Query* GetTimestamp(ezGALTimestampHandle hTimestamp);

  void FillFormatLookupTable();

  Extensions m_extensions;
#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  VkDebugUtilsMessengerEXT m_debugMessenger;
#endif

  vk::Instance m_instance;
  vk::PhysicalDevice m_physicalDevice;
  vk::Device m_device;
  vk::Queue m_queue;

  ezHybridArray<ezUInt32, 2> m_queueFamilyIndices;
  ezGALFormatLookupTableVulkan m_FormatLookupTable;

  vk::PhysicalDeviceMemoryProperties m_memoryProperties;

  ezUniquePtr<ezGALPassVulkan> m_pDefaultPass;

  // We daisy-chain all command buffers in a frame in sequential order via this semaphore for now.
  vk::Semaphore m_lastCommandBufferFinished;

  PerFrameData m_PerFrameData[4];
  ezUInt8 m_uiCurrentPerFrameData = 0;
  ezUInt8 m_uiNextPerFrameData = 1;

  ezUInt64 m_uiFrameCounter = 0;

  struct VkResource
  {
    enum class Type
    {
      Buffer,
      Image
    };

    Type m_type;

    union
    {
      VkImage* m_pImage;
      VkBuffer* m_pBuffer;
    };

    void Release()
    {
      // TODO
    }
  };

  struct UsedTempResource
  {
    EZ_DECLARE_POD_TYPE();

    VkResource* m_pResource;
    ezUInt64 m_uiFrame;
    ezUInt32 m_uiHash;
  };

  ezMap<ezUInt32, ezDynamicArray<VkResource*>, ezCompareHelper<ezUInt32>, ezLocalAllocatorWrapper> m_FreeTempResources[TempResourceType::ENUM_COUNT];
  ezDeque<UsedTempResource, ezLocalAllocatorWrapper> m_UsedTempResources[TempResourceType::ENUM_COUNT];

  ezDynamicArray<VkResource*, ezLocalAllocatorWrapper> m_Timestamps;
  ezUInt32 m_uiCurrentTimestamp = 0;
  ezUInt32 m_uiNextTimestamp = 0;

  ezTime m_SyncTimeDiff;
  bool m_bSyncTimeNeeded = true;
};

#include <RendererVulkan/Device/Implementation/DeviceVulkan_inl.h>
