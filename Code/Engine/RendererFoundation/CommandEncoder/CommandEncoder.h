
#pragma once

#include <Foundation/Math/Color.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <RendererFoundation/CommandEncoder/ContextState.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class EZ_RENDERERFOUNDATION_DLL ezGALCommandEncoder
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezGALCommandEncoder);

public:


  // Dispatch

  void Dispatch(ezUInt32 uiThreadGroupCountX, ezUInt32 uiThreadGroupCountY, ezUInt32 uiThreadGroupCountZ);

  void DispatchIndirect(ezGALBufferHandle hIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes);


  // State setting functions

  void SetShader(ezGALShaderHandle hShader);

  void SetIndexBuffer(ezGALBufferHandle hIndexBuffer);

  void SetVertexBuffer(ezUInt32 uiSlot, ezGALBufferHandle hVertexBuffer);

  void SetVertexDeclaration(ezGALVertexDeclarationHandle hVertexDeclaration);

  ezGALPrimitiveTopology::Enum GetPrimitiveTopology() const { return m_State.m_Topology; }

  void SetPrimitiveTopology(ezGALPrimitiveTopology::Enum Topology);

  void SetConstantBuffer(ezUInt32 uiSlot, ezGALBufferHandle hBuffer);

  void SetSamplerState(ezGALShaderStage::Enum Stage, ezUInt32 uiSlot, ezGALSamplerStateHandle hSamplerState);

  void SetResourceView(ezGALShaderStage::Enum Stage, ezUInt32 uiSlot, ezGALResourceViewHandle hResourceView);

  void SetRenderTargetSetup(const ezGALRenderTargetSetup& RenderTargetSetup);

  void SetUnorderedAccessView(ezUInt32 uiSlot, ezGALUnorderedAccessViewHandle hUnorderedAccessView);

  void SetBlendState(ezGALBlendStateHandle hBlendState, const ezColor& BlendFactor = ezColor::White, ezUInt32 uiSampleMask = 0xFFFFFFFFu);

  void SetDepthStencilState(ezGALDepthStencilStateHandle hDepthStencilState, ezUInt8 uiStencilRefValue = 0xFFu);

  void SetRasterizerState(ezGALRasterizerStateHandle hRasterizerState);

  void SetViewport(const ezRectFloat& rect, float fMinDepth = 0.0f, float fMaxDepth = 1.0f);

  void SetScissorRect(const ezRectU32& rect);

  void SetStreamOutBuffer(ezUInt32 uiSlot, ezGALBufferHandle hBuffer, ezUInt32 uiOffset);

  // Fence & Query functions

  void InsertFence(ezGALFenceHandle hFence);

  bool IsFenceReached(ezGALFenceHandle hFence);

  void WaitForFence(ezGALFenceHandle hFence);

  void BeginQuery(ezGALQueryHandle hQuery);

  void EndQuery(ezGALQueryHandle hQuery);

  /// \return Success if retrieving the query succeeded.
  ezResult GetQueryResult(ezGALQueryHandle hQuery, ezUInt64& uiQueryResult);

  // Timestamp functions

  ezGALTimestampHandle InsertTimestamp();

  // Resource functions

  /// Clears an unordered access view with a float value.
  void ClearUnorderedAccessView(ezGALUnorderedAccessViewHandle hUnorderedAccessView, ezVec4 clearValues);

  /// Clears an unordered access view with an int value.
  void ClearUnorderedAccessView(ezGALUnorderedAccessViewHandle hUnorderedAccessView, ezVec4U32 clearValues);

  void CopyBuffer(ezGALBufferHandle hDest, ezGALBufferHandle hSource);

  void CopyBufferRegion(ezGALBufferHandle hDest, ezUInt32 uiDestOffset, ezGALBufferHandle hSource, ezUInt32 uiSourceOffset, ezUInt32 uiByteCount);

  void UpdateBuffer(ezGALBufferHandle hDest, ezUInt32 uiDestOffset, ezArrayPtr<const ezUInt8> pSourceData,
    ezGALUpdateMode::Enum updateMode = ezGALUpdateMode::Discard);

  void CopyTexture(ezGALTextureHandle hDest, ezGALTextureHandle hSource);

  void CopyTextureRegion(ezGALTextureHandle hDest, const ezGALTextureSubresource& DestinationSubResource, const ezVec3U32& DestinationPoint,
    ezGALTextureHandle hSource, const ezGALTextureSubresource& SourceSubResource, const ezBoundingBoxu32& Box);

  void UpdateTexture(ezGALTextureHandle hDest, const ezGALTextureSubresource& DestinationSubResource, const ezBoundingBoxu32& DestinationBox,
    const ezGALSystemMemoryDescription& pSourceData);

  void ResolveTexture(ezGALTextureHandle hDest, const ezGALTextureSubresource& DestinationSubResource, ezGALTextureHandle hSource,
    const ezGALTextureSubresource& SourceSubResource);

  void ReadbackTexture(ezGALTextureHandle hTexture);

  void CopyTextureReadbackResult(ezGALTextureHandle hTexture, const ezArrayPtr<ezGALSystemMemoryDescription>* pData);

  void GenerateMipMaps(ezGALResourceViewHandle hResourceView);

  // Misc

  void Flush();

  // Debug helper functions

  void PushMarker(const char* Marker);

  void PopMarker();

  void InsertEventMarker(const char* Marker);

  virtual void ClearStatisticsCounters();

  EZ_ALWAYS_INLINE ezGALDevice& GetDevice() { return m_Device; }

protected:
  friend class ezGALDevice;

  ezGALCommandEncoder(ezGALDevice& device);
  virtual ~ezGALCommandEncoder();

  // Dispatch

  virtual void DispatchPlatform(ezUInt32 uiThreadGroupCountX, ezUInt32 uiThreadGroupCountY, ezUInt32 uiThreadGroupCountZ) = 0;

  virtual void DispatchIndirectPlatform(const ezGALBuffer* pIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes) = 0;


  // State setting functions

  virtual void SetShaderPlatform(const ezGALShader* pShader) = 0;

  virtual void SetIndexBufferPlatform(const ezGALBuffer* pIndexBuffer) = 0;

  virtual void SetVertexBufferPlatform(ezUInt32 uiSlot, const ezGALBuffer* pVertexBuffer) = 0;

  virtual void SetVertexDeclarationPlatform(const ezGALVertexDeclaration* pVertexDeclaration) = 0;

  virtual void SetPrimitiveTopologyPlatform(ezGALPrimitiveTopology::Enum Topology) = 0;

  virtual void SetConstantBufferPlatform(ezUInt32 uiSlot, const ezGALBuffer* pBuffer) = 0;

  virtual void SetSamplerStatePlatform(ezGALShaderStage::Enum Stage, ezUInt32 uiSlot, const ezGALSamplerState* pSamplerState) = 0;

  virtual void SetResourceViewPlatform(ezGALShaderStage::Enum Stage, ezUInt32 uiSlot, const ezGALResourceView* pResourceView) = 0;

  virtual void SetRenderTargetSetupPlatform(
    ezArrayPtr<const ezGALRenderTargetView*> pRenderTargetViews, const ezGALRenderTargetView* pDepthStencilView) = 0;

  virtual void SetUnorderedAccessViewPlatform(ezUInt32 uiSlot, const ezGALUnorderedAccessView* pUnorderedAccessView) = 0;

  virtual void SetBlendStatePlatform(const ezGALBlendState* pBlendState, const ezColor& BlendFactor, ezUInt32 uiSampleMask) = 0;

  virtual void SetDepthStencilStatePlatform(const ezGALDepthStencilState* pDepthStencilState, ezUInt8 uiStencilRefValue) = 0;

  virtual void SetRasterizerStatePlatform(const ezGALRasterizerState* pRasterizerState) = 0;

  virtual void SetViewportPlatform(const ezRectFloat& rect, float fMinDepth, float fMaxDepth) = 0;

  virtual void SetScissorRectPlatform(const ezRectU32& rect) = 0;

  virtual void SetStreamOutBufferPlatform(ezUInt32 uiSlot, const ezGALBuffer* pBuffer, ezUInt32 uiOffset) = 0;

  // Fence & Query functions

  virtual void InsertFencePlatform(const ezGALFence* pFence) = 0;

  virtual bool IsFenceReachedPlatform(const ezGALFence* pFence) = 0;

  virtual void WaitForFencePlatform(const ezGALFence* pFence) = 0;

  virtual void BeginQueryPlatform(const ezGALQuery* pQuery) = 0;

  virtual void EndQueryPlatform(const ezGALQuery* pQuery) = 0;

  virtual ezResult GetQueryResultPlatform(const ezGALQuery* pQuery, ezUInt64& uiQueryResult) = 0;

  // Timestamp functions

  virtual void InsertTimestampPlatform(ezGALTimestampHandle hTimestamp) = 0;

  // Resource update functions

  virtual void ClearUnorderedAccessViewPlatform(const ezGALUnorderedAccessView* pUnorderedAccessView, ezVec4 clearValues) = 0;

  virtual void ClearUnorderedAccessViewPlatform(const ezGALUnorderedAccessView* pUnorderedAccessView, ezVec4U32 clearValues) = 0;

  virtual void CopyBufferPlatform(const ezGALBuffer* pDestination, const ezGALBuffer* pSource) = 0;

  virtual void CopyBufferRegionPlatform(
    const ezGALBuffer* pDestination, ezUInt32 uiDestOffset, const ezGALBuffer* pSource, ezUInt32 uiSourceOffset, ezUInt32 uiByteCount) = 0;

  virtual void UpdateBufferPlatform(
    const ezGALBuffer* pDestination, ezUInt32 uiDestOffset, ezArrayPtr<const ezUInt8> pSourceData, ezGALUpdateMode::Enum updateMode) = 0;

  virtual void CopyTexturePlatform(const ezGALTexture* pDestination, const ezGALTexture* pSource) = 0;

  virtual void CopyTextureRegionPlatform(const ezGALTexture* pDestination, const ezGALTextureSubresource& DestinationSubResource,
    const ezVec3U32& DestinationPoint, const ezGALTexture* pSource, const ezGALTextureSubresource& SourceSubResource,
    const ezBoundingBoxu32& Box) = 0;

  virtual void UpdateTexturePlatform(const ezGALTexture* pDestination, const ezGALTextureSubresource& DestinationSubResource,
    const ezBoundingBoxu32& DestinationBox, const ezGALSystemMemoryDescription& pSourceData) = 0;

  virtual void ResolveTexturePlatform(const ezGALTexture* pDestination, const ezGALTextureSubresource& DestinationSubResource,
    const ezGALTexture* pSource, const ezGALTextureSubresource& SourceSubResource) = 0;

  virtual void ReadbackTexturePlatform(const ezGALTexture* pTexture) = 0;

  /// \todo add parameters for mip level & count selection?
  virtual void CopyTextureReadbackResultPlatform(const ezGALTexture* pTexture, const ezArrayPtr<ezGALSystemMemoryDescription>* pData) = 0;

  virtual void GenerateMipMapsPlatform(const ezGALResourceView* pResourceView) = 0;

  // Misc

  virtual void FlushPlatform() = 0;

  // Debug helper functions

  virtual void PushMarkerPlatform(const char* Marker) = 0;

  virtual void PopMarkerPlatform() = 0;

  virtual void InsertEventMarkerPlatform(const char* Marker) = 0;

  // Don't use light hearted ;)
  virtual void InvalidateState();

  // Returns whether a resource view has been unset for the given resource
  bool UnsetResourceViews(const ezGALResourceBase* pResource);
  // Returns whether a unordered access view has been unset for the given resource
  bool UnsetUnorderedAccessViews(const ezGALResourceBase* pResource);

  void AssertRenderingThread()
  {
    EZ_ASSERT_DEV(ezThreadUtils::IsMainThread(), "This function can only be executed on the main thread.");
  }

private:
  friend class ezMemoryUtils;

  
  void CountDispatchCall() { m_uiDispatchCalls++; }
  void CountStateChange() { m_uiStateChanges++; }
  void CountRedundantStateChange() { m_uiRedundantStateChanges++; }

  // Parent Device
  ezGALDevice& m_Device;

  // Used to track redundant state changes
  ezGALContextState m_State;

  // Statistic variables
  ezUInt32 m_uiDispatchCalls = 0;
  ezUInt32 m_uiStateChanges = 0;
  ezUInt32 m_uiRedundantStateChanges = 0;
};

class EZ_RENDERERFOUNDATION_DLL ezGALRenderCommandEncoder : public ezGALCommandEncoder
{
public:
  // Draw functions

  void Draw(ezUInt32 uiVertexCount, ezUInt32 uiStartVertex);
  void DrawIndexed(ezUInt32 uiIndexCount, ezUInt32 uiStartIndex);
  void DrawIndexedInstanced(ezUInt32 uiIndexCountPerInstance, ezUInt32 uiInstanceCount, ezUInt32 uiStartIndex);
  void DrawIndexedInstancedIndirect(ezGALBufferHandle hIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes);
  void DrawInstanced(ezUInt32 uiVertexCountPerInstance, ezUInt32 uiInstanceCount, ezUInt32 uiStartVertex);
  void DrawInstancedIndirect(ezGALBufferHandle hIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes);
  void DrawAuto();

  void BeginStreamOut();
  void EndStreamOut();

  virtual void ClearStatisticsCounters() override;

protected:
  friend class ezGALDevice;

  ezGALRenderCommandEncoder(ezGALDevice& device);
  virtual ~ezGALRenderCommandEncoder();

  // Draw functions

  virtual void DrawPlatform(ezUInt32 uiVertexCount, ezUInt32 uiStartVertex) = 0;
  virtual void DrawIndexedPlatform(ezUInt32 uiIndexCount, ezUInt32 uiStartIndex) = 0;
  virtual void DrawIndexedInstancedPlatform(ezUInt32 uiIndexCountPerInstance, ezUInt32 uiInstanceCount, ezUInt32 uiStartIndex) = 0;
  virtual void DrawIndexedInstancedIndirectPlatform(const ezGALBuffer* pIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes) = 0;
  virtual void DrawInstancedPlatform(ezUInt32 uiVertexCountPerInstance, ezUInt32 uiInstanceCount, ezUInt32 uiStartVertex) = 0;
  virtual void DrawInstancedIndirectPlatform(const ezGALBuffer* pIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes) = 0;
  virtual void DrawAutoPlatform() = 0;

  virtual void BeginStreamOutPlatform() = 0;
  virtual void EndStreamOutPlatform() = 0;

private:
  void CountDrawCall() { m_uiDrawCalls++; }

  // Statistic variables
  ezUInt32 m_uiDrawCalls = 0;
};

class EZ_RENDERERFOUNDATION_DLL ezGALComputeCommandEncoder : public ezGALCommandEncoder
{
};
