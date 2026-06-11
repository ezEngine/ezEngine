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
  m_BlendState.pAttachments = m_blendAttachmentState;
}

ezGALBlendStateVulkan::~ezGALBlendStateVulkan() = default;

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

ezGALDepthStencilStateVulkan::~ezGALDepthStencilStateVulkan() = default;

ezResult ezGALDepthStencilStateVulkan::InitPlatform(ezGALDevice* pDevice)
{
  m_DepthStencilState.depthBoundsTestEnable = VK_FALSE;
  m_DepthStencilState.depthCompareOp = GALCompareFuncToVulkan[m_Description.m_DepthTestFunc];
  m_DepthStencilState.depthTestEnable = m_Description.m_bDepthEnable ? VK_TRUE : VK_FALSE;
  m_DepthStencilState.depthWriteEnable = m_Description.m_bDepthWrite ? VK_TRUE : VK_FALSE;
  m_DepthStencilState.minDepthBounds = 0.f;
  m_DepthStencilState.maxDepthBounds = 1.f;

  m_DepthStencilState.stencilTestEnable = m_Description.m_bStencilEnable ? VK_TRUE : VK_FALSE;
  m_DepthStencilState.front.compareMask = m_Description.m_uiStencilReadMask;
  m_DepthStencilState.front.writeMask = m_Description.m_uiStencilWriteMask;
  m_DepthStencilState.front.compareOp = GALCompareFuncToVulkan[m_Description.m_FrontFaceStencilOp.m_StencilFunc];
  m_DepthStencilState.front.depthFailOp = GALStencilOpTableIndexToVulkan[m_Description.m_FrontFaceStencilOp.m_DepthFailOp];
  m_DepthStencilState.front.failOp = GALStencilOpTableIndexToVulkan[m_Description.m_FrontFaceStencilOp.m_FailOp];
  m_DepthStencilState.front.passOp = GALStencilOpTableIndexToVulkan[m_Description.m_FrontFaceStencilOp.m_PassOp];

  const ezGALStencilOpDescription& backFaceStencilOp = m_Description.m_BackFaceStencilOp;
  m_DepthStencilState.back.compareMask = m_Description.m_uiStencilReadMask;
  m_DepthStencilState.back.writeMask = m_Description.m_uiStencilWriteMask;
  m_DepthStencilState.back.compareOp = GALCompareFuncToVulkan[backFaceStencilOp.m_StencilFunc];
  m_DepthStencilState.back.depthFailOp = GALStencilOpTableIndexToVulkan[backFaceStencilOp.m_DepthFailOp];
  m_DepthStencilState.back.failOp = GALStencilOpTableIndexToVulkan[backFaceStencilOp.m_FailOp];
  m_DepthStencilState.back.passOp = GALStencilOpTableIndexToVulkan[backFaceStencilOp.m_PassOp];

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

ezGALRasterizerStateVulkan::~ezGALRasterizerStateVulkan() = default;



ezResult ezGALRasterizerStateVulkan::InitPlatform(ezGALDevice* pDevice)
{
  // TODO conservative raster extension
  // TODO scissor test is always enabled for vulkan
  // const bool NeedsStateDesc2 = m_Description.m_bConservativeRasterization;

  m_RasterizerState.cullMode = GALCullModeToVulkan[m_Description.m_CullMode];
  m_RasterizerState.depthBiasClamp = m_Description.m_fDepthBiasClamp;
  m_RasterizerState.depthBiasConstantFactor = static_cast<float>(m_Description.m_iDepthBias); // TODO does this have the intended effect?
  m_RasterizerState.depthBiasSlopeFactor = m_Description.m_fSlopeScaledDepthBias;
  m_RasterizerState.depthClampEnable = m_Description.m_fDepthBiasClamp > 0.f;
  m_RasterizerState.frontFace = m_Description.m_bFrontCounterClockwise ? vk::FrontFace::eCounterClockwise : vk::FrontFace::eClockwise;
  m_RasterizerState.lineWidth = 1.f;
  m_RasterizerState.polygonMode = m_Description.m_bWireFrame ? vk::PolygonMode::eLine : vk::PolygonMode::eFill;

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

ezGALSamplerStateVulkan::~ezGALSamplerStateVulkan() = default;

ezResult ezGALSamplerStateVulkan::InitPlatform(ezGALDevice* pDevice)
{
  ezGALSamplerStateCreationDescription desc = this->GetDescription();
  pDevice->AdjustSamplerStateDescription(desc);

  auto pVulkanDevice = static_cast<ezGALDeviceVulkan*>(pDevice);

  vk::SamplerCreateInfo samplerCreateInfo = {};
  samplerCreateInfo.addressModeU = GALTextureAddressModeToVulkan[desc.m_AddressU];
  samplerCreateInfo.addressModeV = GALTextureAddressModeToVulkan[desc.m_AddressV];
  samplerCreateInfo.addressModeW = GALTextureAddressModeToVulkan[desc.m_AddressW];
  if (desc.m_MagFilter == ezGALTextureFilterMode::Anisotropic || desc.m_MinFilter == ezGALTextureFilterMode::Anisotropic || desc.m_MipFilter == ezGALTextureFilterMode::Anisotropic)
  {
    samplerCreateInfo.anisotropyEnable = VK_TRUE;
  }

  vk::SamplerCustomBorderColorCreateInfoEXT customBorderColor;
  if (samplerCreateInfo.addressModeU == vk::SamplerAddressMode::eClampToBorder || samplerCreateInfo.addressModeV == vk::SamplerAddressMode::eClampToBorder || samplerCreateInfo.addressModeW == vk::SamplerAddressMode::eClampToBorder)
  {
    const ezColor col = desc.m_BorderColor;
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
      const bool bTransparent = desc.m_BorderColor.a == 0.0f;
      const bool bBlack = desc.m_BorderColor.r == 0.0f;
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
  samplerCreateInfo.compareEnable = desc.m_SampleCompareFunc == ezGALCompareFunc::Never ? VK_FALSE : VK_TRUE;
  samplerCreateInfo.compareOp = GALCompareFuncToVulkan[desc.m_SampleCompareFunc];
  samplerCreateInfo.magFilter = GALFilterToVulkanFilter[desc.m_MagFilter];
  samplerCreateInfo.minFilter = GALFilterToVulkanFilter[desc.m_MinFilter];
  samplerCreateInfo.maxAnisotropy = static_cast<float>(desc.m_uiMaxAnisotropy);
  samplerCreateInfo.maxLod = desc.m_fMaxMip;
  samplerCreateInfo.minLod = desc.m_fMinMip;
  samplerCreateInfo.mipLodBias = desc.m_fMipLodBias;
  samplerCreateInfo.mipmapMode = GALFilterToVulkanMipmapMode[desc.m_MipFilter];

  m_ResourceImageInfo.imageLayout = vk::ImageLayout::eUndefined;
  VK_SUCCEED_OR_RETURN_EZ_FAILURE(pVulkanDevice->GetVulkanDevice().createSampler(&samplerCreateInfo, nullptr, &m_ResourceImageInfo.sampler));
  return EZ_SUCCESS;
}


ezResult ezGALSamplerStateVulkan::DeInitPlatform(ezGALDevice* pDevice)
{
  ezGALDeviceVulkan* pVulkanDevice = static_cast<ezGALDeviceVulkan*>(pDevice);
  pVulkanDevice->DeleteLater(m_ResourceImageInfo.sampler);
  return EZ_SUCCESS;
}
