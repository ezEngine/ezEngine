#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/RendererVulkanDLL.h>
#include <RendererVulkan/State/StateVulkan.h>

// Mapping tables to map ezGAL constants to Vulkan constants
#include <RendererVulkan/State/Implementation/StateVulkan_MappingTables.inl>

// Blend state

ezGALBlendStateVulkan::ezGALBlendStateVulkan(const ezGALBlendStateCreationDescription& Description)
  : ezGALBlendState(Description)
{
  m_blendState.pAttachments = m_blendAttachmentState;
}

ezGALBlendStateVulkan::~ezGALBlendStateVulkan() {}

static vk::BlendOp ToVulkanBlendOp(ezGALBlendOp::Enum e)
{
  switch (e)
  {
    case ezGALBlendOp::Add:
      return vk::BlendOp::eAdd;
    case ezGALBlendOp::Max:
      return vk::BlendOp::eMax;
    case ezGALBlendOp::Min:
      return vk::BlendOp::eMin;
    case ezGALBlendOp::RevSubtract:
      return vk::BlendOp::eReverseSubtract;
    case ezGALBlendOp::Subtract:
      return vk::BlendOp::eSubtract;
    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
  }

  return vk::BlendOp::eAdd;
}

static vk::BlendFactor ToVulkanBlendFactor(ezGALBlend::Enum e)
{
  switch (e)
  {
    case ezGALBlend::BlendFactor:
      EZ_ASSERT_NOT_IMPLEMENTED;
      return vk::BlendFactor::eZero;
    case ezGALBlend::DestAlpha:
      return vk::BlendFactor::eDstAlpha;
    case ezGALBlend::DestColor:
      return vk::BlendFactor::eDstColor;
    case ezGALBlend::InvBlendFactor:
      EZ_ASSERT_NOT_IMPLEMENTED;
      return vk::BlendFactor::eZero;
    case ezGALBlend::InvDestAlpha:
      return vk::BlendFactor::eOneMinusDstAlpha;
    case ezGALBlend::InvDestColor:
      return vk::BlendFactor::eOneMinusDstColor;
    case ezGALBlend::InvSrcAlpha:
      return vk::BlendFactor::eOneMinusSrcAlpha;
    case ezGALBlend::InvSrcColor:
      return vk::BlendFactor::eOneMinusSrcColor;
    case ezGALBlend::One:
      return vk::BlendFactor::eOne;
    case ezGALBlend::SrcAlpha:
      return vk::BlendFactor::eSrcAlpha;
    case ezGALBlend::SrcAlphaSaturated:
      return vk::BlendFactor::eSrcAlphaSaturate;
    case ezGALBlend::SrcColor:
      return vk::BlendFactor::eSrcColor;
    case ezGALBlend::Zero:
      return vk::BlendFactor::eZero;

    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
  }

  return vk::BlendFactor::eOne;
}

ezResult ezGALBlendStateVulkan::InitPlatform(ezGALDevice* pDevice)
{
  // TODO attachment count has to be set when render targets are known
  // TODO alpha2coverage needs to be implemented in MultisampleStateCreateInfo
  // TODO independent blend is a device feature that is always enabled if present

  for (ezInt32 i = 0; i < 8; ++i)
  {
    m_blendAttachmentState[i].blendEnable = m_Description.m_RenderTargetBlendDescriptions[i].m_bBlendingEnabled ? VK_TRUE : VK_FALSE;
    m_blendAttachmentState[i].colorBlendOp = ToVulkanBlendOp(m_Description.m_RenderTargetBlendDescriptions[i].m_BlendOp);
    m_blendAttachmentState[i].alphaBlendOp = ToVulkanBlendOp(m_Description.m_RenderTargetBlendDescriptions[i].m_BlendOpAlpha);
    m_blendAttachmentState[i].dstColorBlendFactor = ToVulkanBlendFactor(m_Description.m_RenderTargetBlendDescriptions[i].m_DestBlend);
    m_blendAttachmentState[i].dstAlphaBlendFactor = ToVulkanBlendFactor(m_Description.m_RenderTargetBlendDescriptions[i].m_DestBlendAlpha);
    m_blendAttachmentState[i].srcColorBlendFactor = ToVulkanBlendFactor(m_Description.m_RenderTargetBlendDescriptions[i].m_SourceBlend);
    m_blendAttachmentState[i].srcAlphaBlendFactor = ToVulkanBlendFactor(m_Description.m_RenderTargetBlendDescriptions[i].m_SourceBlendAlpha);
    m_blendAttachmentState[i].colorWriteMask = (vk::ColorComponentFlags)(m_Description.m_RenderTargetBlendDescriptions[i].m_uiWriteMask & 0x0F);
  }

  return EZ_SUCCESS;
}

ezResult ezGALBlendStateVulkan::DeInitPlatform(ezGALDevice* pDevice)
{
  return EZ_SUCCESS;
}

// Depth Stencil state

ezGALDepthStencilStateVulkan::ezGALDepthStencilStateVulkan(const ezGALDepthStencilStateCreationDescription& Description)
  : ezGALDepthStencilState(Description)
{
}

ezGALDepthStencilStateVulkan::~ezGALDepthStencilStateVulkan() {}

ezResult ezGALDepthStencilStateVulkan::InitPlatform(ezGALDevice* pDevice)
{
  m_depthStencilState.depthBoundsTestEnable = VK_FALSE;
  m_depthStencilState.depthCompareOp = GALCompareFuncToVulkan[m_Description.m_DepthTestFunc];
  m_depthStencilState.depthTestEnable = m_Description.m_bDepthTest ? VK_TRUE : VK_FALSE;
  m_depthStencilState.depthWriteEnable = m_Description.m_bDepthWrite ? VK_TRUE : VK_FALSE;
  m_depthStencilState.minDepthBounds = 0.f;
  m_depthStencilState.maxDepthBounds = 1.f;

  m_depthStencilState.stencilTestEnable = m_Description.m_bStencilTest ? VK_TRUE : VK_FALSE;
  m_depthStencilState.front.compareMask = m_Description.m_uiStencilReadMask;
  m_depthStencilState.front.writeMask = m_Description.m_uiStencilWriteMask;
  m_depthStencilState.front.compareOp = GALCompareFuncToVulkan[m_Description.m_FrontFaceStencilOp.m_StencilFunc];
  m_depthStencilState.front.depthFailOp = GALStencilOpTableIndexToVulkan[m_Description.m_FrontFaceStencilOp.m_DepthFailOp];
  m_depthStencilState.front.failOp = GALStencilOpTableIndexToVulkan[m_Description.m_FrontFaceStencilOp.m_FailOp];
  m_depthStencilState.front.passOp = GALStencilOpTableIndexToVulkan[m_Description.m_FrontFaceStencilOp.m_PassOp];

  const ezGALStencilOpDescription& backFaceStencilOp =
    m_Description.m_bSeparateFrontAndBack ? m_Description.m_BackFaceStencilOp : m_Description.m_FrontFaceStencilOp;
  m_depthStencilState.back.compareOp = GALCompareFuncToVulkan[backFaceStencilOp.m_StencilFunc];
  m_depthStencilState.back.depthFailOp = GALStencilOpTableIndexToVulkan[backFaceStencilOp.m_DepthFailOp];
  m_depthStencilState.back.failOp = GALStencilOpTableIndexToVulkan[backFaceStencilOp.m_FailOp];
  m_depthStencilState.back.passOp = GALStencilOpTableIndexToVulkan[backFaceStencilOp.m_PassOp];

  return EZ_SUCCESS;
}

ezResult ezGALDepthStencilStateVulkan::DeInitPlatform(ezGALDevice* pDevice)
{
  return EZ_SUCCESS;
}


// Rasterizer state

ezGALRasterizerStateVulkan::ezGALRasterizerStateVulkan(const ezGALRasterizerStateCreationDescription& Description)
  : ezGALRasterizerState(Description)
{
}

ezGALRasterizerStateVulkan::~ezGALRasterizerStateVulkan() {}



ezResult ezGALRasterizerStateVulkan::InitPlatform(ezGALDevice* pDevice)
{
  // TODO conservative raster extension
  // TODO scissor test is always enabled for vulkan
  // const bool NeedsStateDesc2 = m_Description.m_bConservativeRasterization;

  m_rasterizerState.cullMode = GALCullModeToVulkan[m_Description.m_CullMode];
  m_rasterizerState.depthBiasClamp = m_Description.m_fDepthBiasClamp;
  m_rasterizerState.depthBiasConstantFactor = static_cast<float>(m_Description.m_iDepthBias); // TODO does this have the intended effect?
  m_rasterizerState.depthBiasSlopeFactor = m_Description.m_fSlopeScaledDepthBias;
  m_rasterizerState.depthClampEnable = m_Description.m_fDepthBiasClamp > 0.f;
  m_rasterizerState.frontFace = m_Description.m_bFrontCounterClockwise ? vk::FrontFace::eCounterClockwise : vk::FrontFace::eClockwise;
  m_rasterizerState.lineWidth = 1.f;
  m_rasterizerState.polygonMode = m_Description.m_bWireFrame ? vk::PolygonMode::eLine : vk::PolygonMode::eFill;

  return EZ_SUCCESS;
}


ezResult ezGALRasterizerStateVulkan::DeInitPlatform(ezGALDevice* pDevice)
{
  return EZ_SUCCESS;
}

// Sampler state

ezGALSamplerStateVulkan::ezGALSamplerStateVulkan(const ezGALSamplerStateCreationDescription& Description)
  : ezGALSamplerState(Description)
{
}

ezGALSamplerStateVulkan::~ezGALSamplerStateVulkan() {}

ezResult ezGALSamplerStateVulkan::InitPlatform(ezGALDevice* pDevice)
{
  auto pVulkanDevice = static_cast<ezGALDeviceVulkan*>(pDevice);

  vk::SamplerCreateInfo samplerCreateInfo = {};
  samplerCreateInfo.addressModeU = GALTextureAddressModeToVulkan[m_Description.m_AddressU];
  samplerCreateInfo.addressModeV = GALTextureAddressModeToVulkan[m_Description.m_AddressV];
  samplerCreateInfo.addressModeW = GALTextureAddressModeToVulkan[m_Description.m_AddressW];
  if (m_Description.m_MagFilter == ezGALTextureFilterMode::Anisotropic || m_Description.m_MinFilter == ezGALTextureFilterMode::Anisotropic || m_Description.m_MipFilter == ezGALTextureFilterMode::Anisotropic)
  {
    samplerCreateInfo.anisotropyEnable = VK_TRUE;
  }

  vk::SamplerCustomBorderColorCreateInfoEXT customBorderColor;
  if (samplerCreateInfo.addressModeU == vk::SamplerAddressMode::eClampToBorder || samplerCreateInfo.addressModeV == vk::SamplerAddressMode::eClampToBorder || samplerCreateInfo.addressModeW == vk::SamplerAddressMode::eClampToBorder)
  {
    const ezColor col = m_Description.m_BorderColor;
    if (col == ezColor(0, 0, 0, 0))
    {
      samplerCreateInfo.borderColor = vk::BorderColor::eFloatTransparentBlack;
    }
    else if (col == ezColor(0, 0, 0, 1))
    {
      samplerCreateInfo.borderColor = vk::BorderColor::eFloatOpaqueBlack;
    }
    else if (col == ezColor(1, 1, 1, 1))
    {
      samplerCreateInfo.borderColor = vk::BorderColor::eFloatOpaqueWhite;
    }
    else if (pVulkanDevice->GetExtensions().m_bBorderColorFloat)
    {
      customBorderColor.customBorderColor.setFloat32({col.r, col.g, col.b, col.a});
      samplerCreateInfo.borderColor = vk::BorderColor::eFloatCustomEXT;
      samplerCreateInfo.pNext = &customBorderColor;
    }
    else
    {
      // Fallback to close enough.
      const bool bTransparent = m_Description.m_BorderColor.a == 0.0f;
      const bool bBlack = m_Description.m_BorderColor.r == 0.0f;
      if (bBlack)
      {
        samplerCreateInfo.borderColor = bTransparent ? vk::BorderColor::eFloatTransparentBlack : vk::BorderColor::eFloatOpaqueBlack;
      }
      else
      {
        samplerCreateInfo.borderColor = vk::BorderColor::eFloatOpaqueWhite;
      }
    }
  }
  samplerCreateInfo.compareEnable = m_Description.m_SampleCompareFunc == ezGALCompareFunc::Never ? VK_FALSE : VK_TRUE;
  samplerCreateInfo.compareOp = GALCompareFuncToVulkan[m_Description.m_SampleCompareFunc];
  samplerCreateInfo.magFilter = GALFilterToVulkanFilter[m_Description.m_MagFilter];
  samplerCreateInfo.minFilter = GALFilterToVulkanFilter[m_Description.m_MinFilter];
  samplerCreateInfo.maxAnisotropy = static_cast<float>(m_Description.m_uiMaxAnisotropy);
  samplerCreateInfo.maxLod = m_Description.m_fMaxMip;
  samplerCreateInfo.minLod = m_Description.m_fMinMip;
  samplerCreateInfo.mipLodBias = m_Description.m_fMipLodBias;
  samplerCreateInfo.mipmapMode = GALFilterToVulkanMipmapMode[m_Description.m_MipFilter];

  m_resourceImageInfo.imageLayout = vk::ImageLayout::eUndefined;
  VK_SUCCEED_OR_RETURN_EZ_FAILURE(pVulkanDevice->GetVulkanDevice().createSampler(&samplerCreateInfo, nullptr, &m_resourceImageInfo.sampler));
  return EZ_SUCCESS;
}


ezResult ezGALSamplerStateVulkan::DeInitPlatform(ezGALDevice* pDevice)
{
  ezGALDeviceVulkan* pVulkanDevice = static_cast<ezGALDeviceVulkan*>(pDevice);
  pVulkanDevice->DeleteLater(m_resourceImageInfo.sampler);
  return EZ_SUCCESS;
}
