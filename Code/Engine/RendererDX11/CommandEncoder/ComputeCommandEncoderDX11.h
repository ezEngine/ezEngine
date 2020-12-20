
#pragma once

#include <RendererFoundation/CommandEncoder/ComputeCommandEncoder.h>
#include <RendererDX11/CommandEncoder/CommandEncoderDX11.h>

class EZ_RENDERERDX11_DLL ezGALComputeCommandEncoderDX11 : public ezGALCommandEncoderDX11<ezGALComputeCommandEncoder>
{
protected:
  friend class ezGALDeviceDX11;
  friend class ezGALPassDX11;
  friend class ezMemoryUtils;

  using SUPER = ezGALCommandEncoderDX11<ezGALComputeCommandEncoder>;

  ezGALComputeCommandEncoderDX11(ezGALDevice& device);
  virtual ~ezGALComputeCommandEncoderDX11();

  // Dispatch

  virtual void DispatchPlatform(ezUInt32 uiThreadGroupCountX, ezUInt32 uiThreadGroupCountY, ezUInt32 uiThreadGroupCountZ) override;
  virtual void DispatchIndirectPlatform(const ezGALBuffer* pIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes) override;

private:

};
