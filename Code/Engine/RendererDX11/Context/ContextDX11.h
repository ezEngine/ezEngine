
#pragma once

#include <RendererDX11/Basics.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Context/Context.h>
#include <Foundation/Basics/Types/Bitflags.h>

namespace ezGALDX11
{
  EZ_DECLARE_FLAGS(ezUInt16, DeferredStateChanged, VertexBuffer, ConstantBuffer, ShaderResourceView, SamplerState);
}


struct ID3D11DeviceContext;
struct ID3D11RenderTargetView;
struct ID3D11DepthStencilView;
struct ID3D11Buffer;
struct ID3D11ShaderResourceView;
struct ID3D11SamplerState;

/// \brief The DX11 implementation of the graphics context.
class EZ_RENDERERDX11_DLL ezGALContextDX11 : public ezGALContext
{
public:

  EZ_FORCE_INLINE ID3D11DeviceContext* GetDXContext() const;


protected:

  friend class ezGALDeviceDX11;
  friend class ezMemoryUtils;

  ezGALContextDX11(ezGALDevice* pDevice, ID3D11DeviceContext* pDXContext);

  ~ezGALContextDX11();

  // Draw functions

  virtual void ClearPlatform(const ezColor& ClearColor, ezUInt32 uiRenderTargetClearMask, bool bClearDepth, bool bClearStencil, float fDepthClear, ezUInt8 uiStencilClear) EZ_OVERRIDE;

  virtual void DrawPlatform(ezUInt32 uiVertexCount) EZ_OVERRIDE;

  virtual void DrawIndexedPlatform(ezUInt32 uiIndexCount, ezUInt32 uiStartIndex) EZ_OVERRIDE;

  virtual void DrawIndexedInstancedPlatform(ezUInt32 uiIndexCountPerInstance, ezUInt32 uiInstanceCount, ezUInt32 uiStartIndex) EZ_OVERRIDE;

  virtual void DrawIndexedInstancedIndirectPlatform(ezGALBuffer* pIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes) EZ_OVERRIDE;

  virtual void DrawInstancedPlatform(ezUInt32 uiVertexCountPerInstance, ezUInt32 uiInstanceCount) EZ_OVERRIDE;

  virtual void DrawInstancedIndirectPlatform(ezGALBuffer* pIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes) EZ_OVERRIDE;

  virtual void DrawAutoPlatform() EZ_OVERRIDE;

  virtual void BeginStreamOutPlatform() EZ_OVERRIDE;

  virtual void EndStreamOutPlatform() EZ_OVERRIDE;

  // Dispatch

  virtual void DispatchPlatform(ezUInt32 uiThreadGroupCountX, ezUInt32 uiThreadGroupCountY, ezUInt32 uiThreadGroupCountZ) EZ_OVERRIDE;

  virtual void DispatchIndirectPlatform(ezGALBuffer* pIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes) EZ_OVERRIDE;


  // State setting functions

  virtual void SetShaderPlatform(ezGALShader* pShader) EZ_OVERRIDE;

  virtual void SetIndexBufferPlatform(ezGALBuffer* pIndexBuffer) EZ_OVERRIDE;

  virtual void SetVertexBufferPlatform(ezUInt32 uiSlot, ezGALBuffer* pVertexBuffer) EZ_OVERRIDE;

  virtual void SetVertexDeclarationPlatform(ezGALVertexDeclaration* pVertexDeclaration) EZ_OVERRIDE;

  virtual void SetPrimitiveTopologyPlatform(ezGALPrimitiveTopology::Enum Topology) EZ_OVERRIDE;

  virtual void SetConstantBufferPlatform(ezUInt32 uiSlot, ezGALBuffer* pBuffer) EZ_OVERRIDE;

  virtual void SetSamplerStatePlatform(ezGALShaderStage::Enum Stage, ezUInt32 uiSlot, ezGALSamplerState* pSamplerState) EZ_OVERRIDE;

  virtual void SetResourceViewPlatform(ezGALShaderStage::Enum Stage, ezUInt32 uiSlot, ezGALResourceView* pResourceView) EZ_OVERRIDE;

  virtual void SetRenderTargetConfigPlatform(ezGALRenderTargetConfig* pRenderTargetConfig) EZ_OVERRIDE;

  virtual void SetUnorderedAccessViewPlatform(ezUInt32 uiSlot, ezGALResourceView* pResourceView) EZ_OVERRIDE;

  virtual void SetBlendStatePlatform(ezGALBlendState* pBlendState) EZ_OVERRIDE;

  virtual void SetDepthStencilStatePlatform(ezGALDepthStencilState* pDepthStencilState) EZ_OVERRIDE;

  virtual void SetRasterizerStatePlatform(ezGALRasterizerState* pRasterizerState) EZ_OVERRIDE;

  virtual void SetViewportPlatform(float fX, float fY, float fWidth, float fHeight, float fMinDepth, float fMaxDepth) EZ_OVERRIDE;

  virtual void SetScissorRectPlatform(ezUInt32 uiX, ezUInt32 uiY, ezUInt32 uiWidth, ezUInt32 uiHeight) EZ_OVERRIDE;

  virtual void SetStreamOutBufferPlatform(ezUInt32 uiSlot, ezGALBuffer* pBuffer, ezUInt32 uiOffset) EZ_OVERRIDE;

  // Fence & Query functions

  virtual void InsertFencePlatform(ezGALFence* pFence) EZ_OVERRIDE;

  virtual bool IsFenceReachedPlatform(ezGALFence* pFence) EZ_OVERRIDE;

  virtual void BeginQueryPlatform(ezGALQuery* pQuery) EZ_OVERRIDE;

  virtual void EndQueryPlatform(ezGALQuery* pQuery) EZ_OVERRIDE;

  // Resource update functions

  virtual void CopyBufferPlatform(ezGALBuffer* pDestination, ezGALBuffer* pSource) EZ_OVERRIDE;

  virtual void CopyBufferRegionPlatform(ezGALBuffer* pDestination, ezUInt32 uiDestOffset, ezGALBuffer* pSource, ezUInt32 uiSourceOffset, ezUInt32 uiByteCount) EZ_OVERRIDE;

  virtual void UpdateBufferPlatform(ezGALBuffer* pDestination, ezUInt32 uiDestOffset, const void* pSourceData, ezUInt32 uiByteCount) EZ_OVERRIDE;

  virtual void CopyTexturePlatform(ezGALTexture* pDestination, ezGALTexture* pSource) EZ_OVERRIDE;

  virtual void CopyTextureRegionPlatform(ezGALTexture* pDestination, const ezGALTextureSubresource& DestinationSubResource, const ezVec3U32& DestinationPoint, ezGALTexture* pSource, const ezGALTextureSubresource& SourceSubResource, const ezBoundingBoxu32& Box) EZ_OVERRIDE;

  virtual void UpdateTexturePlatform(ezGALTexture* pDestination, const ezGALTextureSubresource& DestinationSubResource, const ezBoundingBoxu32& DestinationBox, const void* pSourceData, ezUInt32 uiSourceRowPitch, ezUInt32 uiSourceDepthPitch) EZ_OVERRIDE;

  virtual void ResolveTexturePlatform(ezGALTexture* pDestination, const ezGALTextureSubresource& DestinationSubResource, ezGALTexture* pSource, const ezGALTextureSubresource& SourceSubResource) EZ_OVERRIDE;

  virtual void ReadbackTexturePlatform(ezGALTexture* pTexture) EZ_OVERRIDE;

  virtual void CopyTextureReadbackResultPlatform(ezGALTexture* pTexture, const ezArrayPtr<ezGALSystemMemoryDescription>* pData) EZ_OVERRIDE;

  // Debug helper functions

  virtual void PushMarkerPlatform(const char* Marker) EZ_OVERRIDE;

  virtual void PopMarkerPlatform() EZ_OVERRIDE;

  virtual void InsertEventMarkerPlatform(const char* Marker) EZ_OVERRIDE;


  void FlushDeferredStateChanges();


  ID3D11DeviceContext* m_pDXContext;


  // Bound objects for deferred state flushes
  ID3D11RenderTargetView* m_pBoundRenderTargets[EZ_GAL_MAX_RENDERTARGET_COUNT];

  ID3D11DepthStencilView* m_pBoundDepthStencilTarget;

  ID3D11Buffer* m_pBoundVertexBuffers[EZ_GAL_MAX_VERTEX_BUFFER_COUNT];

  ezUInt32 m_VertexBufferStrides[EZ_GAL_MAX_VERTEX_BUFFER_COUNT];

  ezUInt32 m_VertexBufferOffsets[EZ_GAL_MAX_VERTEX_BUFFER_COUNT];

  ID3D11Buffer* m_pBoundConstantBuffers[EZ_GAL_MAX_CONSTANT_BUFFER_COUNT];

  ezUInt32 m_uiBoundRenderTargetCount;

  ID3D11ShaderResourceView* m_pBoundShaderResourceViews[ezGALShaderStage::ENUM_COUNT][EZ_GAL_MAX_SHADER_RESOURCE_VIEW_COUNT];

  ID3D11SamplerState* m_pBoundSamplerStates[ezGALShaderStage::ENUM_COUNT][EZ_GAL_MAX_SHADER_RESOURCE_VIEW_COUNT];

  ezBitflags<ezGALDX11::DeferredStateChanged> m_DeferredStateChanged;

};

#include <RendererDX11/Context/Implementation/ContextDX11_inl.h>
