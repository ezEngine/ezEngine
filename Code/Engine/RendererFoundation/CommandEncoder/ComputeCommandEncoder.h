
#pragma once

#include <RendererFoundation/CommandEncoder/CommandEncoder.h>

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
