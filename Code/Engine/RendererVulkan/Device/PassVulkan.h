
#pragma once

#include <RendererFoundation/Device/Pass.h>

class ezGALRenderCommandEncoderVulkan;
class ezGALComputeCommandEncoderVulkan;

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
  ezUniquePtr<ezGALRenderCommandEncoderVulkan> m_pRenderCommandEncoder;
  ezUniquePtr<ezGALComputeCommandEncoderVulkan> m_pComputeCommandEncoder;
};
