
#pragma once

#include <Foundation/Threading/ThreadUtils.h>
#include <RendererFoundation/CommandEncoder/CommandEncoderPlatformInterface.h>
#include <RendererFoundation/CommandEncoder/CommandEncoderState.h>

class EZ_RENDERERFOUNDATION_DLL ezGALCommandEncoder
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezGALCommandEncoder);

public:
  // State setting functions

  void SetShader(ezGALShaderHandle hShader);

  void SetConstantBuffer(const ezShaderResourceBinding& binding, ezGALBufferHandle hBuffer);
  void SetSamplerState(const ezShaderResourceBinding& binding, ezGALSamplerStateHandle hSamplerState);
  void SetResourceView(const ezShaderResourceBinding& binding, ezGALTextureResourceViewHandle hResourceView);
  void SetResourceView(const ezShaderResourceBinding& binding, ezGALBufferResourceViewHandle hResourceView);
  void SetUnorderedAccessView(const ezShaderResourceBinding& binding, ezGALTextureUnorderedAccessViewHandle hUnorderedAccessView);
  void SetUnorderedAccessView(const ezShaderResourceBinding& binding, ezGALBufferUnorderedAccessViewHandle hUnorderedAccessView);
  void SetPushConstants(ezArrayPtr<const ezUInt8> data);

  // Query functions

  void BeginQuery(ezGALQueryHandle hQuery);
  void EndQuery(ezGALQueryHandle hQuery);

  /// \return Success if retrieving the query succeeded.
  ezResult GetQueryResult(ezGALQueryHandle hQuery, ezUInt64& ref_uiQueryResult);

  // Timestamp functions

  ezGALTimestampHandle InsertTimestamp();

  // Resource functions

  /// Clears an unordered access view with a float value.
  void ClearUnorderedAccessView(ezGALTextureUnorderedAccessViewHandle hUnorderedAccessView, ezVec4 vClearValues);
  void ClearUnorderedAccessView(ezGALBufferUnorderedAccessViewHandle hUnorderedAccessView, ezVec4 vClearValues);

  /// Clears an unordered access view with an int value.
  void ClearUnorderedAccessView(ezGALTextureUnorderedAccessViewHandle hUnorderedAccessView, ezVec4U32 vClearValues);
  void ClearUnorderedAccessView(ezGALBufferUnorderedAccessViewHandle hUnorderedAccessView, ezVec4U32 vClearValues);

  void CopyBuffer(ezGALBufferHandle hDest, ezGALBufferHandle hSource);
  void CopyBufferRegion(ezGALBufferHandle hDest, ezUInt32 uiDestOffset, ezGALBufferHandle hSource, ezUInt32 uiSourceOffset, ezUInt32 uiByteCount);
  void UpdateBuffer(ezGALBufferHandle hDest, ezUInt32 uiDestOffset, ezArrayPtr<const ezUInt8> sourceData, ezGALUpdateMode::Enum updateMode = ezGALUpdateMode::Discard);

  void CopyTexture(ezGALTextureHandle hDest, ezGALTextureHandle hSource);
  void CopyTextureRegion(ezGALTextureHandle hDest, const ezGALTextureSubresource& destinationSubResource, const ezVec3U32& vDestinationPoint, ezGALTextureHandle hSource, const ezGALTextureSubresource& sourceSubResource, const ezBoundingBoxu32& box);

  void UpdateTexture(ezGALTextureHandle hDest, const ezGALTextureSubresource& destinationSubResource, const ezBoundingBoxu32& destinationBox, const ezGALSystemMemoryDescription& sourceData);

  void ResolveTexture(ezGALTextureHandle hDest, const ezGALTextureSubresource& destinationSubResource, ezGALTextureHandle hSource, const ezGALTextureSubresource& sourceSubResource);

  void ReadbackTexture(ezGALTextureHandle hTexture);
  void CopyTextureReadbackResult(ezGALTextureHandle hTexture, ezArrayPtr<ezGALTextureSubresource> sourceSubResource, ezArrayPtr<ezGALSystemMemoryDescription> targetData);

  void GenerateMipMaps(ezGALTextureResourceViewHandle hResourceView);

  // Misc

  void Flush();

  // Debug helper functions

  void PushMarker(const char* szMarker);
  void PopMarker();
  void InsertEventMarker(const char* szMarker);

  virtual void ClearStatisticsCounters();

  EZ_ALWAYS_INLINE ezGALDevice& GetDevice() { return m_Device; }
  // Don't use light hearted ;)
  void InvalidateState();

protected:
  friend class ezGALDevice;

  ezGALCommandEncoder(ezGALDevice& device, ezGALCommandEncoderState& state, ezGALCommandEncoderCommonPlatformInterface& commonImpl);
  virtual ~ezGALCommandEncoder();


  void AssertRenderingThread()
  {
    EZ_ASSERT_DEV(ezThreadUtils::IsMainThread(), "This function can only be executed on the main thread.");
  }

private:
  friend class ezMemoryUtils;

  // Parent Device
  ezGALDevice& m_Device;
  ezGALCommandEncoderState& m_State;
  ezGALCommandEncoderCommonPlatformInterface& m_CommonImpl;
};
