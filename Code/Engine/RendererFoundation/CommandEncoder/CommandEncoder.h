
#pragma once

#include <Foundation/Math/Color.h>
#include <Foundation/Math/Rect.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class EZ_RENDERERFOUNDATION_DLL ezGALCommandEncoder
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezGALCommandEncoder);

public:
  // State setting functions

  void SetShader(ezGALShaderHandle hShader);

  void SetConstantBuffer(ezUInt32 uiSlot, ezGALBufferHandle hBuffer);
  void SetSamplerState(ezGALShaderStage::Enum Stage, ezUInt32 uiSlot, ezGALSamplerStateHandle hSamplerState);
  void SetResourceView(ezGALShaderStage::Enum Stage, ezUInt32 uiSlot, ezGALResourceViewHandle hResourceView);
  void SetUnorderedAccessView(ezUInt32 uiSlot, ezGALUnorderedAccessViewHandle hUnorderedAccessView);

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
  void UpdateBuffer(ezGALBufferHandle hDest, ezUInt32 uiDestOffset, ezArrayPtr<const ezUInt8> pSourceData, ezGALUpdateMode::Enum updateMode = ezGALUpdateMode::Discard);

  void CopyTexture(ezGALTextureHandle hDest, ezGALTextureHandle hSource);
  void CopyTextureRegion(ezGALTextureHandle hDest, const ezGALTextureSubresource& DestinationSubResource, const ezVec3U32& DestinationPoint, ezGALTextureHandle hSource, const ezGALTextureSubresource& SourceSubResource, const ezBoundingBoxu32& Box);

  void UpdateTexture(ezGALTextureHandle hDest, const ezGALTextureSubresource& DestinationSubResource, const ezBoundingBoxu32& DestinationBox, const ezGALSystemMemoryDescription& pSourceData);

  void ResolveTexture(ezGALTextureHandle hDest, const ezGALTextureSubresource& DestinationSubResource, ezGALTextureHandle hSource, const ezGALTextureSubresource& SourceSubResource);

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

  // State setting functions

  virtual void SetShaderPlatform(const ezGALShader* pShader) = 0;

  virtual void SetConstantBufferPlatform(ezUInt32 uiSlot, const ezGALBuffer* pBuffer) = 0;
  virtual void SetSamplerStatePlatform(ezGALShaderStage::Enum Stage, ezUInt32 uiSlot, const ezGALSamplerState* pSamplerState) = 0;
  virtual void SetResourceViewPlatform(ezGALShaderStage::Enum Stage, ezUInt32 uiSlot, const ezGALResourceView* pResourceView) = 0;
  virtual void SetUnorderedAccessViewPlatform(ezUInt32 uiSlot, const ezGALUnorderedAccessView* pUnorderedAccessView) = 0;

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
  virtual void CopyBufferRegionPlatform(const ezGALBuffer* pDestination, ezUInt32 uiDestOffset, const ezGALBuffer* pSource, ezUInt32 uiSourceOffset, ezUInt32 uiByteCount) = 0;

  virtual void UpdateBufferPlatform(const ezGALBuffer* pDestination, ezUInt32 uiDestOffset, ezArrayPtr<const ezUInt8> pSourceData, ezGALUpdateMode::Enum updateMode) = 0;

  virtual void CopyTexturePlatform(const ezGALTexture* pDestination, const ezGALTexture* pSource) = 0;
  virtual void CopyTextureRegionPlatform(const ezGALTexture* pDestination, const ezGALTextureSubresource& DestinationSubResource, const ezVec3U32& DestinationPoint, const ezGALTexture* pSource, const ezGALTextureSubresource& SourceSubResource, const ezBoundingBoxu32& Box) = 0;

  virtual void UpdateTexturePlatform(const ezGALTexture* pDestination, const ezGALTextureSubresource& DestinationSubResource, const ezBoundingBoxu32& DestinationBox, const ezGALSystemMemoryDescription& pSourceData) = 0;

  virtual void ResolveTexturePlatform(const ezGALTexture* pDestination, const ezGALTextureSubresource& DestinationSubResource, const ezGALTexture* pSource, const ezGALTextureSubresource& SourceSubResource) = 0;

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

  void CountStateChange() { m_uiStateChanges++; }
  void CountRedundantStateChange() { m_uiRedundantStateChanges++; }

private:
  friend class ezMemoryUtils;

  // Parent Device
  ezGALDevice& m_Device;

  // Used to track redundant state changes
  ezGALShaderHandle m_hShader;

  ezGALBufferHandle m_hConstantBuffers[EZ_GAL_MAX_CONSTANT_BUFFER_COUNT];

  ezHybridArray<ezGALResourceViewHandle, 16> m_hResourceViews[ezGALShaderStage::ENUM_COUNT];
  ezHybridArray<const ezGALResourceBase*, 16> m_pResourcesForResourceViews[ezGALShaderStage::ENUM_COUNT];

  ezHybridArray<ezGALUnorderedAccessViewHandle, 16> m_hUnorderedAccessViews;
  ezHybridArray<const ezGALResourceBase*, 16> m_pResourcesForUnorderedAccessViews;

  ezGALSamplerStateHandle m_hSamplerStates[ezGALShaderStage::ENUM_COUNT][EZ_GAL_MAX_SAMPLER_COUNT];

  // Statistic variables
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

  // State functions

  void SetIndexBuffer(ezGALBufferHandle hIndexBuffer);
  void SetVertexBuffer(ezUInt32 uiSlot, ezGALBufferHandle hVertexBuffer);
  void SetVertexDeclaration(ezGALVertexDeclarationHandle hVertexDeclaration);

  ezGALPrimitiveTopology::Enum GetPrimitiveTopology() const { return m_Topology; }
  void SetPrimitiveTopology(ezGALPrimitiveTopology::Enum Topology);

  void SetBlendState(ezGALBlendStateHandle hBlendState, const ezColor& BlendFactor = ezColor::White, ezUInt32 uiSampleMask = 0xFFFFFFFFu);
  void SetDepthStencilState(ezGALDepthStencilStateHandle hDepthStencilState, ezUInt8 uiStencilRefValue = 0xFFu);
  void SetRasterizerState(ezGALRasterizerStateHandle hRasterizerState);

  void SetViewport(const ezRectFloat& rect, float fMinDepth = 0.0f, float fMaxDepth = 1.0f);
  void SetScissorRect(const ezRectU32& rect);

  void SetStreamOutBuffer(ezUInt32 uiSlot, ezGALBufferHandle hBuffer, ezUInt32 uiOffset);

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

  // State functions

  virtual void SetIndexBufferPlatform(const ezGALBuffer* pIndexBuffer) = 0;
  virtual void SetVertexBufferPlatform(ezUInt32 uiSlot, const ezGALBuffer* pVertexBuffer) = 0;
  virtual void SetVertexDeclarationPlatform(const ezGALVertexDeclaration* pVertexDeclaration) = 0;
  virtual void SetPrimitiveTopologyPlatform(ezGALPrimitiveTopology::Enum Topology) = 0;

  virtual void SetBlendStatePlatform(const ezGALBlendState* pBlendState, const ezColor& BlendFactor, ezUInt32 uiSampleMask) = 0;
  virtual void SetDepthStencilStatePlatform(const ezGALDepthStencilState* pDepthStencilState, ezUInt8 uiStencilRefValue) = 0;
  virtual void SetRasterizerStatePlatform(const ezGALRasterizerState* pRasterizerState) = 0;

  virtual void SetViewportPlatform(const ezRectFloat& rect, float fMinDepth, float fMaxDepth) = 0;
  virtual void SetScissorRectPlatform(const ezRectU32& rect) = 0;

  virtual void SetStreamOutBufferPlatform(ezUInt32 uiSlot, const ezGALBuffer* pBuffer, ezUInt32 uiOffset) = 0;

  virtual void InvalidateState() override;

private:
  void CountDrawCall() { m_uiDrawCalls++; }

  // Used to track redundant state changes
  ezGALBufferHandle m_hVertexBuffers[EZ_GAL_MAX_VERTEX_BUFFER_COUNT];
  ezGALBufferHandle m_hIndexBuffer;

  ezGALVertexDeclarationHandle m_hVertexDeclaration;
  ezGALPrimitiveTopology::Enum m_Topology;

  ezGALBlendStateHandle m_hBlendState;
  ezColor m_BlendFactor;
  ezUInt32 m_uiSampleMask;

  ezGALDepthStencilStateHandle m_hDepthStencilState;
  ezUInt8 m_uiStencilRefValue;

  ezGALRasterizerStateHandle m_hRasterizerState;

  ezRectU32 m_ScissorRect;
  ezRectFloat m_ViewPortRect;
  float m_fViewPortMinDepth;
  float m_fViewPortMaxDepth;

  // Statistic variables
  ezUInt32 m_uiDrawCalls = 0;
};

class EZ_RENDERERFOUNDATION_DLL ezGALComputeCommandEncoder : public ezGALCommandEncoder
{
public:
  // Dispatch

  void Dispatch(ezUInt32 uiThreadGroupCountX, ezUInt32 uiThreadGroupCountY, ezUInt32 uiThreadGroupCountZ);
  void DispatchIndirect(ezGALBufferHandle hIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes);

  virtual void ClearStatisticsCounters() override;

protected:
  friend class ezGALDevice;

  ezGALComputeCommandEncoder(ezGALDevice& device);
  virtual ~ezGALComputeCommandEncoder();

  // Dispatch

  virtual void DispatchPlatform(ezUInt32 uiThreadGroupCountX, ezUInt32 uiThreadGroupCountY, ezUInt32 uiThreadGroupCountZ) = 0;

  virtual void DispatchIndirectPlatform(const ezGALBuffer* pIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes) = 0;

private:
  void CountDispatchCall() { m_uiDispatchCalls++; }

  // Statistic variables
  ezUInt32 m_uiDispatchCalls = 0;
};
