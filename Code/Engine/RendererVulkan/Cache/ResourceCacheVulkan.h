#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

#include <Foundation/Algorithm/HashStream.h>
#include <Foundation/Math/Size.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>
#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Resources/RenderTargetViewVulkan.h>
#include <RendererVulkan/Shader/ShaderVulkan.h>

#include <vulkan/vulkan.hpp>

class ezGALRasterizerStateVulkan;
class ezGALBlendStateVulkan;
class ezGALDepthStencilStateVulkan;
class ezGALShaderVulkan;
class ezGALVertexDeclarationVulkan;
class ezRefCounted;

EZ_DEFINE_AS_POD_TYPE(vk::DynamicState);

/// \brief Creates and caches persistent Vulkan resources. Resources are never freed until the device is shut down.
class EZ_RENDERERVULKAN_DLL ezResourceCacheVulkan
{
public:
  static void Initialize(ezGALDeviceVulkan* pDevice, vk::Device device);
  static void DeInitialize();

  static vk::RenderPass RequestRenderPass(const ezGALRenderingSetup& renderingSetup);
  static vk::Framebuffer RequestFrameBuffer(vk::RenderPass renderPass, const ezGALRenderTargetSetup& renderTargetSetup, ezSizeU32& out_Size, ezEnum<ezGALMSAASampleCount>& out_msaa, ezUInt32& out_uiLayers);

  struct PipelineLayoutDesc
  {
    ezHybridArray<vk::DescriptorSetLayout, 4> m_layout;
    vk::PushConstantRange m_pushConstants;
  };

  struct GraphicsPipelineDesc
  {
    EZ_DECLARE_POD_TYPE();
    vk::RenderPass m_renderPass;     // Created from ezGALRenderingSetup
    vk::PipelineLayout m_layout;     // Created from shader
    ezEnum<ezGALPrimitiveTopology> m_topology;
    ezEnum<ezGALMSAASampleCount> m_msaa;
    ezUInt8 m_uiAttachmentCount = 0; // DX12 requires format list for RT and DT
    const ezGALRasterizerStateVulkan* m_pCurrentRasterizerState = nullptr;
    const ezGALBlendStateVulkan* m_pCurrentBlendState = nullptr;
    const ezGALDepthStencilStateVulkan* m_pCurrentDepthStencilState = nullptr;
    const ezGALShaderVulkan* m_pCurrentShader = nullptr;
    const ezGALVertexDeclarationVulkan* m_pCurrentVertexDecl = nullptr;
  };

  struct ComputePipelineDesc
  {
    EZ_DECLARE_POD_TYPE();
    vk::PipelineLayout m_layout;
    const ezGALShaderVulkan* m_pCurrentShader = nullptr;
  };

  static vk::PipelineLayout RequestPipelineLayout(const PipelineLayoutDesc& desc);
  static vk::Pipeline RequestGraphicsPipeline(const GraphicsPipelineDesc& desc);
  static vk::Pipeline RequestComputePipeline(const ComputePipelineDesc& desc);

  struct DescriptorSetLayoutDesc
  {
    mutable ezUInt32 m_uiHash = 0;
    ezHybridArray<vk::DescriptorSetLayoutBinding, 6> m_bindings;
  };
  static vk::DescriptorSetLayout RequestDescriptorSetLayout(const ezGALShaderVulkan::DescriptorSetLayoutDesc& desc);

  /// \brief Invalidates any caches that use this resource. Basically all pointer types in GraphicsPipelineDesc except for ezGALShaderVulkan.
  static void ResourceDeleted(const ezRefCounted* pResource);
  /// \brief Invalidates any caches that use this shader resource.
  static void ShaderDeleted(const ezGALShaderVulkan* pShader);

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

    static bool Less(const GraphicsPipelineDesc& a, const GraphicsPipelineDesc& b);
    static ezUInt32 Hash(const GraphicsPipelineDesc& desc);
    static bool Equal(const GraphicsPipelineDesc& a, const GraphicsPipelineDesc& b);

    static bool Less(const ComputePipelineDesc& a, const ComputePipelineDesc& b);
    static bool Equal(const ComputePipelineDesc& a, const ComputePipelineDesc& b);

    static ezUInt32 Hash(const ezGALShaderVulkan::DescriptorSetLayoutDesc& desc) { return desc.m_uiHash; }
    static bool Equal(const ezGALShaderVulkan::DescriptorSetLayoutDesc& a, const ezGALShaderVulkan::DescriptorSetLayoutDesc& b);
  };

  struct FrameBufferCache
  {
    vk::Framebuffer m_frameBuffer;
    ezSizeU32 m_size;
    ezEnum<ezGALMSAASampleCount> m_msaa;
    ezUInt32 m_layers = 0;
    EZ_DECLARE_POD_TYPE();
  };

  static vk::RenderPass RequestRenderPassInternal(const RenderPassDesc& desc);
  static void GetRenderPassDesc(const ezGALRenderingSetup& renderingSetup, RenderPassDesc& out_desc);
  static void GetFrameBufferDesc(vk::RenderPass renderPass, const ezGALRenderTargetSetup& renderTargetSetup, FramebufferDesc& out_desc);

public:
  using GraphicsPipelineMap = ezMap<ezResourceCacheVulkan::GraphicsPipelineDesc, vk::Pipeline, ezResourceCacheVulkan::ResourceCacheHash>;
  using ComputePipelineMap = ezMap<ezResourceCacheVulkan::ComputePipelineDesc, vk::Pipeline, ezResourceCacheVulkan::ResourceCacheHash>;


private:
  static ezGALDeviceVulkan* s_pDevice;
  static vk::Device s_device;
  // We have a N to 1 mapping for ezGALRenderingSetup to vk::RenderPass as multiple ezGALRenderingSetup can share the same RenderPassDesc.
  // Thus, we have a two stage resolve to the vk::RenderPass. If a ezGALRenderingSetup is not present in s_shallowRenderPasses we create the RenderPassDesc which has a 1 to 1 relationship with vk::RenderPass and look that one up in s_renderPasses. Finally we add the entry to s_shallowRenderPasses to make sure a shallow lookup will work on the next query.
  static ezHashTable<ezGALRenderingSetup, vk::RenderPass, ResourceCacheHash> s_shallowRenderPasses; // #TODO_VULKAN cache invalidation
  static ezHashTable<RenderPassDesc, vk::RenderPass, ResourceCacheHash> s_renderPasses;
  static ezHashTable<FramebufferKey, FrameBufferCache, ResourceCacheHash> s_frameBuffers;           // #TODO_VULKAN cache invalidation

  static ezHashTable<PipelineLayoutDesc, vk::PipelineLayout, ResourceCacheHash> s_pipelineLayouts;
  static GraphicsPipelineMap s_graphicsPipelines;
  static ComputePipelineMap s_computePipelines;
  static ezMap<const ezRefCounted*, ezHybridArray<GraphicsPipelineMap::Iterator, 1>> s_graphicsPipelineUsedBy;
  static ezMap<const ezRefCounted*, ezHybridArray<ComputePipelineMap::Iterator, 1>> s_computePipelineUsedBy;

  static ezHashTable<ezGALShaderVulkan::DescriptorSetLayoutDesc, vk::DescriptorSetLayout, ResourceCacheHash> s_descriptorSetLayouts;
};
