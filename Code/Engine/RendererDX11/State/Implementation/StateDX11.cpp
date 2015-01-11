
#include <RendererDX11/PCH.h>
#include <RendererDX11/Basics.h>
#include <RendererDX11/State/StateDX11.h>
#include <RendererDX11/Device/DeviceDX11.h>

#include <d3d11.h>


// Mapping tables to map ezGAL constants to DX11 constants
#include <RendererDX11/State/Implementation/StateDX11_MappingTables.inl>

// Blend state

ezGALBlendStateDX11::ezGALBlendStateDX11(const ezGALBlendStateCreationDescription& Description)
  : ezGALBlendState(Description),
    m_pDXBlendState(nullptr)
{
}

ezGALBlendStateDX11::~ezGALBlendStateDX11()
{
}

static D3D11_BLEND_OP ToD3DBlendOp(ezGALBlendOp::Enum e)
{
  switch (e)
  {
  case ezGALBlendOp::Add:
    return D3D11_BLEND_OP_ADD;
  case ezGALBlendOp::Max:
    return D3D11_BLEND_OP_MAX;
  case ezGALBlendOp::Min:
    return D3D11_BLEND_OP_MIN;
  case ezGALBlendOp::RevSubtract:
    return D3D11_BLEND_OP_REV_SUBTRACT;
  case ezGALBlendOp::Subtract:
    return D3D11_BLEND_OP_SUBTRACT;
  default:
    EZ_ASSERT_NOT_IMPLEMENTED;
  }

  return D3D11_BLEND_OP_ADD;
}

static D3D11_BLEND ToD3DBlend(ezGALBlend::Enum e)
{
  switch (e)
  {
  case ezGALBlend::BlendFactor:
    EZ_ASSERT_NOT_IMPLEMENTED;
    // if this is used, it also must be implemented in ezGALContextDX11::SetBlendStatePlatform
    return D3D11_BLEND_BLEND_FACTOR;
  case ezGALBlend::DestAlpha:
    return D3D11_BLEND_DEST_ALPHA;
  case ezGALBlend::DestColor:
    return D3D11_BLEND_DEST_COLOR;
  case ezGALBlend::InvBlendFactor:
    EZ_ASSERT_NOT_IMPLEMENTED;
    // if this is used, it also must be implemented in ezGALContextDX11::SetBlendStatePlatform
    return D3D11_BLEND_INV_BLEND_FACTOR;
  case ezGALBlend::InvDestAlpha:
    return D3D11_BLEND_INV_DEST_ALPHA;
  case ezGALBlend::InvDestColor:
    return D3D11_BLEND_INV_DEST_COLOR;
  case ezGALBlend::InvSrcAlpha:
    return D3D11_BLEND_INV_SRC_ALPHA;
  case ezGALBlend::InvSrcColor:
    return D3D11_BLEND_INV_SRC_COLOR;
  case ezGALBlend::One:
    return D3D11_BLEND_ONE;
  case ezGALBlend::SrcAlpha:
    return D3D11_BLEND_SRC_ALPHA;
  case ezGALBlend::SrcAlphaSaturated:
    return D3D11_BLEND_SRC_ALPHA_SAT;
  case ezGALBlend::SrcColor:
    return D3D11_BLEND_SRC_COLOR;
  case ezGALBlend::Zero:
    return D3D11_BLEND_ZERO;

  default:
    EZ_ASSERT_NOT_IMPLEMENTED;
  }

  return D3D11_BLEND_ONE;
}

ezResult ezGALBlendStateDX11::InitPlatform(ezGALDevice* pDevice)
{
  D3D11_BLEND_DESC DXDesc;
  DXDesc.AlphaToCoverageEnable = m_Description.m_bAlphaToCoverage;
  DXDesc.IndependentBlendEnable = m_Description.m_bIndependentBlend;

  for (ezInt32 i = 0; i < 8; ++i)
  {
    DXDesc.RenderTarget[i].BlendEnable    = m_Description.m_RenderTargetBlendDescriptions[i].m_bBlendingEnabled;
    DXDesc.RenderTarget[i].BlendOp        = ToD3DBlendOp(m_Description.m_RenderTargetBlendDescriptions[i].m_BlendOp);
    DXDesc.RenderTarget[i].BlendOpAlpha   = ToD3DBlendOp(m_Description.m_RenderTargetBlendDescriptions[i].m_BlendOpAlpha);
    DXDesc.RenderTarget[i].DestBlend      = ToD3DBlend(m_Description.m_RenderTargetBlendDescriptions[i].m_DestBlend);
    DXDesc.RenderTarget[i].DestBlendAlpha = ToD3DBlend(m_Description.m_RenderTargetBlendDescriptions[i].m_DestBlendAlpha);
    DXDesc.RenderTarget[i].SrcBlend       = ToD3DBlend(m_Description.m_RenderTargetBlendDescriptions[i].m_SourceBlend);
    DXDesc.RenderTarget[i].SrcBlendAlpha  = ToD3DBlend(m_Description.m_RenderTargetBlendDescriptions[i].m_SourceBlendAlpha);
    DXDesc.RenderTarget[i].RenderTargetWriteMask = m_Description.m_RenderTargetBlendDescriptions[i].m_uiWriteMask & 0x0F; // D3D11: RenderTargetWriteMask can only have the least significant 4 bits set.
  }

  if (FAILED(static_cast<ezGALDeviceDX11*>(pDevice)->GetDXDevice()->CreateBlendState(&DXDesc, &m_pDXBlendState)))
  {
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

ezResult ezGALBlendStateDX11::DeInitPlatform(ezGALDevice* pDevice)
{
  EZ_GAL_DX11_RELEASE(m_pDXBlendState);
  return EZ_SUCCESS;
}

// Depth Stencil state

ezGALDepthStencilStateDX11::ezGALDepthStencilStateDX11(const ezGALDepthStencilStateCreationDescription& Description)
  : ezGALDepthStencilState(Description),
    m_pDXDepthStencilState(nullptr)
{
}

ezGALDepthStencilStateDX11::~ezGALDepthStencilStateDX11()
{
}

ezResult ezGALDepthStencilStateDX11::InitPlatform(ezGALDevice* pDevice)
{
  D3D11_DEPTH_STENCIL_DESC DXDesc;
  DXDesc.DepthEnable = m_Description.m_bDepthTest;
  DXDesc.DepthWriteMask = m_Description.m_bDepthWrite ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
  DXDesc.DepthFunc = GALCompareFuncToDX11[m_Description.m_DepthTestFunc];
  DXDesc.StencilEnable = m_Description.m_bStencilTest;
  DXDesc.StencilReadMask = m_Description.m_uiStencilReadMask;
  DXDesc.StencilWriteMask = m_Description.m_uiStencilWriteMask;

  DXDesc.FrontFace.StencilDepthFailOp = GALStencilOpTableIndexToDX11[m_Description.m_FrontFaceStencilOp.m_FailOp];
  DXDesc.FrontFace.StencilDepthFailOp = GALStencilOpTableIndexToDX11[m_Description.m_FrontFaceStencilOp.m_DepthFailOp];
  DXDesc.FrontFace.StencilPassOp = GALStencilOpTableIndexToDX11[m_Description.m_FrontFaceStencilOp.m_PassOp];
  DXDesc.FrontFace.StencilFunc = GALCompareFuncToDX11[m_Description.m_FrontFaceStencilOp.m_StencilFunc];

  const ezGALStencilOpDescription& backFaceStencilOp = m_Description.m_bSeparateFrontAndBack ? m_Description.m_BackFaceStencilOp : m_Description.m_FrontFaceStencilOp;
  DXDesc.BackFace.StencilDepthFailOp = GALStencilOpTableIndexToDX11[backFaceStencilOp.m_FailOp];
  DXDesc.BackFace.StencilDepthFailOp = GALStencilOpTableIndexToDX11[backFaceStencilOp.m_DepthFailOp];
  DXDesc.BackFace.StencilPassOp = GALStencilOpTableIndexToDX11[backFaceStencilOp.m_PassOp];
  DXDesc.BackFace.StencilFunc = GALCompareFuncToDX11[backFaceStencilOp.m_StencilFunc];


  if (FAILED(static_cast<ezGALDeviceDX11*>(pDevice)->GetDXDevice()->CreateDepthStencilState(&DXDesc, &m_pDXDepthStencilState)))
  {
    return EZ_FAILURE;
  }
  else
  {
    return EZ_SUCCESS;
  }
}

ezResult ezGALDepthStencilStateDX11::DeInitPlatform(ezGALDevice* pDevice)
{
  EZ_GAL_DX11_RELEASE(m_pDXDepthStencilState);
  return EZ_SUCCESS;
}


// Rasterizer state

ezGALRasterizerStateDX11::ezGALRasterizerStateDX11(const ezGALRasterizerStateCreationDescription& Description)
  : ezGALRasterizerState(Description),
    m_pDXRasterizerState(nullptr)
{
}

ezGALRasterizerStateDX11::~ezGALRasterizerStateDX11()
{
}



ezResult ezGALRasterizerStateDX11::InitPlatform(ezGALDevice* pDevice)
{
  D3D11_RASTERIZER_DESC DXDesc;
  DXDesc.AntialiasedLineEnable = m_Description.m_bLineAA ? TRUE : FALSE;
  DXDesc.CullMode = GALCullModeToDX11[m_Description.m_CullMode];
  DXDesc.DepthBias = m_Description.m_iDepthBias;
  DXDesc.DepthBiasClamp = m_Description.m_fDepthBiasClamp;
  DXDesc.DepthClipEnable = m_Description.m_bDepthClip ? TRUE : FALSE;
  DXDesc.FillMode = m_Description.m_bWireFrame ? D3D11_FILL_WIREFRAME : D3D11_FILL_SOLID;
  DXDesc.FrontCounterClockwise = m_Description.m_bFrontCounterClockwise;
  DXDesc.MultisampleEnable = m_Description.m_bMSAA;
  DXDesc.ScissorEnable = m_Description.m_bScissorTest;
  DXDesc.SlopeScaledDepthBias = m_Description.m_fSlopeScaledDepthBias;

  if(FAILED(static_cast<ezGALDeviceDX11*>(pDevice)->GetDXDevice()->CreateRasterizerState(&DXDesc, &m_pDXRasterizerState)))
  {
    return EZ_FAILURE;
  }
  else
  {
    return EZ_SUCCESS;
  }
}


ezResult ezGALRasterizerStateDX11::DeInitPlatform(ezGALDevice* pDevice)
{
  EZ_GAL_DX11_RELEASE(m_pDXRasterizerState);
  return EZ_SUCCESS;
}


// Sampler state

ezGALSamplerStateDX11::ezGALSamplerStateDX11(const ezGALSamplerStateCreationDescription& Description)
: ezGALSamplerState(Description),
m_pDXSamplerState(nullptr)
{
}

ezGALSamplerStateDX11::~ezGALSamplerStateDX11()
{
}

/*
*/

ezResult ezGALSamplerStateDX11::InitPlatform(ezGALDevice* pDevice)
{
  D3D11_SAMPLER_DESC DXDesc;
  DXDesc.AddressU = GALTextureAddressModeToDX11[m_Description.m_AddressU];
  DXDesc.AddressV = GALTextureAddressModeToDX11[m_Description.m_AddressV];
  DXDesc.AddressW = GALTextureAddressModeToDX11[m_Description.m_AddressW];
  DXDesc.BorderColor[0] = m_Description.m_BorderColor.r;
  DXDesc.BorderColor[1] = m_Description.m_BorderColor.g;
  DXDesc.BorderColor[2] = m_Description.m_BorderColor.b;
  DXDesc.BorderColor[3] = m_Description.m_BorderColor.a;
  DXDesc.ComparisonFunc = GALCompareFuncToDX11[m_Description.m_SampleCompareFunc];

  if (m_Description.m_MagFilter == ezGALTextureFilterMode::Anisotropic || m_Description.m_MinFilter == ezGALTextureFilterMode::Anisotropic || m_Description.m_MipFilter == ezGALTextureFilterMode::Anisotropic)
  {
    if (m_Description.m_SampleCompareFunc == ezGALCompareFunc::Never)
      DXDesc.Filter = D3D11_FILTER_ANISOTROPIC;
    else
      DXDesc.Filter = D3D11_FILTER_COMPARISON_ANISOTROPIC;
  }
  else
  {
    ezUInt32 uiTableIndex = 0;

    if (m_Description.m_MipFilter == ezGALTextureFilterMode::Linear)
      uiTableIndex |= 1;

    if (m_Description.m_MagFilter == ezGALTextureFilterMode::Linear)
      uiTableIndex |= 2;

    if (m_Description.m_MinFilter == ezGALTextureFilterMode::Linear)
      uiTableIndex |= 4;

    if (m_Description.m_SampleCompareFunc != ezGALCompareFunc::Never)
      uiTableIndex |= 8;

    DXDesc.Filter = GALFilterTableIndexToDX11[uiTableIndex];
  }
  
  DXDesc.MaxAnisotropy = m_Description.m_uiMaxAnisotropy;
  DXDesc.MaxLOD = m_Description.m_fMaxMip;
  DXDesc.MinLOD = m_Description.m_fMinMip;
  DXDesc.MipLODBias = m_Description.m_fMipLodBias;

  if (FAILED(static_cast<ezGALDeviceDX11*>(pDevice)->GetDXDevice()->CreateSamplerState(&DXDesc, &m_pDXSamplerState)))
  {
    return EZ_FAILURE;
  }
  else
  {
    return EZ_SUCCESS;
  }
}


ezResult ezGALSamplerStateDX11::DeInitPlatform(ezGALDevice* pDevice)
{
  EZ_GAL_DX11_RELEASE(m_pDXSamplerState);
  return EZ_SUCCESS;
}



EZ_STATICLINK_FILE(RendererDX11, RendererDX11_State_Implementation_StateDX11);

