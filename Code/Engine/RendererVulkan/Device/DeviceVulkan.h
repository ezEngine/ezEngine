
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

  /// \todo This shouldn't be accessible, there should be a factory instantiating the correct renderer class via RTTI for example
public:
  ezGALDeviceVulkan(const ezGALDeviceCreationDescription& Description);

  virtual ~ezGALDeviceVulkan();


public:
  vk::Instance GetVulkanInstance() const;
  vk::Device GetVulkanDevice() const;

  vk::CommandBuffer& GetPrimaryCommandBuffer();

  ezArrayPtr<const ezUInt32> GetQueueFamilyIndices() const;
  vk::Queue GetQueue();

  const ezGALFormatLookupTableVulkan& GetFormatLookupTable() const;

  ezInt32 GetMemoryIndex(vk::MemoryPropertyFlags properties, const vk::MemoryRequirements& requirements) const;

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

  virtual ezResult InitPlatform() override;
  virtual ezResult ShutdownPlatform() override;

  // Pass functions

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

  virtual ezGALSwapChain* CreateSwapChainPlatform(const ezGALSwapChainCreationDescription& Description) override;
  virtual void DestroySwapChainPlatform(ezGALSwapChain* pSwapChain) override;

  virtual ezGALFence* CreateFencePlatform() override;
  virtual void DestroyFencePlatform(ezGALFence* pFence) override;

  virtual ezGALQuery* CreateQueryPlatform(const ezGALQueryCreationDescription& Description) override;
  virtual void DestroyQueryPlatform(ezGALQuery* pQuery) override;

  virtual ezGALVertexDeclaration* CreateVertexDeclarationPlatform(const ezGALVertexDeclarationCreationDescription& Description) override;
  virtual void DestroyVertexDeclarationPlatform(ezGALVertexDeclaration* pVertexDeclaration) override;

  // Timestamp functions

  virtual ezGALTimestampHandle GetTimestampPlatform() override;
  virtual ezResult GetTimestampResultPlatform(ezGALTimestampHandle hTimestamp, ezTime& result) override;

  // Swap chain functions

  virtual void PresentPlatform(ezGALSwapChain* pSwapChain, bool bVSync) override;

  // Misc functions

  virtual void BeginFramePlatform() override;
  virtual void EndFramePlatform() override;

  virtual void SetPrimarySwapChainPlatform(ezGALSwapChain* pSwapChain) override;

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
    ezGALFence* m_pFence = nullptr;
    //ID3D11Query* m_pDisjointTimerQuery = nullptr;
    double m_fInvTicksPerSecond = -1.0;
    ezUInt64 m_uiFrame = -1;
  };

  void FreeTempResources(ezUInt64 uiFrame);

  //ID3D11Query* GetTimestamp(ezGALTimestampHandle hTimestamp);

  void FillFormatLookupTable();

  vk::Instance m_instance;
  vk::PhysicalDevice m_physicalDevice;
  vk::Device m_device;
  vk::Queue m_queue;

  ezHybridArray<ezUInt32, 2> m_queueFamilyIndices;
  ezGALFormatLookupTableVulkan m_FormatLookupTable;

  vk::PhysicalDeviceMemoryProperties m_memoryProperties;

  vk::CommandPool m_commandPool;

  static constexpr ezUInt32 NUM_CMD_BUFFERS = 4;
  vk::CommandBuffer m_commandBuffers[NUM_CMD_BUFFERS];
  vk::Fence m_commandBufferFences[NUM_CMD_BUFFERS];
  ezUInt32 m_uiCurrentCmdBufferIndex = 0;

  ezUniquePtr<ezGALPassVulkan> m_pDefaultPass;

  PerFrameData m_PerFrameData[4];
  ezUInt8 m_uiCurrentPerFrameData = 0;
  ezUInt8 m_uiNextPerFrameData = 0;

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
