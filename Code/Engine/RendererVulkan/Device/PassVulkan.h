
#pragma once

#include <RendererFoundation/Device/Pass.h>

struct ezGALCommandEncoderRenderState;
class ezGALRenderCommandEncoder;
class ezGALComputeCommandEncoder;

class ezGALCommandEncoderImplVulkan;

class ezGALPassVulkan : public ezGALPass
{
protected:
  friend class ezGALDeviceVulkan;
  friend class ezMemoryUtils;

  virtual ezGALRenderCommandEncoder* BeginRenderingPlatform(const ezGALRenderingSetup& renderingSetup, const char* szName) override;
  virtual void EndRenderingPlatform(ezGALRenderCommandEncoder* pCommandEncoder) override;

  virtual ezGALComputeCommandEncoder* BeginComputePlatform(const char* szName) override;
  virtual void EndComputePlatform(ezGALComputeCommandEncoder* pCommandEncoder) override;

  ezGALPassVulkan(ezGALDevice& device);
  virtual ~ezGALPassVulkan();

  void BeginPass(const char* szName);
  void EndPass();

private:
  ezUniquePtr<ezGALCommandEncoderRenderState> m_pCommandEncoderState;
  ezUniquePtr<ezGALCommandEncoderImplVulkan> m_pCommandEncoderImpl;

  ezUniquePtr<ezGALRenderCommandEncoder> m_pRenderCommandEncoder;
  ezUniquePtr<ezGALComputeCommandEncoder> m_pComputeCommandEncoder;
};
