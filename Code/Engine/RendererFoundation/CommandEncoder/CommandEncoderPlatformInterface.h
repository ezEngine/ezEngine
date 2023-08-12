
#pragma once

#include <Foundation/Math/Color.h>
#include <Foundation/Math/Rect.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class EZ_RENDERERFOUNDATION_DLL ezGALCommandEncoderCommonPlatformInterface
{
public:
  // State setting functions

  virtual void SetShaderPlatform(const ezGALShader* pShader) = 0;

  virtual void SetConstantBufferPlatform(ezUInt32 uiSlot, const ezGALBuffer* pBuffer) = 0;
  virtual void SetSamplerStatePlatform(ezGALShaderStage::Enum stage, ezUInt32 uiSlot, const ezGALSamplerState* pSamplerState) = 0;
  virtual void SetResourceViewPlatform(ezGALShaderStage::Enum stage, ezUInt32 uiSlot, const ezGALResourceView* pResourceView) = 0;
  virtual void SetUnorderedAccessViewPlatform(ezUInt32 uiSlot, const ezGALUnorderedAccessView* pUnorderedAccessView) = 0;

  // Query functions

  virtual void BeginQueryPlatform(const ezGALQuery* pQuery) = 0;
  virtual void EndQueryPlatform(const ezGALQuery* pQuery) = 0;
  virtual ezResult GetQueryResultPlatform(const ezGALQuery* pQuery, ezUInt64& ref_uiQueryResult) = 0;

  // Timestamp functions

  virtual void InsertTimestampPlatform(ezGALTimestampHandle hTimestamp) = 0;

  // Resource update functions

  virtual void ClearUnorderedAccessViewPlatform(const ezGALUnorderedAccessView* pUnorderedAccessView, ezVec4 vClearValues) = 0;
  virtual void ClearUnorderedAccessViewPlatform(const ezGALUnorderedAccessView* pUnorderedAccessView, ezVec4U32 vClearValues) = 0;

  virtual void CopyBufferPlatform(const ezGALBuffer* pDestination, const ezGALBuffer* pSource) = 0;
  virtual void CopyBufferRegionPlatform(const ezGALBuffer* pDestination, ezUInt32 uiDestOffset, const ezGALBuffer* pSource, ezUInt32 uiSourceOffset, ezUInt32 uiByteCount) = 0;

  virtual void UpdateBufferPlatform(const ezGALBuffer* pDestination, ezUInt32 uiDestOffset, ezArrayPtr<const ezUInt8> sourceData, ezGALUpdateMode::Enum updateMode) = 0;

  virtual void CopyTexturePlatform(const ezGALTexture* pDestination, const ezGALTexture* pSource) = 0;
  virtual void CopyTextureRegionPlatform(const ezGALTexture* pDestination, const ezGALTextureSubresource& destinationSubResource, const ezVec3U32& vDestinationPoint, const ezGALTexture* pSource, const ezGALTextureSubresource& sourceSubResource, const ezBoundingBoxu32& box) = 0;

  virtual void UpdateTexturePlatform(const ezGALTexture* pDestination, const ezGALTextureSubresource& destinationSubResource, const ezBoundingBoxu32& destinationBox, const ezGALSystemMemoryDescription& sourceData) = 0;

  virtual void ResolveTexturePlatform(const ezGALTexture* pDestination, const ezGALTextureSubresource& destinationSubResource, const ezGALTexture* pSource, const ezGALTextureSubresource& sourceSubResource) = 0;

  virtual void ReadbackTexturePlatform(const ezGALTexture* pTexture) = 0;

  virtual void CopyTextureReadbackResultPlatform(const ezGALTexture* pTexture, ezArrayPtr<ezGALTextureSubresource> sourceSubResource, ezArrayPtr<ezGALSystemMemoryDescription> targetData) = 0;

  virtual void GenerateMipMapsPlatform(const ezGALResourceView* pResourceView) = 0;

  // Misc

  virtual void FlushPlatform() = 0;

  // Debug helper functions

  virtual void PushMarkerPlatform(const char* szMarker) = 0;
  virtual void PopMarkerPlatform() = 0;
  virtual void InsertEventMarkerPlatform(const char* szMarker) = 0;
};

class EZ_RENDERERFOUNDATION_DLL ezGALCommandEncoderRenderPlatformInterface
{
public:
  // Draw functions

  virtual void ClearPlatform(const ezColor& clearColor, ezUInt32 uiRenderTargetClearMask, bool bClearDepth, bool bClearStencil, float fDepthClear, ezUInt8 uiStencilClear) = 0;

  virtual void DrawPlatform(ezUInt32 uiVertexCount, ezUInt32 uiStartVertex) = 0;
  virtual void DrawIndexedPlatform(ezUInt32 uiIndexCount, ezUInt32 uiStartIndex) = 0;
  virtual void DrawIndexedInstancedPlatform(ezUInt32 uiIndexCountPerInstance, ezUInt32 uiInstanceCount, ezUInt32 uiStartIndex) = 0;
  virtual void DrawIndexedInstancedIndirectPlatform(const ezGALBuffer* pIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes) = 0;
  virtual void DrawInstancedPlatform(ezUInt32 uiVertexCountPerInstance, ezUInt32 uiInstanceCount, ezUInt32 uiStartVertex) = 0;
  virtual void DrawInstancedIndirectPlatform(const ezGALBuffer* pIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes) = 0;
  virtual void DrawAutoPlatform() = 0;

  // State functions

  virtual void SetIndexBufferPlatform(const ezGALBuffer* pIndexBuffer) = 0;
  virtual void SetVertexBufferPlatform(ezUInt32 uiSlot, const ezGALBuffer* pVertexBuffer) = 0;
  virtual void SetVertexDeclarationPlatform(const ezGALVertexDeclaration* pVertexDeclaration) = 0;
  virtual void SetPrimitiveTopologyPlatform(ezGALPrimitiveTopology::Enum topology) = 0;

  virtual void SetBlendStatePlatform(const ezGALBlendState* pBlendState, const ezColor& blendFactor, ezUInt32 uiSampleMask) = 0;
  virtual void SetDepthStencilStatePlatform(const ezGALDepthStencilState* pDepthStencilState, ezUInt8 uiStencilRefValue) = 0;
  virtual void SetRasterizerStatePlatform(const ezGALRasterizerState* pRasterizerState) = 0;

  virtual void SetViewportPlatform(const ezRectFloat& rect, float fMinDepth, float fMaxDepth) = 0;
  virtual void SetScissorRectPlatform(const ezRectU32& rect) = 0;
};

class EZ_RENDERERFOUNDATION_DLL ezGALCommandEncoderComputePlatformInterface
{
public:
  // Dispatch

  virtual void DispatchPlatform(ezUInt32 uiThreadGroupCountX, ezUInt32 uiThreadGroupCountY, ezUInt32 uiThreadGroupCountZ) = 0;
  virtual void DispatchIndirectPlatform(const ezGALBuffer* pIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes) = 0;
};
