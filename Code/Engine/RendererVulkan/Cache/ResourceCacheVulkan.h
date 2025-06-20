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

  static vk::PipelineCache GetPipelineCache() { return s_pipelineCache; }
  static vk::RenderPass RequestRenderPass(const ezGALRenderPassDescriptor& renderPass);
  static vk::Framebuffer RequestFrameBuffer(vk::RenderPass vkRenderPass, const ezGALFrameBufferDescriptor& frameBuffer);

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
  };

private:
  static ezResult SavePipelineCache();
  static ezResult LoadPipelineCache(vk::PipelineCache& out_pipelineCache);

private:
  static ezGALDeviceVulkan* s_pDevice;
  static vk::Device s_device;
  static vk::PipelineCache s_pipelineCache;
  // We have a N to 1 mapping for ezGALRenderingSetup to vk::RenderPass as multiple ezGALRenderingSetup can share the same RenderPassDesc.
  static ezHashTable<ezGALRenderPassDescriptor, vk::RenderPass, ResourceCacheHash> s_renderPasses;
  static ezHashTable<FramebufferKey, vk::Framebuffer, ResourceCacheHash> s_frameBuffers; // #TODO_VULKAN cache invalidation
};
