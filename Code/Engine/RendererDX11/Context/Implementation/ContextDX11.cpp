
#include <RendererDX11/PCH.h>
#include <RendererDX11/Context/ContextDX11.h>
#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/Device/SwapChainDX11.h>
#include <RendererDX11/Shader/ShaderDX11.h>
#include <RendererDX11/Resources/BufferDX11.h>
#include <RendererDX11/Resources/TextureDX11.h>
#include <RendererDX11/Resources/RenderTargetViewDX11.h>
#include <RendererDX11/Shader/VertexDeclarationDX11.h>
#include <RendererDX11/State/StateDX11.h>
#include <RendererDX11/Resources/ResourceViewDX11.h>

#include <Foundation/Logging/Log.h>
#include <System/Window/Window.h>
#include <Foundation/Math/Color.h>

#include <d3d11.h>
#include <d3d11_1.h>


ezGALContextDX11::ezGALContextDX11(ezGALDevice* pDevice, ID3D11DeviceContext* pDXContext)
: ezGALContext(pDevice),
  m_pDXContext(pDXContext),
  m_pDXAnnotation(nullptr),
  m_pBoundDepthStencilTarget(nullptr),
  m_uiBoundRenderTargetCount(0)
{
  EZ_ASSERT_RELEASE(m_pDXContext != nullptr, "Invalid DX context!");

  if (FAILED(m_pDXContext->QueryInterface(__uuidof(ID3DUserDefinedAnnotation), (void **)&m_pDXAnnotation)))
  {
    ezLog::Warning("Failed to get annotation interface. GALContext marker will not work");
  }

  for (ezUInt32 i = 0; i < EZ_GAL_MAX_RENDERTARGET_COUNT; i++)
  {
    m_pBoundRenderTargets[i] = nullptr;
  }

  for (ezUInt32 i = 0; i < EZ_GAL_MAX_VERTEX_BUFFER_COUNT; i++)
  {
    m_pBoundVertexBuffers[i] = nullptr;
    m_VertexBufferOffsets[i] = 0;
    m_VertexBufferStrides[i] = 0;
  }

  for (ezUInt32 i = 0; i < EZ_GAL_MAX_CONSTANT_BUFFER_COUNT; i++)
  {
    m_pBoundConstantBuffers[i] = nullptr;
  }

  for (ezUInt32 s = 0; s < ezGALShaderStage::ENUM_COUNT; s++)
  {
    for (ezUInt32 i = 0; i < EZ_GAL_MAX_SHADER_RESOURCE_VIEW_COUNT; i++)
    {
      m_pBoundShaderResourceViews[s][i] = nullptr;
      m_pBoundSamplerStates[s][i] = nullptr;
    }
  }
}

ezGALContextDX11::~ezGALContextDX11()
{
  EZ_GAL_DX11_RELEASE(m_pDXContext);
  EZ_GAL_DX11_RELEASE(m_pDXAnnotation);
}


// Draw functions

void ezGALContextDX11::ClearPlatform(const ezColor& ClearColor, ezUInt32 uiRenderTargetClearMask, bool bClearDepth, bool bClearStencil, float fDepthClear, ezUInt8 uiStencilClear)
{
  for (ezUInt32 i = 0; i < m_uiBoundRenderTargetCount; i++)
  {
    if (uiRenderTargetClearMask & (1u << i))
    {
      m_pDXContext->ClearRenderTargetView(m_pBoundRenderTargets[i], ClearColor.GetData());
    }
  }

  if ((bClearDepth || bClearStencil) && m_pBoundDepthStencilTarget)
  {
    /// \todo Parameters
    m_pDXContext->ClearDepthStencilView(m_pBoundDepthStencilTarget, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, fDepthClear, uiStencilClear);
  }
}

void ezGALContextDX11::DrawPlatform(ezUInt32 uiVertexCount, ezUInt32 uiStartVertex)
{
  FlushDeferredStateChanges();

  m_pDXContext->Draw(uiVertexCount, uiStartVertex);
}

void ezGALContextDX11::DrawIndexedPlatform(ezUInt32 uiIndexCount, ezUInt32 uiStartIndex)
{
  FlushDeferredStateChanges();

  m_pDXContext->DrawIndexed(uiIndexCount, uiStartIndex, 0);
}

void ezGALContextDX11::DrawIndexedInstancedPlatform(ezUInt32 uiIndexCountPerInstance, ezUInt32 uiInstanceCount, ezUInt32 uiStartIndex)
{
  FlushDeferredStateChanges();

  m_pDXContext->DrawIndexedInstanced(uiIndexCountPerInstance, uiInstanceCount, uiStartIndex, 0, 0);
}

void ezGALContextDX11::DrawIndexedInstancedIndirectPlatform(ezGALBuffer* pIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes)
{
  FlushDeferredStateChanges();

  m_pDXContext->DrawIndexedInstancedIndirect(static_cast<ezGALBufferDX11*>(pIndirectArgumentBuffer)->GetDXBuffer(), uiArgumentOffsetInBytes);
}

void ezGALContextDX11::DrawInstancedPlatform(ezUInt32 uiVertexCountPerInstance, ezUInt32 uiInstanceCount, ezUInt32 uiStartVertex)
{
  FlushDeferredStateChanges();

  m_pDXContext->DrawInstanced(uiVertexCountPerInstance, uiInstanceCount, uiStartVertex, 0);
}

void ezGALContextDX11::DrawInstancedIndirectPlatform(ezGALBuffer* pIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes)
{
  FlushDeferredStateChanges();

  m_pDXContext->DrawInstancedIndirect(static_cast<ezGALBufferDX11*>(pIndirectArgumentBuffer)->GetDXBuffer(), uiArgumentOffsetInBytes);
}

void ezGALContextDX11::DrawAutoPlatform()
{
  FlushDeferredStateChanges();

  m_pDXContext->DrawAuto();
}

void ezGALContextDX11::BeginStreamOutPlatform()
{
  FlushDeferredStateChanges();
}

void ezGALContextDX11::EndStreamOutPlatform()
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

// Some state changes are deferred so they can be updated faster
void ezGALContextDX11::FlushDeferredStateChanges()
{
  if (m_DeferredStateChanged.IsSet(ezGALDX11::DeferredStateChanged::VertexBuffer))
  {
    m_pDXContext->IASetVertexBuffers(0, EZ_GAL_MAX_VERTEX_BUFFER_COUNT, m_pBoundVertexBuffers, m_VertexBufferStrides, m_VertexBufferOffsets);
  }

  if (m_DeferredStateChanged.IsSet(ezGALDX11::DeferredStateChanged::ConstantBuffer))
  {
    m_pDXContext->VSSetConstantBuffers(0, 4, m_pBoundConstantBuffers);
    m_pDXContext->PSSetConstantBuffers(0, 4, m_pBoundConstantBuffers);
  }

  if (m_DeferredStateChanged.IsSet(ezGALDX11::DeferredStateChanged::ShaderResourceView))
  {
    /// \todo Optimize / add other stages
    m_pDXContext->PSSetShaderResources(0, EZ_GAL_MAX_SHADER_RESOURCE_VIEW_COUNT, m_pBoundShaderResourceViews[ezGALShaderStage::PixelShader]);
  }

  if (m_DeferredStateChanged.IsSet(ezGALDX11::DeferredStateChanged::SamplerState))
  {
    /// \todo Optimize / add other stages
    m_pDXContext->PSSetSamplers(0, EZ_GAL_MAX_SHADER_RESOURCE_VIEW_COUNT, m_pBoundSamplerStates[ezGALShaderStage::PixelShader]);
  }

  m_DeferredStateChanged.Clear();
}

// Dispatch

void ezGALContextDX11::DispatchPlatform(ezUInt32 uiThreadGroupCountX, ezUInt32 uiThreadGroupCountY, ezUInt32 uiThreadGroupCountZ)
{
  FlushDeferredStateChanges();

  m_pDXContext->Dispatch(uiThreadGroupCountX, uiThreadGroupCountY, uiThreadGroupCountZ);
}

void ezGALContextDX11::DispatchIndirectPlatform(ezGALBuffer* pIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes)
{
  FlushDeferredStateChanges();

  m_pDXContext->DispatchIndirect(static_cast<ezGALBufferDX11*>(pIndirectArgumentBuffer)->GetDXBuffer(), uiArgumentOffsetInBytes);
}


// State setting functions

void ezGALContextDX11::SetShaderPlatform(ezGALShader* pShader)
{
  /// \todo Optimize (change only shaders which need to be set)
  if (pShader != nullptr)
  {
    ezGALShaderDX11* pDXShader = static_cast<ezGALShaderDX11*>(pShader);

    m_pDXContext->VSSetShader(pDXShader->GetDXVertexShader(), nullptr, 0);
    m_pDXContext->HSSetShader(pDXShader->GetDXHullShader(), nullptr, 0);
    m_pDXContext->DSSetShader(pDXShader->GetDXDomainShader(), nullptr, 0);
    m_pDXContext->GSSetShader(pDXShader->GetDXGeometryShader(), nullptr, 0);
    m_pDXContext->PSSetShader(pDXShader->GetDXPixelShader(), nullptr, 0);
    m_pDXContext->CSSetShader(pDXShader->GetDXComputeShader(), nullptr, 0);
  }
  else
  {
    m_pDXContext->VSSetShader(nullptr, nullptr, 0);
    m_pDXContext->HSSetShader(nullptr, nullptr, 0);
    m_pDXContext->DSSetShader(nullptr, nullptr, 0);
    m_pDXContext->GSSetShader(nullptr, nullptr, 0);
    m_pDXContext->PSSetShader(nullptr, nullptr, 0);
    m_pDXContext->CSSetShader(nullptr, nullptr, 0);
  }

}

void ezGALContextDX11::SetIndexBufferPlatform(ezGALBuffer* pIndexBuffer)
{
  if (pIndexBuffer != nullptr)
  {
    ezGALBufferDX11* pDX11Buffer = static_cast<ezGALBufferDX11*>(pIndexBuffer);
    m_pDXContext->IASetIndexBuffer(pDX11Buffer->GetDXBuffer(), pDX11Buffer->GetIndexFormat(), 0 /* \todo: Expose */);
  }
  else
  {
    m_pDXContext->IASetIndexBuffer(nullptr, DXGI_FORMAT_R16_UINT, 0);
  }
}

void ezGALContextDX11::SetVertexBufferPlatform(ezUInt32 uiSlot, ezGALBuffer* pVertexBuffer)
{
  EZ_ASSERT_DEV(uiSlot < EZ_GAL_MAX_VERTEX_BUFFER_COUNT, "Invalid slot index");

  m_pBoundVertexBuffers[uiSlot] = pVertexBuffer != nullptr ? static_cast<ezGALBufferDX11*>(pVertexBuffer)->GetDXBuffer() : nullptr;
  m_VertexBufferStrides[uiSlot] = pVertexBuffer != nullptr ? pVertexBuffer->GetDescription().m_uiStructSize : 0;
  m_DeferredStateChanged.Add(ezGALDX11::DeferredStateChanged::VertexBuffer);
}

void ezGALContextDX11::SetVertexDeclarationPlatform(ezGALVertexDeclaration* pVertexDeclaration)
{
  m_pDXContext->IASetInputLayout(pVertexDeclaration != nullptr ? static_cast<ezGALVertexDeclarationDX11*>(pVertexDeclaration)->GetDXInputLayout() : nullptr);
}


static const D3D11_PRIMITIVE_TOPOLOGY GALTopologyToDX11[ezGALPrimitiveTopology::ENUM_COUNT] =
{
  D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST

  /// \todo Add other primitive types (when adding to ez enum)
};

void ezGALContextDX11::SetPrimitiveTopologyPlatform(ezGALPrimitiveTopology::Enum Topology)
{
  m_pDXContext->IASetPrimitiveTopology(GALTopologyToDX11[Topology]);
}

void ezGALContextDX11::SetConstantBufferPlatform(ezUInt32 uiSlot, ezGALBuffer* pBuffer)
{
  /// \todo Check if the device supports the slot index?
  m_pBoundConstantBuffers[uiSlot] = pBuffer != nullptr ? static_cast<ezGALBufferDX11*>(pBuffer)->GetDXBuffer() : nullptr;
  m_DeferredStateChanged.Add(ezGALDX11::DeferredStateChanged::ConstantBuffer);
}

void ezGALContextDX11::SetSamplerStatePlatform(ezGALShaderStage::Enum Stage, ezUInt32 uiSlot, ezGALSamplerState* pSamplerState)
{
  /// \todo Check if the device supports the stage / the slot index
  m_pBoundSamplerStates[Stage][uiSlot] = pSamplerState != nullptr ? static_cast<ezGALSamplerStateDX11*>(pSamplerState)->GetDXSamplerState() : nullptr;
  m_DeferredStateChanged.Add(ezGALDX11::DeferredStateChanged::SamplerState);
}

void ezGALContextDX11::SetResourceViewPlatform(ezGALShaderStage::Enum Stage, ezUInt32 uiSlot, ezGALResourceView* pResourceView)
{
  m_pBoundShaderResourceViews[Stage][uiSlot] = pResourceView != nullptr ? static_cast<ezGALResourceViewDX11*>(pResourceView)->GetDXResourceView() : nullptr;
  m_DeferredStateChanged.Add(ezGALDX11::DeferredStateChanged::ShaderResourceView);
}

void ezGALContextDX11::SetRenderTargetSetupPlatform( ezGALRenderTargetView** ppRenderTargetViews, ezUInt32 uiRenderTargetCount, ezGALRenderTargetView* pDepthStencilView )
{
  for (ezUInt32 i = 0; i < EZ_GAL_MAX_RENDERTARGET_COUNT; i++)
  {
    m_pBoundRenderTargets[i] = nullptr;
  }
  m_pBoundDepthStencilTarget = nullptr;

  if (uiRenderTargetCount > 0 || pDepthStencilView != nullptr)
  {
    for ( ezUInt32 i = 0; i < uiRenderTargetCount; i++ )
    {
      if ( ppRenderTargetViews[i] != nullptr )
      {
        m_pBoundRenderTargets[i] = static_cast<ezGALRenderTargetViewDX11*>(ppRenderTargetViews[i])->GetRenderTargetView();
      }
      
    }

    if (pDepthStencilView != nullptr )
    {
      m_pBoundDepthStencilTarget = static_cast<ezGALRenderTargetViewDX11*>(pDepthStencilView)->GetDepthStencilView();
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

void ezGALContextDX11::SetUnorderedAccessViewPlatform(ezUInt32 uiSlot, ezGALResourceView* pResourceView)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

void ezGALContextDX11::SetBlendStatePlatform(ezGALBlendState* pBlendState, const ezColor& BlendFactor, ezUInt32 uiSampleMask)
{
  FLOAT BlendFactors[4] =
  {
    BlendFactor.r,
    BlendFactor.g,
    BlendFactor.b,
    BlendFactor.a
  };

  m_pDXContext->OMSetBlendState(pBlendState != nullptr ? static_cast<ezGALBlendStateDX11*>(pBlendState)->GetDXBlendState() : nullptr, BlendFactors, uiSampleMask);
}

void ezGALContextDX11::SetDepthStencilStatePlatform(ezGALDepthStencilState* pDepthStencilState, ezUInt8 uiStencilRefValue)
{
  m_pDXContext->OMSetDepthStencilState(pDepthStencilState != nullptr ? static_cast<ezGALDepthStencilStateDX11*>(pDepthStencilState)->GetDXDepthStencilState() : nullptr, uiStencilRefValue );
}

void ezGALContextDX11::SetRasterizerStatePlatform(ezGALRasterizerState* pRasterizerState)
{
  m_pDXContext->RSSetState(pRasterizerState != nullptr ? static_cast<ezGALRasterizerStateDX11*>(pRasterizerState)->GetDXRasterizerState() : nullptr);
}

void ezGALContextDX11::SetViewportPlatform(float fX, float fY, float fWidth, float fHeight, float fMinDepth, float fMaxDepth)
{
  D3D11_VIEWPORT Viewport;
  Viewport.TopLeftX = fX; Viewport.TopLeftY = fY;
  Viewport.Width = fWidth; Viewport.Height = fHeight;
  Viewport.MinDepth = fMinDepth; Viewport.MaxDepth = fMaxDepth;

  m_pDXContext->RSSetViewports(1, &Viewport);
}

void ezGALContextDX11::SetScissorRectPlatform(ezUInt32 uiX, ezUInt32 uiY, ezUInt32 uiWidth, ezUInt32 uiHeight)
{
  D3D11_RECT ScissorRect;
  ScissorRect.left = uiX; ScissorRect.top = uiY;
  ScissorRect.right = uiX + uiWidth; ScissorRect.bottom = uiY + uiHeight;

  m_pDXContext->RSSetScissorRects(1, &ScissorRect);
}

void ezGALContextDX11::SetStreamOutBufferPlatform(ezUInt32 uiSlot, ezGALBuffer* pBuffer, ezUInt32 uiOffset)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

// Fence & Query functions

void ezGALContextDX11::InsertFencePlatform(ezGALFence* pFence)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

bool ezGALContextDX11::IsFenceReachedPlatform(ezGALFence* pFence)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
  return false;
}

void ezGALContextDX11::BeginQueryPlatform(ezGALQuery* pQuery)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

void ezGALContextDX11::EndQueryPlatform(ezGALQuery* pQuery)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

// Resource update functions

void ezGALContextDX11::CopyBufferPlatform(ezGALBuffer* pDestination, ezGALBuffer* pSource)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

void ezGALContextDX11::CopyBufferRegionPlatform(ezGALBuffer* pDestination, ezUInt32 uiDestOffset, ezGALBuffer* pSource, ezUInt32 uiSourceOffset, ezUInt32 uiByteCount)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

void ezGALContextDX11::UpdateBufferPlatform(ezGALBuffer* pDestination, ezUInt32 uiDestOffset, const void* pSourceData, ezUInt32 uiByteCount)
{
  ezGALBufferDX11* pDXBuffer = static_cast<ezGALBufferDX11*>(pDestination);

  if (pDXBuffer->GetDescription().m_BufferType == ezGALBufferType::ConstantBuffer)
  {
    EZ_ASSERT_DEV(uiDestOffset == 0 && uiByteCount == pDXBuffer->GetSize(), "Constant buffers can't be updated partially (and we don't check for DX11.1)!");

    D3D11_MAPPED_SUBRESOURCE MapResult;
    m_pDXContext->Map(pDXBuffer->GetDXBuffer(), 0, D3D11_MAP_WRITE_DISCARD, 0, &MapResult);

    memcpy(MapResult.pData, pSourceData, uiByteCount);

    m_pDXContext->Unmap(pDXBuffer->GetDXBuffer(), 0);
  }
  else
  {
    ezLog::Warning("UpdateBuffer currently only for constant buffers implemented!");
    EZ_ASSERT_NOT_IMPLEMENTED;
  }
}

void ezGALContextDX11::CopyTexturePlatform(ezGALTexture* pDestination, ezGALTexture* pSource)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

void ezGALContextDX11::CopyTextureRegionPlatform(ezGALTexture* pDestination, const ezGALTextureSubresource& DestinationSubResource, const ezVec3U32& DestinationPoint, ezGALTexture* pSource, const ezGALTextureSubresource& SourceSubResource, const ezBoundingBoxu32& Box)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

void ezGALContextDX11::UpdateTexturePlatform(ezGALTexture* pDestination, const ezGALTextureSubresource& DestinationSubResource, const ezBoundingBoxu32& DestinationBox, const void* pSourceData, ezUInt32 uiSourceRowPitch, ezUInt32 uiSourceDepthPitch)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

void ezGALContextDX11::ResolveTexturePlatform(ezGALTexture* pDestination, const ezGALTextureSubresource& DestinationSubResource, ezGALTexture* pSource, const ezGALTextureSubresource& SourceSubResource)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

void ezGALContextDX11::ReadbackTexturePlatform(ezGALTexture* pTexture)
{
  ezGALTextureDX11* pDXTexture = static_cast<ezGALTextureDX11*>(pTexture);

  // MSAA textures (e.g. backbuffers) need to be converted to non MSAA versions
  const bool bMSAASourceTexture = pDXTexture->GetDescription().m_SampleCount != ezGALMSAASampleCount::None;

  EZ_ASSERT_DEV(pDXTexture->GetDXStagingTexture() != nullptr, "No staging resource available for read-back");
  EZ_ASSERT_DEV(pDXTexture->GetDXTexture() != nullptr, "Texture object is invalid");

  if (bMSAASourceTexture)
  {
    /// \todo Other mip levels etc?
    m_pDXContext->ResolveSubresource(pDXTexture->GetDXStagingTexture(), 0, pDXTexture->GetDXTexture(), 0, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
  }
  else
  {
    m_pDXContext->CopyResource(pDXTexture->GetDXStagingTexture(), pDXTexture->GetDXTexture());
  }

}

void ezGALContextDX11::CopyTextureReadbackResultPlatform(ezGALTexture* pTexture, const ezArrayPtr<ezGALSystemMemoryDescription>* pData)
{
  ezGALTextureDX11* pDXTexture = static_cast<ezGALTextureDX11*>(pTexture);

  EZ_ASSERT_DEV(pDXTexture->GetDXStagingTexture() != nullptr, "No staging resource available for read-back");

  D3D11_MAPPED_SUBRESOURCE Mapped;
  if(SUCCEEDED(m_pDXContext->Map(pDXTexture->GetDXStagingTexture(), 0, D3D11_MAP_READ, 0, &Mapped)))
  {
    if (Mapped.RowPitch == (*pData)[0].m_uiRowPitch)
    {
      const ezUInt32 uiMemorySize = ezGALResourceFormat::GetBitsPerElement(pDXTexture->GetDescription().m_Format) * pDXTexture->GetDescription().m_uiWidth * pDXTexture->GetDescription().m_uiHeight / 8;
      memcpy((*pData)[0].m_pData, Mapped.pData, uiMemorySize);
    }
    else
    {
      // Copy row by row
      for (ezUInt32 y = 0; y < pDXTexture->GetDescription().m_uiHeight; ++y)
      {
        const void* pSource = ezMemoryUtils::AddByteOffset(Mapped.pData, y * Mapped.RowPitch);
        void* pDest = ezMemoryUtils::AddByteOffset((*pData)[0].m_pData, y * (*pData)[0].m_uiRowPitch);

        memcpy(pDest, pSource, ezGALResourceFormat::GetBitsPerElement(pDXTexture->GetDescription().m_Format) * pDXTexture->GetDescription().m_uiWidth / 8);
      }
    }

    m_pDXContext->Unmap(pDXTexture->GetDXStagingTexture(), 0);
  }
}

// Debug helper functions

void ezGALContextDX11::PushMarkerPlatform(const char* szMarker)
{
  if (m_pDXAnnotation != nullptr)
  {
    ezStringWChar wsMarker(szMarker);
    m_pDXAnnotation->BeginEvent(wsMarker.GetData());
  }
}

void ezGALContextDX11::PopMarkerPlatform()
{
  if (m_pDXAnnotation != nullptr)
  {
    m_pDXAnnotation->EndEvent();
  }
}

void ezGALContextDX11::InsertEventMarkerPlatform(const char* szMarker)
{
  if (m_pDXAnnotation != nullptr)
  {
    ezStringWChar wsMarker(szMarker);
    m_pDXAnnotation->SetMarker(wsMarker.GetData());
  }
}



EZ_STATICLINK_FILE(RendererDX11, RendererDX11_Context_Implementation_ContextDX11);

