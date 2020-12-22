#include <RendererFoundationPCH.h>

#include <RendererFoundation/CommandEncoder/ComputeCommandEncoder.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Buffer.h>

ezGALComputeCommandEncoder::ezGALComputeCommandEncoder(ezGALDevice& device, ezGALCommandEncoderState& state, ezGALCommandEncoderCommonPlatformInterface& commonImpl, ezGALCommandEncoderComputePlatformInterface& computeImpl)
  : ezGALCommandEncoder(device, state, commonImpl)
  , m_ComputeImpl(computeImpl)
{
}

ezGALComputeCommandEncoder::~ezGALComputeCommandEncoder() = default;

void ezGALComputeCommandEncoder::Dispatch(ezUInt32 uiThreadGroupCountX, ezUInt32 uiThreadGroupCountY, ezUInt32 uiThreadGroupCountZ)
{
  AssertRenderingThread();

  EZ_ASSERT_DEBUG(uiThreadGroupCountX > 0 && uiThreadGroupCountY > 0 && uiThreadGroupCountZ > 0, "Thread group counts of zero are not meaningful. Did you mean 1?");

  /// \todo Assert for compute

  m_ComputeImpl.DispatchPlatform(uiThreadGroupCountX, uiThreadGroupCountY, uiThreadGroupCountZ);

  CountDispatchCall();
}

void ezGALComputeCommandEncoder::DispatchIndirect(ezGALBufferHandle hIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes)
{
  AssertRenderingThread();
  /// \todo Assert for compute
  /// \todo Assert for indirect dispatch
  /// \todo Assert offset < buffer size

  const ezGALBuffer* pBuffer = GetDevice().GetBuffer(hIndirectArgumentBuffer);
  EZ_ASSERT_DEV(pBuffer != nullptr, "Invalid buffer handle for indirect arguments!");

  /// \todo Assert that the buffer can be used for indirect arguments (flag in desc)
  m_ComputeImpl.DispatchIndirectPlatform(pBuffer, uiArgumentOffsetInBytes);

  CountDispatchCall();
}

void ezGALComputeCommandEncoder::ClearStatisticsCounters()
{
  ezGALCommandEncoder::ClearStatisticsCounters();

  m_uiDispatchCalls = 0;
}
