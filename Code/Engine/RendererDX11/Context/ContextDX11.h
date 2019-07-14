
#pragma once

#include <RendererDX11/RendererDX11DLL.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Context/Context.h>
#include <Foundation/Types/Bitflags.h>

struct ID3D11DeviceChild;
struct ID3D11DeviceContext;
struct ID3DUserDefinedAnnotation;
struct ID3D11RenderTargetView;
struct ID3D11DepthStencilView;
struct ID3D11Buffer;
struct ID3D11ShaderResourceView;
struct ID3D11UnorderedAccessView;
struct ID3D11SamplerState;
struct ID3D11Query;

/// \brief The DX11 implementation of the graphics context.
class EZ_RENDERERDX11_DLL ezGALContextDX11 : public ezGALContext
{
public:

  EZ_ALWAYS_INLINE ID3D11DeviceContext* GetDXContext() const;

protected:

  friend class ezGALDeviceDX11;
  friend class ezMemoryUtils;

  ezGALContextDX11(ezGALDevice* pDevice, ID3D11DeviceContext* pDXContext);

  ~ezGALContextDX11();

  // Draw functions

  virtual void ClearPlatform(const ezColor& ClearColor, ezUInt32 uiRenderTargetClearMask, bool bClearDepth, bool bClearStencil, float fDepthClear, ezUInt8 uiStencilClear) override;

  virtual void ClearUnorderedAccessViewPlatform(const ezGALUnorderedAccessView* pUnorderedAccessView, ezVec4 clearValues) override;

  virtual void ClearUnorderedAccessViewPlatform(const ezGALUnorderedAccessView* pUnorderedAccessView, ezVec4U32 clearValues) override;

  virtual void DrawPlatform(ezUInt32 uiVertexCount, ezUInt32 uiStartVertex) override;

  virtual void DrawIndexedPlatform(ezUInt32 uiIndexCount, ezUInt32 uiStartIndex) override;

  virtual void DrawIndexedInstancedPlatform(ezUInt32 uiIndexCountPerInstance, ezUInt32 uiInstanceCount, ezUInt32 uiStartIndex) override;

  virtual void DrawIndexedInstancedIndirectPlatform(const ezGALBuffer* pIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes) override;

  virtual void DrawInstancedPlatform(ezUInt32 uiVertexCountPerInstance, ezUInt32 uiInstanceCount, ezUInt32 uiStartVertex) override;

  virtual void DrawInstancedIndirectPlatform(const ezGALBuffer* pIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes) override;

  virtual void DrawAutoPlatform() override;

  virtual void BeginStreamOutPlatform() override;

  virtual void EndStreamOutPlatform() override;

  // Dispatch

  virtual void DispatchPlatform(ezUInt32 uiThreadGroupCountX, ezUInt32 uiThreadGroupCountY, ezUInt32 uiThreadGroupCountZ) override;

  virtual void DispatchIndirectPlatform(const ezGALBuffer* pIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes) override;


  // State setting functions

  virtual void SetShaderPlatform(const ezGALShader* pShader) override;

  virtual void SetIndexBufferPlatform(const ezGALBuffer* pIndexBuffer) override;

  virtual void SetVertexBufferPlatform(ezUInt32 uiSlot, const ezGALBuffer* pVertexBuffer) override;

  virtual void SetVertexDeclarationPlatform(const ezGALVertexDeclaration* pVertexDeclaration) override;

  virtual void SetPrimitiveTopologyPlatform(ezGALPrimitiveTopology::Enum Topology) override;

  virtual void SetConstantBufferPlatform(ezUInt32 uiSlot, const ezGALBuffer* pBuffer) override;

  virtual void SetSamplerStatePlatform(ezGALShaderStage::Enum Stage, ezUInt32 uiSlot, const ezGALSamplerState* pSamplerState) override;

  virtual void SetResourceViewPlatform(ezGALShaderStage::Enum Stage, ezUInt32 uiSlot, const ezGALResourceView* pResourceView) override;

  virtual void SetRenderTargetSetupPlatform(ezArrayPtr<const ezGALRenderTargetView*> pRenderTargetViews, const ezGALRenderTargetView* pDepthStencilView) override;

  virtual void SetUnorderedAccessViewPlatform(ezUInt32 uiSlot, const ezGALUnorderedAccessView* pUnorderedAccessView) override;

  virtual void SetBlendStatePlatform(const ezGALBlendState* pBlendState, const ezColor& BlendFactor, ezUInt32 uiSampleMask) override;

  virtual void SetDepthStencilStatePlatform(const ezGALDepthStencilState* pDepthStencilState, ezUInt8 uiStencilRefValue) override;

  virtual void SetRasterizerStatePlatform(const ezGALRasterizerState* pRasterizerState) override;

  virtual void SetViewportPlatform(const ezRectFloat& rect, float fMinDepth, float fMaxDepth) override;

  virtual void SetScissorRectPlatform(const ezRectU32& rect) override;

  virtual void SetStreamOutBufferPlatform(ezUInt32 uiSlot, const ezGALBuffer* pBuffer, ezUInt32 uiOffset) override;

  // Fence & Query functions

  virtual void InsertFencePlatform(const ezGALFence* pFence) override;

  virtual bool IsFenceReachedPlatform(const ezGALFence* pFence) override;

  virtual void WaitForFencePlatform(const ezGALFence* pFence) override;

  virtual void BeginQueryPlatform(const ezGALQuery* pQuery) override;

  virtual void EndQueryPlatform(const ezGALQuery* pQuery) override;

  virtual ezResult GetQueryResultPlatform(const ezGALQuery* pQuery, ezUInt64& uiQueryResult) override;

  // Timestamp functions

  virtual void InsertTimestampPlatform(ezGALTimestampHandle hTimestamp) override;

  // Resource update functions

  virtual void CopyBufferPlatform(const ezGALBuffer* pDestination, const ezGALBuffer* pSource) override;

  virtual void CopyBufferRegionPlatform(const ezGALBuffer* pDestination, ezUInt32 uiDestOffset, const ezGALBuffer* pSource, ezUInt32 uiSourceOffset, ezUInt32 uiByteCount) override;

  virtual void UpdateBufferPlatform(const ezGALBuffer* pDestination, ezUInt32 uiDestOffset, ezArrayPtr<const ezUInt8> pSourceData, ezGALUpdateMode::Enum updateMode) override;

  virtual void CopyTexturePlatform(const ezGALTexture* pDestination, const ezGALTexture* pSource) override;

  virtual void CopyTextureRegionPlatform(const ezGALTexture* pDestination, const ezGALTextureSubresource& DestinationSubResource, const ezVec3U32& DestinationPoint, const ezGALTexture* pSource, const ezGALTextureSubresource& SourceSubResource, const ezBoundingBoxu32& Box) override;

  virtual void UpdateTexturePlatform(const ezGALTexture* pDestination, const ezGALTextureSubresource& DestinationSubResource, const ezBoundingBoxu32& DestinationBox, const ezGALSystemMemoryDescription& pSourceData) override;

  virtual void ResolveTexturePlatform(const ezGALTexture* pDestination, const ezGALTextureSubresource& DestinationSubResource, const ezGALTexture* pSource, const ezGALTextureSubresource& SourceSubResource) override;

  virtual void ReadbackTexturePlatform(const ezGALTexture* pTexture) override;

  virtual void CopyTextureReadbackResultPlatform(const ezGALTexture* pTexture, const ezArrayPtr<ezGALSystemMemoryDescription>* pData) override;

  virtual void GenerateMipMapsPlatform(const ezGALResourceView* pResourceView) override;

  // Misc

  virtual void FlushPlatform() override;

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
  ezGAL::ModifiedRange m_BoundVertexBuffersRange;

  ezUInt32 m_VertexBufferStrides[EZ_GAL_MAX_VERTEX_BUFFER_COUNT];
  ezUInt32 m_VertexBufferOffsets[EZ_GAL_MAX_VERTEX_BUFFER_COUNT];

  ID3D11Buffer* m_pBoundConstantBuffers[EZ_GAL_MAX_CONSTANT_BUFFER_COUNT];
  ezGAL::ModifiedRange m_BoundConstantBuffersRange[ezGALShaderStage::ENUM_COUNT];

  ezUInt32 m_uiBoundRenderTargetCount;

  ezHybridArray<ID3D11ShaderResourceView*, 16> m_pBoundShaderResourceViews[ezGALShaderStage::ENUM_COUNT];
  ezGAL::ModifiedRange m_BoundShaderResourceViewsRange[ezGALShaderStage::ENUM_COUNT];

  ezHybridArray<ID3D11UnorderedAccessView*, 16> m_pBoundUnoderedAccessViews;
  ezGAL::ModifiedRange m_pBoundUnoderedAccessViewsRange;

  ID3D11SamplerState* m_pBoundSamplerStates[ezGALShaderStage::ENUM_COUNT][EZ_GAL_MAX_SAMPLER_COUNT];
  ezGAL::ModifiedRange m_BoundSamplerStatesRange[ezGALShaderStage::ENUM_COUNT];

  ID3D11DeviceChild* m_pBoundShaders[ezGALShaderStage::ENUM_COUNT];
};

#include <RendererDX11/Context/Implementation/ContextDX11_inl.h>
