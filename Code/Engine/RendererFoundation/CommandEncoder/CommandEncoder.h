
#pragma once

#include <Foundation/Threading/ThreadUtils.h>
#include <RendererFoundation/CommandEncoder/CommandEncoderPlatformInterface.h>
#include <RendererFoundation/CommandEncoder/CommandEncoderState.h>

struct ezGALRenderingSetup;
struct ezGALDeviceEvent;

class EZ_RENDERERFOUNDATION_DLL ezGALCommandEncoder
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezGALCommandEncoder);

public:
  ezGALCommandEncoder(ezGALDevice& ref_device, ezGALCommandEncoderCommonPlatformInterface& ref_commonImpl);
  virtual ~ezGALCommandEncoder();

  // State setting functions

  void SetShader(ezGALShaderHandle hShader);

  void SetConstantBuffer(const ezShaderResourceBinding& binding, ezGALBufferHandle hBuffer);
  void SetSamplerState(const ezShaderResourceBinding& binding, ezGALSamplerStateHandle hSamplerState);
  void SetResourceView(const ezShaderResourceBinding& binding, ezGALTextureResourceViewHandle hResourceView);
  void SetResourceView(const ezShaderResourceBinding& binding, ezGALBufferResourceViewHandle hResourceView);
  void SetUnorderedAccessView(const ezShaderResourceBinding& binding, ezGALTextureUnorderedAccessViewHandle hUnorderedAccessView);
  void SetUnorderedAccessView(const ezShaderResourceBinding& binding, ezGALBufferUnorderedAccessViewHandle hUnorderedAccessView);
  void SetPushConstants(ezArrayPtr<const ezUInt8> data);

  // GPU -> CPU query functions

  /// Inserts a timestamp.
  /// \return A handle to be passed into ezGALDevice::GetTimestampResult.
  ezGALTimestampHandle InsertTimestamp();

  /// \brief Starts an occlusion query.
  /// This function must be called within a render scope and EndOcclusionQuery must be called within the same scope. Only one occlusion query can be active at any given time.
  /// \param type The type of the occlusion query.
  /// \return A handle to be passed into EndOcclusionQuery.
  /// \sa EndOcclusionQuery
  ezGALOcclusionHandle BeginOcclusionQuery(ezEnum<ezGALQueryType> type);

  /// \brief Ends an occlusion query.
  /// The given handle must afterwards be passed into the ezGALDevice::GetOcclusionQueryResult function, which needs to be repeated every frame until results are ready.
  /// \param hOcclusion Value returned by the previous call to BeginOcclusionQuery.
  /// \sa ezGALDevice::GetOcclusionQueryResult
  void EndOcclusionQuery(ezGALOcclusionHandle hOcclusion);

  /// Inserts a fence.
  /// You need to flush commands to the GPU in order to be able to wait for a fence by either ending a frame or calling `ezCommandEncoder::Flush` explicitly.
  /// \return A handle to be passed into ezGALDevice::GetFenceResult.
  /// \sa ezGALDevice::GetFenceResult
  ezGALFenceHandle InsertFence();

  // Resource functions

  /// Clears an unordered access view with a float value.
  void ClearUnorderedAccessView(ezGALTextureUnorderedAccessViewHandle hUnorderedAccessView, ezVec4 vClearValues);
  void ClearUnorderedAccessView(ezGALBufferUnorderedAccessViewHandle hUnorderedAccessView, ezVec4 vClearValues);

  /// Clears an unordered access view with an int value.
  void ClearUnorderedAccessView(ezGALTextureUnorderedAccessViewHandle hUnorderedAccessView, ezVec4U32 vClearValues);
  void ClearUnorderedAccessView(ezGALBufferUnorderedAccessViewHandle hUnorderedAccessView, ezVec4U32 vClearValues);

  void CopyBuffer(ezGALBufferHandle hDest, ezGALBufferHandle hSource);
  void CopyBufferRegion(ezGALBufferHandle hDest, ezUInt32 uiDestOffset, ezGALBufferHandle hSource, ezUInt32 uiSourceOffset, ezUInt32 uiByteCount);

  void UpdateBuffer(ezGALBufferHandle hDest, ezUInt32 uiDestOffset, ezArrayPtr<const ezUInt8> sourceData, ezGALUpdateMode::Enum updateMode = ezGALUpdateMode::TransientConstantBuffer);

  void CopyTexture(ezGALTextureHandle hDest, ezGALTextureHandle hSource);
  void CopyTextureRegion(ezGALTextureHandle hDest, const ezGALTextureSubresource& destinationSubResource, const ezVec3U32& vDestinationPoint, ezGALTextureHandle hSource, const ezGALTextureSubresource& sourceSubResource, const ezBoundingBoxu32& box);

  void UpdateTexture(ezGALTextureHandle hDest, const ezGALTextureSubresource& destinationSubResource, const ezBoundingBoxu32& destinationBox, const ezGALSystemMemoryDescription& sourceData);

  void ResolveTexture(ezGALTextureHandle hDest, const ezGALTextureSubresource& destinationSubResource, ezGALTextureHandle hSource, const ezGALTextureSubresource& sourceSubResource);

  void ReadbackTexture(ezGALReadbackTextureHandle hDestination, ezGALTextureHandle hSource);
  void ReadbackBuffer(ezGALReadbackBufferHandle hDestination, ezGALBufferHandle hSource);

  void GenerateMipMaps(ezGALTextureResourceViewHandle hResourceView);

  // Misc

  /// \brief Submits all pending work to the GPU.
  /// Call this if you want to wait for a fence or some other kind of GPU synchronization to take place to ensure the work is actually submitted to the GPU.
  void Flush();

  // Debug helper functions

  void PushMarker(const char* szMarker);
  void PopMarker();
  void InsertEventMarker(const char* szMarker);

  // Dispatch

  void BeginCompute(const char* szName = "");
  void EndCompute();

  ezResult Dispatch(ezUInt32 uiThreadGroupCountX, ezUInt32 uiThreadGroupCountY, ezUInt32 uiThreadGroupCountZ);
  ezResult DispatchIndirect(ezGALBufferHandle hIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes);

  // Draw functions

  void BeginRendering(const ezGALRenderingSetup& renderingSetup, const char* szName = "");
  void EndRendering();
  bool IsInRenderingScope() const;

  /// \brief Clears active rendertargets.
  ///
  /// \param uiRenderTargetClearMask
  ///   Each bit represents a bound color target. If all bits are set, all bound color targets will be cleared.
  void Clear(const ezColor& clearColor, ezUInt32 uiRenderTargetClearMask = 0xFFFFFFFFu, bool bClearDepth = true, bool bClearStencil = true, float fDepthClear = 1.0f, ezUInt8 uiStencilClear = 0x0u);

  ezResult Draw(ezUInt32 uiVertexCount, ezUInt32 uiStartVertex);
  ezResult DrawIndexed(ezUInt32 uiIndexCount, ezUInt32 uiStartIndex);
  ezResult DrawIndexedInstanced(ezUInt32 uiIndexCountPerInstance, ezUInt32 uiInstanceCount, ezUInt32 uiStartIndex);
  ezResult DrawIndexedInstancedIndirect(ezGALBufferHandle hIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes);
  ezResult DrawInstanced(ezUInt32 uiVertexCountPerInstance, ezUInt32 uiInstanceCount, ezUInt32 uiStartVertex);
  ezResult DrawInstancedIndirect(ezGALBufferHandle hIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes);

  // State functions

  void SetIndexBuffer(ezGALBufferHandle hIndexBuffer);
  void SetVertexBuffer(ezUInt32 uiSlot, ezGALBufferHandle hVertexBuffer, ezUInt32 uiOffset = 0);
  void SetVertexDeclaration(ezGALVertexDeclarationHandle hVertexDeclaration);

  ezGALPrimitiveTopology::Enum GetPrimitiveTopology() const { return m_State.m_Topology; }
  void SetPrimitiveTopology(ezGALPrimitiveTopology::Enum topology);

  void SetBlendState(ezGALBlendStateHandle hBlendState, const ezColor& blendFactor = ezColor::White, ezUInt32 uiSampleMask = 0xFFFFFFFFu);
  void SetDepthStencilState(ezGALDepthStencilStateHandle hDepthStencilState, ezUInt8 uiStencilRefValue = 0xFFu);
  void SetRasterizerState(ezGALRasterizerStateHandle hRasterizerState);

  void SetViewport(const ezRectFloat& rect, float fMinDepth = 0.0f, float fMaxDepth = 1.0f);
  void SetScissorRect(const ezRectU32& rect);

  // Internal

  EZ_ALWAYS_INLINE ezGALDevice& GetDevice() { return m_Device; }
  // Don't use light hearted ;)
  void InvalidateState();

protected:
  friend class ezGALDevice;

  void GALStaticDeviceEventHandler(const ezGALDeviceEvent& e);

  void AssertRenderingThread()
  {
    EZ_ASSERT_DEV(ezThreadUtils::IsMainThread(), "This function can only be executed on the main thread.");
  }

  void AssertOutsideRenderingScope()
  {
    EZ_ASSERT_DEBUG(m_CurrentCommandEncoderType != CommandEncoderType::Render, "This function can only be executed outside a render scope.");
  }

private:
  void ClearStatisticsCounters();
  void CountDispatchCall() { m_uiDispatchCalls++; }
  void CountDrawCall() { m_uiDrawCalls++; }

private:
  friend class ezMemoryUtils;

  // Statistic variables
  ezUInt32 m_uiDispatchCalls = 0;
  ezUInt32 m_uiDrawCalls = 0;

  enum class CommandEncoderType
  {
    Invalid,
    Render,
    Compute
  };

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  // This code ensures in debug build that a buffer is not updated twice per frame in the same location
  struct BufferRange
  {
    inline bool overlapRange(ezUInt32 uiOffset, ezUInt32 uiLength) const
    {
      return !(m_uiOffset > (uiOffset + uiLength - 1) || (m_uiOffset + m_uiLength - 1) < uiOffset);
    }
    ezUInt32 m_uiOffset = 0;
    ezUInt32 m_uiLength = 0;
    EZ_DECLARE_POD_TYPE();
  };
  ezMap<ezGALBufferHandle, ezHybridArray<BufferRange, 1>> m_BufferUpdates;
#endif

  CommandEncoderType m_CurrentCommandEncoderType = CommandEncoderType::Invalid;
  bool m_bMarker = false;

  // Parent Device
  ezGALDevice& m_Device;
  ezGALCommandEncoderRenderState m_State;
  ezGALCommandEncoderCommonPlatformInterface& m_CommonImpl;

  ezGALOcclusionHandle m_hPendingOcclusionQuery = {};
};
