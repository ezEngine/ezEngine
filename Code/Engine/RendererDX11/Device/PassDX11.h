
#pragma once

#include <RendererFoundation/Device/Pass.h>

struct ezGALCommandEncoderRenderState;
class ezGALRenderCommandEncoder;
class ezGALComputeCommandEncoder;

class ezGALCommandEncoderImplDX11;

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
  ezUniquePtr<ezGALCommandEncoderRenderState> m_pCommandEncoderState;
  ezUniquePtr<ezGALCommandEncoderImplDX11> m_pCommandEncoderImpl;

  ezUniquePtr<ezGALRenderCommandEncoder> m_pRenderCommandEncoder;
  ezUniquePtr<ezGALComputeCommandEncoder> m_pComputeCommandEncoder;
};
