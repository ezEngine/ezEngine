#include <RendererWebGPU/RendererWebGPUPCH.h>

#include <RendererWebGPU/Device/DeviceWebGPU.h>
#include <RendererWebGPU/RendererWebGPUDLL.h>
#include <RendererWebGPU/State/StateWebGPU.h>

// Mapping tables to map ezGAL constants to WebGPU constants
#include <RendererWebGPU/State/Implementation/StateWebGPU_MappingTables.inl>

// Blend state

static wgpu::CompareFunction ToWGPU(bool bEnable, ezGALCompareFunc::Enum mode)
{
  if (bEnable)
  {
    switch (mode)
    {
      // TODO WebGPU: need to know when to enable this
      // case ezGALCompareFunc::None:
      //   return wgpu::CompareFunction::Undefined;
      case ezGALCompareFunc::Never:
        // return wgpu::CompareFunction::Never;
        return wgpu::CompareFunction::Undefined;
      case ezGALCompareFunc::Less:
        return wgpu::CompareFunction::Less;
      case ezGALCompareFunc::Equal:
        return wgpu::CompareFunction::Equal;
      case ezGALCompareFunc::LessEqual:
        return wgpu::CompareFunction::LessEqual;
      case ezGALCompareFunc::Greater:
        return wgpu::CompareFunction::Greater;
      case ezGALCompareFunc::NotEqual:
        return wgpu::CompareFunction::NotEqual;
      case ezGALCompareFunc::GreaterEqual:
        return wgpu::CompareFunction::GreaterEqual;
      case ezGALCompareFunc::Always:
        return wgpu::CompareFunction::Always;

        EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
    }
  }

  return wgpu::CompareFunction::Undefined;
}

wgpu::BlendOperation ToWGPU(ezGALBlendOp::Enum op)
{
  switch (op)
  {
    case ezGALBlendOp::Add:
      return wgpu::BlendOperation::Add;
    case ezGALBlendOp::Subtract:
      return wgpu::BlendOperation::Subtract;
    case ezGALBlendOp::RevSubtract:
      return wgpu::BlendOperation::ReverseSubtract;
    case ezGALBlendOp::Min:
      return wgpu::BlendOperation::Min;
    case ezGALBlendOp::Max:
      return wgpu::BlendOperation::Max;

      EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return wgpu::BlendOperation::Undefined;
}


wgpu::BlendFactor ToWGPU(ezGALBlend::Enum op)
{
  switch (op)
  {
    case ezGALBlend::Zero:
      return wgpu::BlendFactor::Zero;
    case ezGALBlend::One:
      return wgpu::BlendFactor::One;
    case ezGALBlend::SrcColor:
      return wgpu::BlendFactor::Src;
    case ezGALBlend::InvSrcColor:
      return wgpu::BlendFactor::OneMinusSrc;
    case ezGALBlend::SrcAlpha:
      return wgpu::BlendFactor::SrcAlpha;
    case ezGALBlend::InvSrcAlpha:
      return wgpu::BlendFactor::OneMinusSrcAlpha;
    case ezGALBlend::DestAlpha:
      return wgpu::BlendFactor::DstAlpha;
    case ezGALBlend::InvDestAlpha:
      return wgpu::BlendFactor::OneMinusDst;
    case ezGALBlend::DestColor:
      return wgpu::BlendFactor::Dst;
    case ezGALBlend::InvDestColor:
      return wgpu::BlendFactor::OneMinusDst;
    case ezGALBlend::SrcAlphaSaturated:
      return wgpu::BlendFactor::SrcAlphaSaturated;
    case ezGALBlend::BlendFactor:
      return wgpu::BlendFactor::Constant;
    case ezGALBlend::InvBlendFactor:
      return wgpu::BlendFactor::OneMinusConstant;

      EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return wgpu::BlendFactor::Undefined;
}

ezGALBlendStateWebGPU::ezGALBlendStateWebGPU(const ezGALBlendStateCreationDescription& Description)
  : ezGALBlendState(Description)
{
}

ezGALBlendStateWebGPU::~ezGALBlendStateWebGPU() = default;

ezResult ezGALBlendStateWebGPU::InitPlatform(ezGALDevice* pDevice)
{
  // EZ_WEBGPU_TRACE();

  for (ezUInt32 i = 0; i < EZ_GAL_MAX_RENDERTARGET_COUNT; ++i)
  {
    if (m_Description.m_RenderTargetBlendDescriptions->m_bBlendingEnabled)
    {
      m_States[i].color.operation = ToWGPU(m_Description.m_RenderTargetBlendDescriptions[i].m_BlendOp);
      m_States[i].color.srcFactor = ToWGPU(m_Description.m_RenderTargetBlendDescriptions[i].m_SourceBlend);
      m_States[i].color.dstFactor = ToWGPU(m_Description.m_RenderTargetBlendDescriptions[i].m_DestBlend);

      m_States[i].alpha.operation = ToWGPU(m_Description.m_RenderTargetBlendDescriptions[i].m_BlendOpAlpha);
      m_States[i].alpha.srcFactor = ToWGPU(m_Description.m_RenderTargetBlendDescriptions[i].m_SourceBlendAlpha);
      m_States[i].alpha.dstFactor = ToWGPU(m_Description.m_RenderTargetBlendDescriptions[i].m_DestBlendAlpha);

      // TODO WebGPU: use m_Description.m_RenderTargetBlendDescriptions[i].m_uiWriteMask
    }
    else
    {
      m_States[i].color.operation = wgpu::BlendOperation::Undefined;
      m_States[i].alpha.operation = wgpu::BlendOperation::Undefined;
    }
  }

  return EZ_SUCCESS;
}

ezResult ezGALBlendStateWebGPU::DeInitPlatform(ezGALDevice* pDevice)
{
  return EZ_SUCCESS;
}

// Depth Stencil state

ezGALDepthStencilStateWebGPU::ezGALDepthStencilStateWebGPU(const ezGALDepthStencilStateCreationDescription& Description)
  : ezGALDepthStencilState(Description)
{
}

ezGALDepthStencilStateWebGPU::~ezGALDepthStencilStateWebGPU() = default;

ezResult ezGALDepthStencilStateWebGPU::InitPlatform(ezGALDevice* pDevice)
{
  EZ_WEBGPU_TRACE();

  const auto& d = m_Description;

  // d.m_bDepthTest
  m_State.depthWriteEnabled = d.m_bDepthWrite;
  m_State.depthCompare = ToWGPU(d.m_bDepthTest, d.m_DepthTestFunc);

  return EZ_SUCCESS;
}

ezResult ezGALDepthStencilStateWebGPU::DeInitPlatform(ezGALDevice* pDevice)
{
  // EZ_WEBGPU_TRACE();
  return EZ_SUCCESS;
}

// Rasterizer state

ezGALRasterizerStateWebGPU::ezGALRasterizerStateWebGPU(const ezGALRasterizerStateCreationDescription& Description)
  : ezGALRasterizerState(Description)
{
}

ezGALRasterizerStateWebGPU::~ezGALRasterizerStateWebGPU() = default;

ezResult ezGALRasterizerStateWebGPU::InitPlatform(ezGALDevice* pDevice)
{
  // EZ_WEBGPU_TRACE();
  // nothing to do it seems
  return EZ_SUCCESS;
}

ezResult ezGALRasterizerStateWebGPU::DeInitPlatform(ezGALDevice* pDevice)
{
  // EZ_WEBGPU_TRACE();
  return EZ_SUCCESS;
}

// Sampler state

ezGALSamplerStateWebGPU::ezGALSamplerStateWebGPU(const ezGALSamplerStateCreationDescription& Description)
  : ezGALSamplerState(Description)
{
}

ezGALSamplerStateWebGPU::~ezGALSamplerStateWebGPU() = default;

static wgpu::AddressMode ToWGPU(ezImageAddressMode::Enum mode)
{
  switch (mode)
  {
    case ezImageAddressMode::Repeat:
      return wgpu::AddressMode::Repeat;
    case ezImageAddressMode::Clamp:
    case ezImageAddressMode::ClampBorder:
      return wgpu::AddressMode::ClampToEdge;
    case ezImageAddressMode::Mirror:
      return wgpu::AddressMode::MirrorRepeat;

      EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return wgpu::AddressMode::Repeat;
}

static wgpu::FilterMode ToWGPU(ezGALTextureFilterMode::Enum mode)
{
  switch (mode)
  {
    case ezGALTextureFilterMode::Point:
      return wgpu::FilterMode::Nearest;
    case ezGALTextureFilterMode::Linear:
    case ezGALTextureFilterMode::Anisotropic:
      return wgpu::FilterMode::Linear;

      EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return wgpu::FilterMode::Linear;
}

ezResult ezGALSamplerStateWebGPU::InitPlatform(ezGALDevice* pDevice0)
{
  ezGALDeviceWebGPU* pDevice = (ezGALDeviceWebGPU*)pDevice0;

  wgpu::SamplerDescriptor desc;
  desc.addressModeU = ToWGPU(m_Description.m_AddressU);
  desc.addressModeV = ToWGPU(m_Description.m_AddressV);
  desc.addressModeW = ToWGPU(m_Description.m_AddressW);
  desc.magFilter = ToWGPU(m_Description.m_MagFilter);
  desc.minFilter = ToWGPU(m_Description.m_MinFilter);
  desc.mipmapFilter = m_Description.m_MipFilter == ezGALTextureFilterMode::Point ? wgpu::MipmapFilterMode::Nearest : wgpu::MipmapFilterMode::Linear;
  desc.lodMinClamp = ezMath::Max(0.0f, m_Description.m_fMinMip);
  desc.lodMaxClamp = ezMath::Min(32.0f, m_Description.m_fMaxMip);
  desc.compare = ToWGPU(false, m_Description.m_SampleCompareFunc); // TODO WebGPU: when to activate compare mode ?
  desc.maxAnisotropy = 1;

  if (m_Description.m_MipFilter != ezGALTextureFilterMode::Point && m_Description.m_MinFilter == ezGALTextureFilterMode::Anisotropic && m_Description.m_MagFilter == ezGALTextureFilterMode::Anisotropic)
  {
    desc.maxAnisotropy = (uint16_t)m_Description.m_uiMaxAnisotropy;
  }

  m_Sampler = pDevice->GetDevice().CreateSampler(&desc);

  return EZ_SUCCESS;
}

ezResult ezGALSamplerStateWebGPU::DeInitPlatform(ezGALDevice* pDevice)
{
  m_Sampler = nullptr;
  return EZ_SUCCESS;
}
