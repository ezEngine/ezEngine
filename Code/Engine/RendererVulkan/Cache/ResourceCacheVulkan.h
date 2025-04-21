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

  static vk::RenderPass RequestRenderPass(const ezGALRenderPassDescriptor& renderPass);
  static vk::Framebuffer RequestFrameBuffer(vk::RenderPass vkRenderPass, const ezGALFrameBufferDescriptor& frameBuffer);

  struct PipelineLayoutDesc
  {
    ezHybridArray<vk::DescriptorSetLayout, 4> m_layout;
    vk::PushConstantRange m_pushConstants;
  };

  struct GraphicsPipelineDesc
  {
    EZ_DECLARE_POD_TYPE();
    ezGALRenderPassDescriptor m_renderPassDesc;
    vk::RenderPass m_renderPass; // Created from ezGALRenderPassDescriptor above
    vk::PipelineLayout m_layout; // Created from shader (descriptor sets)
    ezEnum<ezGALPrimitiveTopology> m_topology;
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
    ezGALFrameBufferDescriptor m_frameBuffer;
  };


  struct ResourceCacheHash
  {
    static ezUInt32 Hash(const ezGALRenderPassDescriptor& renderingSetup);
    static bool Equal(const ezGALRenderPassDescriptor& a, const ezGALRenderPassDescriptor& b);

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

public:
  using GraphicsPipelineMap = ezMap<ezResourceCacheVulkan::GraphicsPipelineDesc, vk::Pipeline, ezResourceCacheVulkan::ResourceCacheHash>;
  using ComputePipelineMap = ezMap<ezResourceCacheVulkan::ComputePipelineDesc, vk::Pipeline, ezResourceCacheVulkan::ResourceCacheHash>;


private:
  static ezGALDeviceVulkan* s_pDevice;
  static vk::Device s_device;
  static ezHashTable<ezGALRenderPassDescriptor, vk::RenderPass, ResourceCacheHash> s_renderPasses;
  static ezHashTable<FramebufferKey, vk::Framebuffer, ResourceCacheHash> s_frameBuffers; // #TODO_VULKAN cache invalidation

  static ezHashTable<PipelineLayoutDesc, vk::PipelineLayout, ResourceCacheHash> s_pipelineLayouts;
  static GraphicsPipelineMap s_graphicsPipelines;
  static ComputePipelineMap s_computePipelines;
  static ezMap<const ezRefCounted*, ezHybridArray<GraphicsPipelineMap::Iterator, 1>> s_graphicsPipelineUsedBy;
  static ezMap<const ezRefCounted*, ezHybridArray<ComputePipelineMap::Iterator, 1>> s_computePipelineUsedBy;

  static ezHashTable<ezGALShaderVulkan::DescriptorSetLayoutDesc, vk::DescriptorSetLayout, ResourceCacheHash> s_descriptorSetLayouts;
};
