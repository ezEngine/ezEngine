
#pragma once

#include <RendererFoundation/CommandEncoder/ComputeCommandEncoder.h>
#include <RendererVulkan/CommandEncoder/CommandEncoderVulkan.h>

class EZ_RENDERERVULKAN_DLL ezGALComputeCommandEncoderVulkan : public ezGALCommandEncoderVulkan<ezGALComputeCommandEncoder>
{
protected:
  friend class ezGALDeviceVulkan;
  friend class ezGALPassVulkan;
  friend class ezMemoryUtils;

  using SUPER = ezGALCommandEncoderVulkan<ezGALComputeCommandEncoder>;

  ezGALComputeCommandEncoderVulkan(ezGALDevice& device);
  virtual ~ezGALComputeCommandEncoderVulkan();

  void BeginEncode(vk::CommandBuffer& commandBuffer);
  void EndEncode();

  // Dispatch

  virtual void DispatchPlatform(ezUInt32 uiThreadGroupCountX, ezUInt32 uiThreadGroupCountY, ezUInt32 uiThreadGroupCountZ) override;
  virtual void DispatchIndirectPlatform(const ezGALBuffer* pIndirectArgumentBuffer, ezUInt32 uiArgumentOffsetInBytes) override;

private:

};
