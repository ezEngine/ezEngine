
#include <RendererDX11PCH.h>

#include <RendererDX11/CommandEncoder/RenderCommandEncoderDX11.h>
#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/Resources/BufferDX11.h>
#include <RendererDX11/Resources/FenceDX11.h>
#include <RendererDX11/Resources/QueryDX11.h>
#include <RendererDX11/Resources/RenderTargetViewDX11.h>
#include <RendererDX11/Resources/ResourceViewDX11.h>
#include <RendererDX11/Resources/TextureDX11.h>
#include <RendererDX11/Resources/UnorderedAccessViewDX11.h>
#include <RendererDX11/Shader/ShaderDX11.h>
#include <RendererDX11/Shader/VertexDeclarationDX11.h>
#include <RendererDX11/State/StateDX11.h>

#include <d3d11_1.h>

ezGALRenderCommandEncoderDX11::ezGALRenderCommandEncoderDX11(ezGALDevice& device)
  : SUPER(device)
{
}

ezGALRenderCommandEncoderDX11::~ezGALRenderCommandEncoderDX11() = default;

void ezGALRenderCommandEncoderDX11::BeginEncode(const ezGALRenderingSetup& renderingSetup)
{
  if (m_RenderTargetSetup != renderingSetup.m_RenderTargetSetup)
  {
    m_RenderTargetSetup = renderingSetup.m_RenderTargetSetup;

    const ezGALRenderTargetView* pRenderTargetViews[EZ_GAL_MAX_RENDERTARGET_COUNT] = {nullptr};
    const ezGALRenderTargetView* pDepthStencilView = nullptr;

    ezUInt32 uiRenderTargetCount = 0;

    bool bFlushNeeded = false;

    if (m_RenderTargetSetup.HasRenderTargets())
    {
      for (ezUInt8 uiIndex = 0; uiIndex <= m_RenderTargetSetup.GetMaxRenderTargetIndex(); ++uiIndex)
      {
        const ezGALRenderTargetView* pRenderTargetView = GetDevice().GetRenderTargetView(m_RenderTargetSetup.GetRenderTarget(uiIndex));
        if (pRenderTargetView != nullptr)
        {
          const ezGALResourceBase* pTexture = pRenderTargetView->GetTexture()->GetParentResource();

          bFlushNeeded |= UnsetResourceViews(pTexture);
          bFlushNeeded |= UnsetUnorderedAccessViews(pTexture);
        }

        pRenderTargetViews[uiIndex] = pRenderTargetView;
      }

      uiRenderTargetCount = m_RenderTargetSetup.GetMaxRenderTargetIndex() + 1;
    }

    pDepthStencilView = GetDevice().GetRenderTargetView(m_RenderTargetSetup.GetDepthStencilTarget());
    if (pDepthStencilView != nullptr)
    {
      const ezGALResourceBase* pTexture = pDepthStencilView->GetTexture()->GetParentResource();

      bFlushNeeded |= UnsetResourceViews(pTexture);
      bFlushNeeded |= UnsetUnorderedAccessViews(pTexture);
    }

    if (bFlushNeeded)
    {
      FlushPlatform();
    }

    for (ezUInt32 i = 0; i < EZ_GAL_MAX_RENDERTARGET_COUNT; i++)
    {
      m_pBoundRenderTargets[i] = nullptr;
    }
    m_pBoundDepthStencilTarget = nullptr;

    if (uiRenderTargetCount != 0 || pDepthStencilView != nullptr)
    {
      for (ezUInt32 i = 0; i < uiRenderTargetCount; i++)
      {
        if (pRenderTargetViews[i] != nullptr)
        {
          m_pBoundRenderTargets[i] = static_cast<const ezGALRenderTargetViewDX11*>(pRenderTargetViews[i])->GetRenderTargetView();
        }
      }

      if (pDepthStencilView != nullptr)
      {
        m_pBoundDepthStencilTarget = static_cast<const ezGALRenderTargetViewDX11*>(pDepthStencilView)->GetDepthStencilView();
      }

      // Bind rendertargets, bind max(new rt count, old rt count) to overwrite bound rts if new count < old count
      m_pDXContext->OMSetRenderTargets(ezMath::Max(uiRenderTargetCount, m_uiBoundRenderTargetCount), m_pBoundRenderTargets, m_pBoundDepthStencilTarget);

      m_uiBoundRenderTargetCount = uiRenderTargetCount;
    }
    else
    {
      m_pBoundDepthStencilTarget = nullptr;
      m_pDXContext->OMSetRenderTargets(0, nullptr, nullptr);
      m_uiBoundRenderTargetCount = 0;
    }
  }

  ClearPlatform(renderingSetup.m_ClearColor, renderingSetup.m_uiRenderTargetClearMask, renderingSetup.m_bClearDepth, renderingSetup.m_bClearStencil, renderingSetup.m_fDepthClear, renderingSetup.m_uiStencilClear);
}

void ezGALRenderCommandEncoderDX11::EndEncode()
{
  // Nothing to do here
}

// Draw functions

void ezGALRenderCommandEncoderDX11::ClearPlatform(const ezColor& ClearColor, ezUInt32 uiRenderTargetClearMask, bool bClearDepth, bool bClearStencil, float fDepthClear, ezUInt8 uiStencilClear)
{
  for (ezUInt32 i = 0; i < m_uiBoundRenderTargetCount; i++)
  {
    if (uiRenderTargetClearMask & (1u << i) && m_pBoundRenderTargets[i])
    {
      m_pDXContext->ClearRenderTargetView(m_pBoundRenderTargets[i], ClearColor.GetData());
    }
  }

  if ((bClearDepth || bClearStencil) && m_pBoundDepthStencilTarget)
  {
    ezUInt32 uiClearFlags = bClearDepth ? D3D11_CLEAR_DEPTH : 0;
    uiClearFlags |= bClearStencil ? D3D11_CLEAR_STENCIL : 0;

    m_pDXContext->ClearDepthStencilView(m_pBoundDepthStencilTarget, uiClearFlags, fDepthClear, uiStencilClear);
  }
}

void ezGALRenderCommandEncoderDX11::DrawPlatform(ezUInt32 uiVertexCount, ezUInt32 uiStartVertex)
{
  FlushDeferredStateChanges();

  m_pDXContext->Draw(uiVertexCount, uiStartVertex);
}

void ezGALRenderCommandEncoderDX11::DrawIndexedPlatform(ezUInt32 uiIndexCount, ezUInt32 uiStartIndex)
{
  FlushDeferredStateChanges();

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  m_pDXContext->DrawIndexed(uiIndexCount, uiStartIndex, 0);

  // In debug builds, with a debugger attached, the engine will break on D3D errors
  // this can be very annoying when an error happens repeatedly
  // you can disable it at runtime, by using the debugger to set bChangeBreakPolicy to 'true', or dragging the
  // the instruction pointer into the if
  volatile bool bChangeBreakPolicy = false;
  if (bChangeBreakPolicy)
  {
    ezGALDeviceDX11& device = static_cast<ezGALDeviceDX11&>(GetDevice());
    if (device.m_pDebug != nullptr)
    {
      ID3D11InfoQueue* pInfoQueue = nullptr;
      if (SUCCEEDED(device.m_pDebug->QueryInterface(__uuidof(ID3D11InfoQueue), (void**)&pInfoQueue)))
      {
        // modify these, if you want to keep certain things enabled
        static BOOL bBreakOnCorruption = FALSE;
        static BOOL bBreakOnError = FALSE;
        static BOOL bBreakOnWarning = FALSE;

        pInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, bBreakOnCorruption);
        pInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, bBreakOnError);
        pInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_WARNING, bBreakOnWarning);
      }
    }
  }
#else
  m_pDXContext->DrawIndexed(uiIndexCount, uiStartIndex, 0);
#endif
}

void ezGALRenderCommandEncoderDX11::DrawIndexedInstancedPlatform(ezUInt32 uiIndexCountPerInstance, ezUInt32 uiInstanceCount, ezUInt32 uiStartIndex)
{
  FlushDeferredStateChanges();

  m_pDXContext->DrawIndexedInstanced(uiIndexCountPerInstance, uiInstanceCount, uiStartIndex, 0, 0);
}

void ezGALRenderCommandEncoderDX11::DrawIndexedInstancedIndirectPlatform(const ezGALBuffer* pIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes)
{
  FlushDeferredStateChanges();

  m_pDXContext->DrawIndexedInstancedIndirect(static_cast<const ezGALBufferDX11*>(pIndirectArgumentBuffer)->GetDXBuffer(), uiArgumentOffsetInBytes);
}

void ezGALRenderCommandEncoderDX11::DrawInstancedPlatform(ezUInt32 uiVertexCountPerInstance, ezUInt32 uiInstanceCount, ezUInt32 uiStartVertex)
{
  FlushDeferredStateChanges();

  m_pDXContext->DrawInstanced(uiVertexCountPerInstance, uiInstanceCount, uiStartVertex, 0);
}

void ezGALRenderCommandEncoderDX11::DrawInstancedIndirectPlatform(const ezGALBuffer* pIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes)
{
  FlushDeferredStateChanges();

  m_pDXContext->DrawInstancedIndirect(static_cast<const ezGALBufferDX11*>(pIndirectArgumentBuffer)->GetDXBuffer(), uiArgumentOffsetInBytes);
}

void ezGALRenderCommandEncoderDX11::DrawAutoPlatform()
{
  FlushDeferredStateChanges();

  m_pDXContext->DrawAuto();
}

void ezGALRenderCommandEncoderDX11::BeginStreamOutPlatform()
{
  FlushDeferredStateChanges();

  EZ_ASSERT_NOT_IMPLEMENTED;
}

void ezGALRenderCommandEncoderDX11::EndStreamOutPlatform()
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

void ezGALRenderCommandEncoderDX11::SetIndexBufferPlatform(const ezGALBuffer* pIndexBuffer)
{
  if (pIndexBuffer != nullptr)
  {
    const ezGALBufferDX11* pDX11Buffer = static_cast<const ezGALBufferDX11*>(pIndexBuffer);
    m_pDXContext->IASetIndexBuffer(pDX11Buffer->GetDXBuffer(), pDX11Buffer->GetIndexFormat(), 0 /* \todo: Expose */);
  }
  else
  {
    m_pDXContext->IASetIndexBuffer(nullptr, DXGI_FORMAT_R16_UINT, 0);
  }
}

void ezGALRenderCommandEncoderDX11::SetVertexBufferPlatform(ezUInt32 uiSlot, const ezGALBuffer* pVertexBuffer)
{
  EZ_ASSERT_DEV(uiSlot < EZ_GAL_MAX_VERTEX_BUFFER_COUNT, "Invalid slot index");

  m_pBoundVertexBuffers[uiSlot] = pVertexBuffer != nullptr ? static_cast<const ezGALBufferDX11*>(pVertexBuffer)->GetDXBuffer() : nullptr;
  m_VertexBufferStrides[uiSlot] = pVertexBuffer != nullptr ? pVertexBuffer->GetDescription().m_uiStructSize : 0;
  m_BoundVertexBuffersRange.SetToIncludeValue(uiSlot);
}

void ezGALRenderCommandEncoderDX11::SetVertexDeclarationPlatform(const ezGALVertexDeclaration* pVertexDeclaration)
{
  m_pDXContext->IASetInputLayout(
    pVertexDeclaration != nullptr ? static_cast<const ezGALVertexDeclarationDX11*>(pVertexDeclaration)->GetDXInputLayout() : nullptr);
}

static const D3D11_PRIMITIVE_TOPOLOGY GALTopologyToDX11[ezGALPrimitiveTopology::ENUM_COUNT] = {
  D3D11_PRIMITIVE_TOPOLOGY_POINTLIST,
  D3D11_PRIMITIVE_TOPOLOGY_LINELIST,
  D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
};

void ezGALRenderCommandEncoderDX11::SetPrimitiveTopologyPlatform(ezGALPrimitiveTopology::Enum Topology)
{
  m_pDXContext->IASetPrimitiveTopology(GALTopologyToDX11[Topology]);
}

void ezGALRenderCommandEncoderDX11::SetBlendStatePlatform(const ezGALBlendState* pBlendState, const ezColor& BlendFactor, ezUInt32 uiSampleMask)
{
  FLOAT BlendFactors[4] = {BlendFactor.r, BlendFactor.g, BlendFactor.b, BlendFactor.a};

  m_pDXContext->OMSetBlendState(
    pBlendState != nullptr ? static_cast<const ezGALBlendStateDX11*>(pBlendState)->GetDXBlendState() : nullptr, BlendFactors, uiSampleMask);
}

void ezGALRenderCommandEncoderDX11::SetDepthStencilStatePlatform(const ezGALDepthStencilState* pDepthStencilState, ezUInt8 uiStencilRefValue)
{
  m_pDXContext->OMSetDepthStencilState(
    pDepthStencilState != nullptr ? static_cast<const ezGALDepthStencilStateDX11*>(pDepthStencilState)->GetDXDepthStencilState() : nullptr,
    uiStencilRefValue);
}

void ezGALRenderCommandEncoderDX11::SetRasterizerStatePlatform(const ezGALRasterizerState* pRasterizerState)
{
  m_pDXContext->RSSetState(
    pRasterizerState != nullptr ? static_cast<const ezGALRasterizerStateDX11*>(pRasterizerState)->GetDXRasterizerState() : nullptr);
}

void ezGALRenderCommandEncoderDX11::SetViewportPlatform(const ezRectFloat& rect, float fMinDepth, float fMaxDepth)
{
  D3D11_VIEWPORT Viewport;
  Viewport.TopLeftX = rect.x;
  Viewport.TopLeftY = rect.y;
  Viewport.Width = rect.width;
  Viewport.Height = rect.height;
  Viewport.MinDepth = fMinDepth;
  Viewport.MaxDepth = fMaxDepth;

  m_pDXContext->RSSetViewports(1, &Viewport);
}

void ezGALRenderCommandEncoderDX11::SetScissorRectPlatform(const ezRectU32& rect)
{
  D3D11_RECT ScissorRect;
  ScissorRect.left = rect.x;
  ScissorRect.top = rect.y;
  ScissorRect.right = rect.x + rect.width;
  ScissorRect.bottom = rect.y + rect.height;

  m_pDXContext->RSSetScissorRects(1, &ScissorRect);
}

void ezGALRenderCommandEncoderDX11::SetStreamOutBufferPlatform(ezUInt32 uiSlot, const ezGALBuffer* pBuffer, ezUInt32 uiOffset)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

void ezGALRenderCommandEncoderDX11::FlushDeferredStateChanges()
{
  SUPER::FlushDeferredStateChanges();

  if (m_BoundVertexBuffersRange.IsValid())
  {
    const ezUInt32 uiStartSlot = m_BoundVertexBuffersRange.m_uiMin;
    const ezUInt32 uiNumSlots = m_BoundVertexBuffersRange.GetCount();

    m_pDXContext->IASetVertexBuffers(uiStartSlot, uiNumSlots, m_pBoundVertexBuffers + uiStartSlot, m_VertexBufferStrides + uiStartSlot, m_VertexBufferOffsets + uiStartSlot);

    m_BoundVertexBuffersRange.Reset();
  }
}
