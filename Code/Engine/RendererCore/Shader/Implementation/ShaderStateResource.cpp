#include <RendererCore/PCH.h>
#include <RendererCore/Shader/ShaderStateResource.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezShaderStateResource, ezResourceBase, 1, ezRTTIDefaultAllocator<ezShaderStateResource>);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezShaderStateResource::ezShaderStateResource() : ezResource(ezResourceBase::DoUpdate::OnMainThread, 0)
{
}

ezResourceLoadDesc ezShaderStateResource::UnloadData(Unload WhatToUnload)
{
  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = GetLoadingState();

  if (WhatToUnload == Unload::AllQualityLevels)
  {
    res.m_State = ezResourceState::Unloaded;

    if (!m_hBlendState.IsInvalidated())
    {
      ezGALDevice::GetDefaultDevice()->DestroyBlendState(m_hBlendState);
      m_hBlendState.Invalidate();
    }

    if (!m_hDepthStencilState.IsInvalidated())
    {
      ezGALDevice::GetDefaultDevice()->DestroyDepthStencilState(m_hDepthStencilState);
      m_hDepthStencilState.Invalidate();
    }

    if (!m_hRasterizerState.IsInvalidated())
    {
      ezGALDevice::GetDefaultDevice()->DestroyRasterizerState(m_hRasterizerState);
      m_hRasterizerState.Invalidate();
    }
  }

  return res;
}

ezResourceLoadDesc ezShaderStateResource::UpdateContent(ezStreamReaderBase* stream)
{
  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::LoadedResourceMissing;

  if (stream == nullptr)
    return res;

  ezShaderStateResourceDescriptor desc;
  desc.Load(*stream);

  return CreateResource(desc);
}

void ezShaderStateResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(ezShaderStateResource);
  out_NewMemoryUsage.m_uiMemoryGPU = sizeof(ezShaderStateResourceDescriptor); // just a guess, better than zero
}

ezResourceLoadDesc ezShaderStateResource::CreateResource(const ezShaderStateResourceDescriptor& descriptor)
{
  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Loaded;

  auto pDev = ezGALDevice::GetDefaultDevice();

  m_hBlendState = pDev->CreateBlendState(descriptor.m_BlendDesc);
  m_hDepthStencilState = pDev->CreateDepthStencilState(descriptor.m_DepthStencilDesc);
  m_hRasterizerState = pDev->CreateRasterizerState(descriptor.m_RasterizerDesc);

  return res;
}

enum ezShaderStateVersion : ezUInt32
{
  Version0 = 0,
  Version1,


  ENUM_COUNT,
  Current = ENUM_COUNT - 1
};

void ezShaderStateResourceDescriptor::Save(ezStreamWriterBase& stream) const
{
  stream << (ezUInt32) ezShaderStateVersion::Current;

  // Blend State
  {
    stream << m_BlendDesc.m_bAlphaToCoverage;
    stream << m_BlendDesc.m_bIndependentBlend;

    const ezUInt8 iBlends = m_BlendDesc.m_bIndependentBlend ? EZ_GAL_MAX_RENDERTARGET_COUNT : 1;
    stream << iBlends; // in case EZ_GAL_MAX_RENDERTARGET_COUNT ever changes

    for (ezUInt32 b = 0; b < iBlends; ++b)
    {
      stream << m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_bBlendingEnabled;
      stream << (ezUInt8) m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_BlendOp;
      stream << (ezUInt8) m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_BlendOpAlpha;
      stream << (ezUInt8) m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_DestBlend;
      stream << (ezUInt8) m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_DestBlendAlpha;
      stream << (ezUInt8) m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_SourceBlend;
      stream << (ezUInt8) m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_SourceBlendAlpha;
      stream << m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_uiWriteMask;
    }
  }

  // Depth Stencil State
  {
    stream << (ezUInt8) m_DepthStencilDesc.m_DepthTestFunc;
    stream << m_DepthStencilDesc.m_bDepthTest;
    stream << m_DepthStencilDesc.m_bDepthWrite;
    stream << m_DepthStencilDesc.m_bSeparateFrontAndBack;
    stream << m_DepthStencilDesc.m_bStencilTest;
    stream << m_DepthStencilDesc.m_uiStencilReadMask;
    stream << m_DepthStencilDesc.m_uiStencilWriteMask;
    stream << (ezUInt8) m_DepthStencilDesc.m_FrontFaceStencilOp.m_DepthFailOp;
    stream << (ezUInt8) m_DepthStencilDesc.m_FrontFaceStencilOp.m_FailOp;
    stream << (ezUInt8) m_DepthStencilDesc.m_FrontFaceStencilOp.m_PassOp;
    stream << (ezUInt8) m_DepthStencilDesc.m_FrontFaceStencilOp.m_StencilFunc;
    stream << (ezUInt8) m_DepthStencilDesc.m_BackFaceStencilOp.m_DepthFailOp;
    stream << (ezUInt8) m_DepthStencilDesc.m_BackFaceStencilOp.m_FailOp;
    stream << (ezUInt8) m_DepthStencilDesc.m_BackFaceStencilOp.m_PassOp;
    stream << (ezUInt8) m_DepthStencilDesc.m_BackFaceStencilOp.m_StencilFunc;
  }

  // Rasterizer State
  {
    stream << m_RasterizerDesc.m_bDepthClip;
    stream << m_RasterizerDesc.m_bFrontCounterClockwise;
    stream << m_RasterizerDesc.m_bLineAA;
    stream << m_RasterizerDesc.m_bMSAA;
    stream << m_RasterizerDesc.m_bScissorTest;
    stream << m_RasterizerDesc.m_bWireFrame;
    stream << (ezUInt8) m_RasterizerDesc.m_CullMode;
    stream << m_RasterizerDesc.m_fDepthBiasClamp;
    stream << m_RasterizerDesc.m_fSlopeScaledDepthBias;
    stream << m_RasterizerDesc.m_iDepthBias;
  }
}

void ezShaderStateResourceDescriptor::Load(ezStreamReaderBase& stream)
{
  ezUInt32 uiVersion = 0;
  stream >> uiVersion;
  
  EZ_ASSERT_DEV(uiVersion >= ezShaderStateVersion::Version1 && uiVersion <= ezShaderStateVersion::Current, "Invalid version %u", uiVersion);

  // Blend State
  {
    stream >> m_BlendDesc.m_bAlphaToCoverage;
    stream >> m_BlendDesc.m_bIndependentBlend;

    ezUInt8 iBlends = 0;
    stream >> iBlends; // in case EZ_GAL_MAX_RENDERTARGET_COUNT ever changes

    for (ezUInt32 b = 0; b < iBlends; ++b)
    {
      ezUInt8 uiTemp;
      stream >> m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_bBlendingEnabled;
      stream >> uiTemp; m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_BlendOp = (ezGALBlendOp::Enum) uiTemp;
      stream >> uiTemp; m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_BlendOpAlpha = (ezGALBlendOp::Enum) uiTemp;
      stream >> uiTemp; m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_DestBlend = (ezGALBlend::Enum) uiTemp;
      stream >> uiTemp; m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_DestBlendAlpha = (ezGALBlend::Enum) uiTemp;
      stream >> uiTemp; m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_SourceBlend = (ezGALBlend::Enum) uiTemp;
      stream >> uiTemp; m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_SourceBlendAlpha = (ezGALBlend::Enum) uiTemp;
      stream >> m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_uiWriteMask;
    }
  }

  // Depth Stencil State
  {
    ezUInt8 uiTemp = 0;
    stream >> uiTemp; m_DepthStencilDesc.m_DepthTestFunc = (ezGALCompareFunc::Enum) uiTemp;
    stream >> m_DepthStencilDesc.m_bDepthTest;
    stream >> m_DepthStencilDesc.m_bDepthWrite;
    stream >> m_DepthStencilDesc.m_bSeparateFrontAndBack;
    stream >> m_DepthStencilDesc.m_bStencilTest;
    stream >> m_DepthStencilDesc.m_uiStencilReadMask;
    stream >> m_DepthStencilDesc.m_uiStencilWriteMask;
    stream >> uiTemp; m_DepthStencilDesc.m_FrontFaceStencilOp.m_DepthFailOp = (ezGALStencilOp::Enum) uiTemp;
    stream >> uiTemp; m_DepthStencilDesc.m_FrontFaceStencilOp.m_FailOp = (ezGALStencilOp::Enum) uiTemp;
    stream >> uiTemp; m_DepthStencilDesc.m_FrontFaceStencilOp.m_PassOp = (ezGALStencilOp::Enum) uiTemp;
    stream >> uiTemp; m_DepthStencilDesc.m_FrontFaceStencilOp.m_StencilFunc = (ezGALCompareFunc::Enum) uiTemp;
    stream >> uiTemp; m_DepthStencilDesc.m_BackFaceStencilOp.m_DepthFailOp = (ezGALStencilOp::Enum) uiTemp;
    stream >> uiTemp; m_DepthStencilDesc.m_BackFaceStencilOp.m_FailOp = (ezGALStencilOp::Enum) uiTemp;
    stream >> uiTemp; m_DepthStencilDesc.m_BackFaceStencilOp.m_PassOp = (ezGALStencilOp::Enum) uiTemp;
    stream >> uiTemp; m_DepthStencilDesc.m_BackFaceStencilOp.m_StencilFunc = (ezGALCompareFunc::Enum) uiTemp;
  }

  // Rasterizer State
  {
    ezUInt8 uiTemp = 0;
    stream >> m_RasterizerDesc.m_bDepthClip;
    stream >> m_RasterizerDesc.m_bFrontCounterClockwise;
    stream >> m_RasterizerDesc.m_bLineAA;
    stream >> m_RasterizerDesc.m_bMSAA;
    stream >> m_RasterizerDesc.m_bScissorTest;
    stream >> m_RasterizerDesc.m_bWireFrame;
    stream >> uiTemp; m_RasterizerDesc.m_CullMode = (ezGALCullMode::Enum) uiTemp;
    stream >> m_RasterizerDesc.m_fDepthBiasClamp;
    stream >> m_RasterizerDesc.m_fSlopeScaledDepthBias;
    stream >> m_RasterizerDesc.m_iDepthBias;
  }
}

ezUInt32 ezShaderStateResourceDescriptor::CalculateHash() const
{
  return m_BlendDesc.CalculateHash() + m_RasterizerDesc.CalculateHash() + m_DepthStencilDesc.CalculateHash();
}

ezResult ezShaderStateResourceDescriptor::Load(const char* szSource)
{
  ezStringBuilder sSource = szSource;

  /// \todo Requires some fine tuning
  if (sSource.FindSubString_NoCase("wireframe"))
    m_RasterizerDesc.m_bWireFrame = true;

  return EZ_SUCCESS;
}


