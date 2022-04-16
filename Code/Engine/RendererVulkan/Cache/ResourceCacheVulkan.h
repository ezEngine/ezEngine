#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

#include <Foundation/Algorithm/HashStream.h>
#include <Foundation/Math/Size.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>
#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Resources/RenderTargetViewVulkan.h>

#include <vulkan/vulkan.hpp>

class ezGALRasterizerStateVulkan;
class ezGALBlendStateVulkan;
class ezGALDepthStencilStateVulkan;
class ezGALShaderVulkan;
class ezGALVertexDeclarationVulkan;

EZ_DEFINE_AS_POD_TYPE(vk::DynamicState);

/// \brief Creates and caches persistent Vulkan resources. Resources are never freed until the device is shut down.
class EZ_RENDERERVULKAN_DLL ezResourceCacheVulkan
{
public:
  static void Initialize(ezGALDeviceVulkan* pDevice, vk::Device device);
  static void DeInitialize();

  static vk::RenderPass RequestRenderPass(const ezGALRenderingSetup& renderingSetup);
  static vk::Framebuffer RequestFrameBuffer(vk::RenderPass renderPass, const ezGALRenderTargetSetup& renderTargetSetup, ezSizeU32& out_Size, ezGALMSAASampleCount& out_msaa);

  struct PipelineLayoutDesc
  {
    EZ_DECLARE_POD_TYPE();
    int m_TODO = 0;
  };

  struct GraphicsPipelineDesc
  {
    EZ_DECLARE_POD_TYPE();
    vk::RenderPass m_renderPass;
    vk::PipelineLayout m_layout;
    ezEnum<ezGALPrimitiveTopology> m_topology;
    ezEnum<ezGALMSAASampleCount> m_msaa;
    ezUInt8 m_uiAttachmentCount = 0;
    const ezGALRasterizerStateVulkan* m_pCurrentRasterizerState = nullptr;
    const ezGALBlendStateVulkan* m_pCurrentBlendState = nullptr;
    const ezGALDepthStencilStateVulkan* m_pCurrentDepthStencilState = nullptr;
    const ezGALShaderVulkan* m_pCurrentShader = nullptr;
    const ezGALVertexDeclarationVulkan* m_pCurrentVertexDecl = nullptr;
    ezUInt32 m_VertexBufferStrides[EZ_GAL_MAX_VERTEX_BUFFER_COUNT] = {};
  };

  static vk::PipelineLayout RequestPipelineLayout(const PipelineLayoutDesc& desc);
  static vk::Pipeline RequestGraphicsPipeline(const GraphicsPipelineDesc& desc);

private:
  struct FramebufferKey
  {
    vk::RenderPass m_renderPass;
    ezGALRenderTargetSetup m_renderTargetSetup;
  };

  /// \brief Hashable version without pointers of vk::FramebufferCreateInfo
  struct FramebufferDesc
  {
    VkRenderPass renderPass;
    ezSizeU32 m_size = {0, 0};
    uint32_t layers = 1;
    ezHybridArray<vk::ImageView, EZ_GAL_MAX_RENDERTARGET_COUNT + 1> attachments;
    ezEnum<ezGALMSAASampleCount> m_msaa;
  };

  /// \brief Hashable version without pointers or redundant data of vk::AttachmentDescription
  struct AttachmentDesc
  {
    EZ_DECLARE_POD_TYPE();
    vk::Format format = vk::Format::eUndefined;
    vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1;
    // Not set at all right now
    vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eSampled;
    // Not set at all right now
    vk::ImageLayout initialLayout = vk::ImageLayout::eUndefined;
    // No support for eDontCare in EZ right now
    vk::AttachmentLoadOp loadOp = vk::AttachmentLoadOp::eClear;
    vk::AttachmentStoreOp storeOp = vk::AttachmentStoreOp::eStore;
    vk::AttachmentLoadOp stencilLoadOp = vk::AttachmentLoadOp::eClear;
    vk::AttachmentStoreOp stencilStoreOp = vk::AttachmentStoreOp::eStore;
  };

  /// \brief Hashable version without pointers of vk::RenderPassCreateInfo
  struct RenderPassDesc
  {
    ezHybridArray<AttachmentDesc, EZ_GAL_MAX_RENDERTARGET_COUNT> attachments;
  };

  struct ResourceCacheHash
  {
    static ezUInt32 Hash(const RenderPassDesc& renderingSetup);
    static bool Equal(const RenderPassDesc& a, const RenderPassDesc& b);

    static ezUInt32 Hash(const ezGALRenderingSetup& renderingSetup);
    static bool Equal(const ezGALRenderingSetup& a, const ezGALRenderingSetup& b);

    static ezUInt32 Hash(const FramebufferKey& renderTargetSetup);
    static bool Equal(const FramebufferKey& a, const FramebufferKey& b);

    static ezUInt32 Hash(const PipelineLayoutDesc& desc);
    static bool Equal(const PipelineLayoutDesc& a, const PipelineLayoutDesc& b);

    static ezUInt32 Hash(const GraphicsPipelineDesc& desc);
    static bool Equal(const GraphicsPipelineDesc& a, const GraphicsPipelineDesc& b);
  };

  struct FrameBufferCache
  {
    vk::Framebuffer m_frameBuffer;
    ezSizeU32 m_size;
    ezGALMSAASampleCount m_msaa;
    EZ_DECLARE_POD_TYPE();
  };

  static vk::RenderPass RequestRenderPassInternal(const RenderPassDesc& desc);
  static void GetRenderPassDesc(const ezGALRenderingSetup& renderingSetup, RenderPassDesc& out_desc);
  static void GetFrameBufferDesc(vk::RenderPass renderPass, const ezGALRenderTargetSetup& renderTargetSetup, FramebufferDesc& out_desc);

private:
  static ezGALDeviceVulkan* s_pDevice;
  static vk::Device s_device;
  // We have a N to 1 mapping for ezGALRenderingSetup to vk::RenderPass as multiple ezGALRenderingSetup can share the same RenderPassDesc.
  // Thus, we have a two stage resolve to the vk::RenderPass. If a ezGALRenderingSetup is not present in s_shallowRenderPasses we create the RenderPassDesc which has a 1 to 1 relationship with vk::RenderPass and look that one up in s_renderPasses. Finally we add the entry to s_shallowRenderPasses to make sure a shallow lookup will work on the next query.
  static ezHashTable<ezGALRenderingSetup, vk::RenderPass, ResourceCacheHash> s_shallowRenderPasses;
  static ezHashTable<RenderPassDesc, vk::RenderPass, ResourceCacheHash> s_renderPasses;
  static ezHashTable<FramebufferKey, FrameBufferCache, ResourceCacheHash> s_frameBuffers;

  static ezHashTable<PipelineLayoutDesc, vk::PipelineLayout, ResourceCacheHash> s_pipelineLayouts;
  static ezHashTable<GraphicsPipelineDesc, vk::Pipeline, ResourceCacheHash> s_graphicsPipelines;
};
