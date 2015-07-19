
#pragma once

#include <RendererDX11/Basics.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Context/Context.h>
#include <Foundation/Types/Bitflags.h>

namespace ezGALDX11
{
  EZ_DECLARE_FLAGS(ezUInt16, DeferredStateChanged, VertexBuffer, ConstantBuffer, ShaderResourceView, SamplerState);
}


struct ID3D11DeviceContext;
struct ID3DUserDefinedAnnotation;
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

  virtual void ClearPlatform(const ezColor& ClearColor, ezUInt32 uiRenderTargetClearMask, bool bClearDepth, bool bClearStencil, float fDepthClear, ezUInt8 uiStencilClear) override;

  virtual void DrawPlatform(ezUInt32 uiVertexCount, ezUInt32 uiStartVertex) override;

  virtual void DrawIndexedPlatform(ezUInt32 uiIndexCount, ezUInt32 uiStartIndex) override;

  virtual void DrawIndexedInstancedPlatform(ezUInt32 uiIndexCountPerInstance, ezUInt32 uiInstanceCount, ezUInt32 uiStartIndex) override;

  virtual void DrawIndexedInstancedIndirectPlatform(ezGALBuffer* pIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes) override;

  virtual void DrawInstancedPlatform(ezUInt32 uiVertexCountPerInstance, ezUInt32 uiInstanceCount, ezUInt32 uiStartVertex) override;

  virtual void DrawInstancedIndirectPlatform(ezGALBuffer* pIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes) override;

  virtual void DrawAutoPlatform() override;

  virtual void BeginStreamOutPlatform() override;

  virtual void EndStreamOutPlatform() override;

  // Dispatch

  virtual void DispatchPlatform(ezUInt32 uiThreadGroupCountX, ezUInt32 uiThreadGroupCountY, ezUInt32 uiThreadGroupCountZ) override;

  virtual void DispatchIndirectPlatform(ezGALBuffer* pIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes) override;


  // State setting functions

  virtual void SetShaderPlatform(ezGALShader* pShader) override;

  virtual void SetIndexBufferPlatform(ezGALBuffer* pIndexBuffer) override;

  virtual void SetVertexBufferPlatform(ezUInt32 uiSlot, ezGALBuffer* pVertexBuffer) override;

  virtual void SetVertexDeclarationPlatform(ezGALVertexDeclaration* pVertexDeclaration) override;

  virtual void SetPrimitiveTopologyPlatform(ezGALPrimitiveTopology::Enum Topology) override;

  virtual void SetConstantBufferPlatform(ezUInt32 uiSlot, ezGALBuffer* pBuffer) override;

  virtual void SetSamplerStatePlatform(ezGALShaderStage::Enum Stage, ezUInt32 uiSlot, ezGALSamplerState* pSamplerState) override;

  virtual void SetResourceViewPlatform(ezGALShaderStage::Enum Stage, ezUInt32 uiSlot, ezGALResourceView* pResourceView) override;

  virtual void SetRenderTargetSetupPlatform( ezGALRenderTargetView** ppRenderTargetViews, ezUInt32 uiRenderTargetCount, ezGALRenderTargetView* pDepthStencilView ) override;

  virtual void SetUnorderedAccessViewPlatform(ezUInt32 uiSlot, ezGALResourceView* pResourceView) override;

  virtual void SetBlendStatePlatform(ezGALBlendState* pBlendState, const ezColor& BlendFactor, ezUInt32 uiSampleMask) override;

  virtual void SetDepthStencilStatePlatform(ezGALDepthStencilState* pDepthStencilState, ezUInt8 uiStencilRefValue) override;

  virtual void SetRasterizerStatePlatform(ezGALRasterizerState* pRasterizerState) override;

  virtual void SetViewportPlatform(float fX, float fY, float fWidth, float fHeight, float fMinDepth, float fMaxDepth) override;

  virtual void SetScissorRectPlatform(ezUInt32 uiX, ezUInt32 uiY, ezUInt32 uiWidth, ezUInt32 uiHeight) override;

  virtual void SetStreamOutBufferPlatform(ezUInt32 uiSlot, ezGALBuffer* pBuffer, ezUInt32 uiOffset) override;

  // Fence & Query functions

  virtual void InsertFencePlatform(ezGALFence* pFence) override;

  virtual bool IsFenceReachedPlatform(ezGALFence* pFence) override;

  virtual void BeginQueryPlatform(ezGALQuery* pQuery) override;

  virtual void EndQueryPlatform(ezGALQuery* pQuery) override;

  // Resource update functions

  virtual void CopyBufferPlatform(ezGALBuffer* pDestination, ezGALBuffer* pSource) override;

  virtual void CopyBufferRegionPlatform(ezGALBuffer* pDestination, ezUInt32 uiDestOffset, ezGALBuffer* pSource, ezUInt32 uiSourceOffset, ezUInt32 uiByteCount) override;

  virtual void UpdateBufferPlatform(ezGALBuffer* pDestination, ezUInt32 uiDestOffset, const void* pSourceData, ezUInt32 uiByteCount) override;

  virtual void CopyTexturePlatform(ezGALTexture* pDestination, ezGALTexture* pSource) override;

  virtual void CopyTextureRegionPlatform(ezGALTexture* pDestination, const ezGALTextureSubresource& DestinationSubResource, const ezVec3U32& DestinationPoint, ezGALTexture* pSource, const ezGALTextureSubresource& SourceSubResource, const ezBoundingBoxu32& Box) override;

  virtual void UpdateTexturePlatform(ezGALTexture* pDestination, const ezGALTextureSubresource& DestinationSubResource, const ezBoundingBoxu32& DestinationBox, const void* pSourceData, ezUInt32 uiSourceRowPitch, ezUInt32 uiSourceDepthPitch) override;

  virtual void ResolveTexturePlatform(ezGALTexture* pDestination, const ezGALTextureSubresource& DestinationSubResource, ezGALTexture* pSource, const ezGALTextureSubresource& SourceSubResource) override;

  virtual void ReadbackTexturePlatform(ezGALTexture* pTexture) override;

  virtual void CopyTextureReadbackResultPlatform(ezGALTexture* pTexture, const ezArrayPtr<ezGALSystemMemoryDescription>* pData) override;

  // Debug helper functions

  virtual void PushMarkerPlatform(const char* szMarker) override;

  virtual void PopMarkerPlatform() override;

  virtual void InsertEventMarkerPlatform(const char* szMarker) override;


  void FlushDeferredStateChanges();


  ID3D11DeviceContext* m_pDXContext;
  ID3DUserDefinedAnnotation* m_pDXAnnotation;

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
