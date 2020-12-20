
#pragma once

#include <RendererFoundation/Device/Pass.h>

class ezGALRenderCommandEncoderDX11;
class ezGALComputeCommandEncoderDX11;

class ezGALPassDX11 : public ezGALPass
{
protected:
  friend class ezGALDeviceDX11;
  friend class ezMemoryUtils;

  ezGALPassDX11(ezGALDevice& device);
  virtual ~ezGALPassDX11();

  virtual ezGALRenderCommandEncoder* BeginRenderingPlatform(const ezGALRenderingSetup& renderingSetup, const char* szName) override;
  virtual void EndRenderingPlatform(ezGALRenderCommandEncoder* pCommandEncoder) override;

  virtual ezGALComputeCommandEncoder* BeginComputePlatform(const char* szName) override;
  virtual void EndComputePlatform(ezGALComputeCommandEncoder* pCommandEncoder) override;

  void BeginPass(const char* szName);
  void EndPass();

private:
  ezUniquePtr<ezGALRenderCommandEncoderDX11> m_pRenderCommandEncoder;
  ezUniquePtr<ezGALComputeCommandEncoderDX11> m_pComputeCommandEncoder;
};
