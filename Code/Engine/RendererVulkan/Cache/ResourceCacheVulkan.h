#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

#include <Foundation/Algorithm/HashStream.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>
#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Resources/RenderTargetViewVulkan.h>

#include <vulkan/vulkan.hpp>

/// \brief Creates and caches persistent Vulkan resources. Resources are never freed until the device ist shut down.
class EZ_RENDERERVULKAN_DLL ezResourceCacheVulkan
{
public:
  /// \brief Hashable version without pointers of vk::FramebufferCreateInfo
  struct FramebufferDesc
  {
    VkRenderPass renderPass;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t layers = 1;
    ezHybridArray<vk::ImageView, EZ_GAL_MAX_RENDERTARGET_COUNT + 1> attachments;
  };

  /// \brief Hashable version without pointers or redundant data of vk::AttachmentDescription
  struct AttachmentDesc
  {
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

  static void Initialize(ezGALDeviceVulkan* pDevice, vk::Device device);
  static void DeInitialize();

  static void GetRenderPassDesc(const ezGALRenderingSetup& renderingSetup, RenderPassDesc& out_desc);
  static vk::RenderPass RequestRenderPass(const RenderPassDesc& desc);
  static void GetFrameBufferDesc(vk::RenderPass renderPass, const ezGALRenderingSetup& renderingSetup, FramebufferDesc& out_desc);
  static vk::Framebuffer RequestFrameBuffer(const FramebufferDesc& desc);

private:
  static ezGALDeviceVulkan* s_pDevice;
  static vk::Device s_device;

  struct ezRenderPassHash
  {
    static ezUInt32 Hash(const RenderPassDesc& renderingSetup);
    static bool Equal(const RenderPassDesc& a, const RenderPassDesc& b);
  };

  struct ezFrameBufferHash
  {
    static ezUInt32 Hash(const FramebufferDesc& renderingSetup);
    static bool Equal(const FramebufferDesc& a, const FramebufferDesc& b);
  };

  static ezHashTable<RenderPassDesc, vk::RenderPass, ezRenderPassHash> s_renderPasses;
  static ezHashTable<FramebufferDesc, vk::Framebuffer, ezFrameBufferHash> s_frameBuffers;
};
