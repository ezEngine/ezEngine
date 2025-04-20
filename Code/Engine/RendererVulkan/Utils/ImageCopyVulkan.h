#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

#include <RendererFoundation/Shader/ShaderUtils.h>
#include <RendererVulkan/Cache/ResourceCacheVulkan.h>

#include <vulkan/vulkan.hpp>

class ezGALBufferVulkan;
class ezGALTextureVulkan;
class ezGALRenderTargetViewVulkan;
class ezGALTextureResourceViewVulkan;
class ezGALBufferResourceViewVulkan;
class ezGALTextureUnorderedAccessViewVulkan;
class ezGALBufferUnorderedAccessViewVulkan;


/// \brief
class EZ_RENDERERVULKAN_DLL ezImageCopyVulkan
{
public:
  ezImageCopyVulkan(ezGALDeviceVulkan& GALDeviceVulkan);
  ~ezImageCopyVulkan();
  void Init(const ezGALTextureVulkan* pSource, const ezGALTextureVulkan* pTarget, ezShaderUtils::ezBuiltinShaderType type);

  void Copy(const ezVec3U32& sourceOffset, const vk::ImageSubresourceLayers& sourceLayers, const ezVec3U32& targetOffset, const vk::ImageSubresourceLayers& targetLayers, const ezVec3U32& extends);

  static void Initialize(ezGALDeviceVulkan& GALDeviceVulkan);
  static void DeInitialize(ezGALDeviceVulkan& GALDeviceVulkan);

  struct FramebufferCacheKey
  {
    EZ_DECLARE_POD_TYPE();

    vk::RenderPass m_renderPass;
    vk::ImageView m_targetView;
    ezVec3U32 m_extends;
    uint32_t m_layerCount;
  };

  struct ImageViewCacheKey
  {
    EZ_DECLARE_POD_TYPE();

    vk::Image m_image;
    vk::ImageSubresourceLayers m_subresourceLayers;
  };

  struct ImageViewCacheValue
  {
    EZ_DECLARE_POD_TYPE();

    vk::ImageSubresourceLayers m_subresourceLayers;
    vk::ImageView m_imageView;
  };

private:
  void RenderInternal(const ezVec3U32& sourceOffset, const vk::ImageSubresourceLayers& sourceLayers, const ezVec3U32& targetOffset, const vk::ImageSubresourceLayers& targetLayers, const ezVec3U32& extends);

  static void OnBeforeImageDestroyed(ezGALDeviceVulkan::OnBeforeImageDestroyedData data);


private:
  ezGALDeviceVulkan& m_GALDeviceVulkan;

  // Init input
  const ezGALTextureVulkan* m_pSource = nullptr;
  const ezGALTextureVulkan* m_pTarget = nullptr;
  ezShaderUtils::ezBuiltinShaderType m_type = ezShaderUtils::ezBuiltinShaderType::CopyImage;

  // Init derived Vulkan objects
  vk::RenderPass m_renderPass;
  ezShaderUtils::ezBuiltinShader m_shader;
  ezGALVertexDeclarationHandle m_hVertexDecl;
  ezResourceCacheVulkan::PipelineLayoutDesc m_LayoutDesc;
  ezResourceCacheVulkan::GraphicsPipelineDesc m_PipelineDesc;
  vk::Pipeline m_pipeline;

  // Cache to keep important resources alive
  // This avoids recreating them every frame
  struct Cache
  {
    Cache(ezAllocator* pAllocator);
    ~Cache();

    ezHashTable<ezGALShaderHandle, ezGALVertexDeclarationHandle> m_vertexDeclarations;
    ezHashTable<ImageViewCacheKey, vk::ImageView> m_sourceImageViews;
    ezHashTable<vk::Image, ImageViewCacheValue> m_imageToSourceImageViewCacheKey;
    ezHashTable<ImageViewCacheKey, vk::ImageView> m_targetImageViews;
    ezHashTable<vk::Image, ImageViewCacheValue> m_imageToTargetImageViewCacheKey;
    ezHashTable<FramebufferCacheKey, vk::Framebuffer> m_framebuffers;
    ezHashTable<ezShaderUtils::ezBuiltinShaderType, ezShaderUtils::ezBuiltinShader> m_shaders;

    ezEventSubscriptionID m_onBeforeImageDeletedSubscription;
  };

  static ezUniquePtr<Cache> s_cache;
};
