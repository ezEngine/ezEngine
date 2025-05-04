#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>
#include <RendererFoundation/Resources/RenderTargetView.h>

namespace
{
  struct RenderTargetViewInfo
  {
    ezEnum<ezGALMSAASampleCount> m_MSAA;
    ezSizeU32 m_Size = {0, 0};
    ezUInt32 m_uiSliceCount = 0;
  };

  RenderTargetViewInfo getRenderTargetViewInfo(ezGALRenderTargetViewHandle hView, ezEnum<ezGALResourceFormat>& out_format)
  {
    RenderTargetViewInfo info;
    const ezGALRenderTargetView* pRTV = ezGALDevice::GetDefaultDevice()->GetRenderTargetView(hView);
    EZ_ASSERT_DEV(pRTV, "Render target view must be valid");
    const ezGALRenderTargetViewCreationDescription& viewDesc = pRTV->GetDescription();
    const ezGALTextureCreationDescription& texDesc = pRTV->GetTexture()->GetDescription();
    out_format = viewDesc.m_OverrideViewFormat;
    if (out_format == ezGALResourceFormat::Invalid)
      out_format = texDesc.m_Format;

    info.m_MSAA = texDesc.m_SampleCount;
    ezVec3U32 size = pRTV->GetTexture()->GetMipMapSize(viewDesc.m_uiMipLevel);
    info.m_Size = {size.x, size.y};
    info.m_uiSliceCount = viewDesc.m_uiSliceCount;
    return info;
  }
} // namespace

bool ezGALRenderTargets::operator==(const ezGALRenderTargets& other) const
{
  if (m_hDSTarget != other.m_hDSTarget)
    return false;

  for (ezUInt8 uiRTIndex = 0; uiRTIndex < EZ_GAL_MAX_RENDERTARGET_COUNT; ++uiRTIndex)
  {
    if (m_hRTs[uiRTIndex] != other.m_hRTs[uiRTIndex])
      return false;
  }
  return true;
}

bool ezGALRenderTargets::operator!=(const ezGALRenderTargets& other) const
{
  return !(*this == other);
}

ezGALRenderingSetup& ezGALRenderingSetup::SetColorTarget(ezUInt8 uiIndex, ezGALRenderTargetViewHandle hRenderTarget, ezEnum<ezGALRenderTargetLoadOp> loadOp, ezEnum<ezGALRenderTargetStoreOp> storeOp)
{
  EZ_ASSERT_DEBUG(uiIndex <= m_RenderPass.m_uiRTCount, "Render targets must be defined in order, starting at 0 and must not have gaps. The index {} should be less or equal to {}", uiIndex, m_RenderPass.m_uiRTCount);

  RenderTargetViewInfo info = getRenderTargetViewInfo(hRenderTarget, m_RenderPass.m_ColorFormat[uiIndex]);
  const bool bFirstRenderTarget = GetColorTargetCount() == 0 && m_FrameBuffer.m_hDepthTarget.IsInvalidated();
  if (!bFirstRenderTarget)
  {
    EZ_ASSERT_DEBUG(m_RenderPass.m_Msaa == info.m_MSAA, "Missmatch between this render target's MSAA mode ({}) and the previously set MSAA mode ({}).", info.m_MSAA, m_RenderPass.m_Msaa);
    EZ_ASSERT_DEBUG(m_FrameBuffer.m_Size == info.m_Size, "Missmatch between this render target's size ({}) and the previously set size ({}).", info.m_Size, m_FrameBuffer.m_Size);
    EZ_ASSERT_DEBUG(m_FrameBuffer.m_uiSliceCount == info.m_uiSliceCount, "Missmatch between this render target's slice count ({}) and the previously set slice count ({}).", info.m_uiSliceCount, m_FrameBuffer.m_uiSliceCount);
  }
  else
  {
    m_RenderPass.m_Msaa = info.m_MSAA;
    m_FrameBuffer.m_Size = info.m_Size;
    m_FrameBuffer.m_uiSliceCount = info.m_uiSliceCount;
  }

  EZ_ASSERT_DEBUG(!ezGALResourceFormat::IsDepthFormat(m_RenderPass.m_ColorFormat[uiIndex]), "The format {} must be a color format", m_RenderPass.m_DepthFormat);

  m_FrameBuffer.m_hColorTarget[uiIndex] = hRenderTarget;
  m_RenderPass.m_uiRTCount = ezMath::Max(m_RenderPass.m_uiRTCount, static_cast<ezUInt8>(uiIndex + 1u));
  m_RenderPass.m_ColorLoadOp[uiIndex] = loadOp;
  m_RenderPass.m_ColorStoreOp[uiIndex] = storeOp;

  return *this;
}

ezGALRenderingSetup& ezGALRenderingSetup::SetDepthStencilTarget(ezGALRenderTargetViewHandle hDSTarget, ezEnum<ezGALRenderTargetLoadOp> depthLoadOp, ezEnum<ezGALRenderTargetStoreOp> depthStoreOp, ezEnum<ezGALRenderTargetLoadOp> stencilLoadOp, ezEnum<ezGALRenderTargetStoreOp> stencilStoreOp)
{
  RenderTargetViewInfo info = getRenderTargetViewInfo(hDSTarget, m_RenderPass.m_DepthFormat);
  const bool bFirstRenderTarget = GetColorTargetCount() == 0 && m_FrameBuffer.m_hDepthTarget.IsInvalidated();
  if (!bFirstRenderTarget)
  {
    EZ_ASSERT_DEBUG(m_RenderPass.m_Msaa == info.m_MSAA, "Missmatch between this render target's MSAA mode ({}) and the previously set MSAA mode ({}).", info.m_MSAA, m_RenderPass.m_Msaa);
    m_FrameBuffer.m_Size.height = ezMath::Min(m_FrameBuffer.m_Size.height, info.m_Size.height);
    m_FrameBuffer.m_Size.width = ezMath::Min(m_FrameBuffer.m_Size.width, info.m_Size.width);
    EZ_ASSERT_DEBUG(m_FrameBuffer.m_uiSliceCount == info.m_uiSliceCount, "Missmatch between this render target's slice count ({}) and the previously set slice count ({}).", info.m_uiSliceCount, m_FrameBuffer.m_uiSliceCount);
  }
  else
  {
    m_RenderPass.m_Msaa = info.m_MSAA;
    m_FrameBuffer.m_Size = info.m_Size;
    m_FrameBuffer.m_uiSliceCount = info.m_uiSliceCount;
  }

  m_FrameBuffer.m_hDepthTarget = hDSTarget;
  m_RenderPass.m_DepthLoadOp = depthLoadOp;
  m_RenderPass.m_DepthStoreOp = depthStoreOp;

  EZ_ASSERT_DEBUG(ezGALResourceFormat::IsDepthFormat(m_RenderPass.m_DepthFormat), "The format {} is not a depth format", m_RenderPass.m_DepthFormat);
  if (ezGALResourceFormat::IsStencilFormat(m_RenderPass.m_DepthFormat))
  {
    m_RenderPass.m_StencilLoadOp = stencilLoadOp;
    m_RenderPass.m_StencilStoreOp = stencilStoreOp;
  }
  else
  {
    m_RenderPass.m_StencilLoadOp = ezGALRenderTargetLoadOp::DontCare;
    m_RenderPass.m_StencilStoreOp = ezGALRenderTargetStoreOp::Discard;
  }
  return *this;
}

ezGALRenderingSetup& ezGALRenderingSetup::SetClearColor(ezUInt8 uiIndex, const ezColor& color)
{
  EZ_ASSERT_DEBUG(uiIndex < m_RenderPass.m_uiRTCount, "Render target not set, call SetRenderTarget first");
  m_ClearColor[uiIndex] = color;
  m_RenderPass.m_ColorLoadOp[uiIndex] = ezGALRenderTargetLoadOp::Clear;
  return *this;
}

ezGALRenderingSetup& ezGALRenderingSetup::SetClearDepth(float fDepthClear)
{
  EZ_ASSERT_DEBUG(HasDepthStencilTarget(), "Depth target not set, call SetDepthStencilTarget first");
  m_fClearDepth = fDepthClear;
  m_RenderPass.m_DepthLoadOp = ezGALRenderTargetLoadOp::Clear;
  return *this;
}

ezGALRenderingSetup& ezGALRenderingSetup::SetClearStencil(ezUInt8 uiStencilClear)
{
  EZ_ASSERT_DEBUG(HasDepthStencilTarget(), "Depth target not set, call SetDepthStencilTarget first");
  m_uiClearStencil = uiStencilClear;
  m_RenderPass.m_StencilLoadOp = ezGALRenderTargetLoadOp::Clear;
  return *this;
}

void ezGALRenderingSetup::Reset()
{
  *this = ezGALRenderingSetup();
}

void ezGALRenderingSetup::DestroyAllAttachedViews()
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  ezArrayPtr<ezGALRenderTargetViewHandle> colorViews(m_FrameBuffer.m_hColorTarget);
  for (ezGALRenderTargetViewHandle& hView : colorViews)
  {
    if (!hView.IsInvalidated())
    {
      pDevice->DestroyRenderTargetView(hView);
      hView.Invalidate();
    }
  }

  if (!m_FrameBuffer.m_hDepthTarget.IsInvalidated())
  {
    pDevice->DestroyRenderTargetView(m_FrameBuffer.m_hDepthTarget);
    m_FrameBuffer.m_hDepthTarget.Invalidate();
  }
  Reset();
}
