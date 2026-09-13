#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

#include <Foundation/Algorithm/HashStream.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>
#include <RendererFoundation/Utils/DependencyTracker.h>
#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Shader/ShaderVulkan.h>

class ezGALRasterizerStateVulkan;
class ezGALBlendStateVulkan;
class ezGALDepthStencilStateVulkan;
class ezGALShaderVulkan;
class ezGALVertexDeclarationVulkan;
class ezRefCounted;

EZ_DEFINE_AS_POD_TYPE(vk::DynamicState);

/// Creates and caches persistent Vulkan resources. Resources are never freed until the device is shut down.
class EZ_RENDERERVULKAN_DLL ezResourceCacheVulkan
{
public:
  static void Initialize(ezGALDeviceVulkan* pDevice, vk::Device device);
  static void DeInitialize();

  static vk::PipelineCache GetPipelineCache() { return s_PipelineCache; }
  static vk::RenderPass RequestRenderPass(const ezGALRenderPassDescriptor& renderPass);
  static vk::Framebuffer RequestFrameBuffer(vk::RenderPass vkRenderPass, const ezGALFrameBufferDescriptor& frameBuffer);

  /// Destroys every cached framebuffer that uses the given view. Must be called once a render target view's image view is destroyed, as framebuffers are keyed on render target view handles which get recycled.
  static void RenderTargetViewDestroyed(vk::ImageView imageView);

private:
  struct FramebufferKey
  {
    vk::RenderPass m_renderPass;
    ezGALFrameBufferDescriptor m_frameBuffer;

    bool operator<(const FramebufferKey& rhs) const
    {
      if (m_renderPass != rhs.m_renderPass)
        return m_renderPass < rhs.m_renderPass;

      return m_frameBuffer < rhs.m_frameBuffer;
    }

    bool operator==(const FramebufferKey& rhs) const
    {
      return m_renderPass == rhs.m_renderPass && m_frameBuffer == rhs.m_frameBuffer;
    }
  };

  static void OnFrameBufferInvalidated(FramebufferKey key);

  using FrameBufferTracker = ezDependencyTracker<FramebufferKey, vk::ImageView>;

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
  static vk::Device s_Device;
  static vk::PipelineCache s_PipelineCache;
  // We have a N to 1 mapping for ezGALRenderingSetup to vk::RenderPass as multiple ezGALRenderingSetup can share the same RenderPassDesc.
  static ezHashTable<ezGALRenderPassDescriptor, vk::RenderPass, ResourceCacheHash> s_RenderPasses;
  static ezHashTable<FramebufferKey, vk::Framebuffer, ResourceCacheHash> s_FrameBuffers;
  // Maps each framebuffer to the image views it was built from, so a destroyed render target view can be resolved to the affected framebuffers without scanning the cache.
  // Heap allocated so its memory is released in DeInitialize instead of being reported as a leak.
  static ezUniquePtr<FrameBufferTracker> s_pFrameBufferTracker;
};
