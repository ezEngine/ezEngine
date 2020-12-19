
#pragma once

#include <RendererFoundation/Resources/RenderTargetSetup.h>

class EZ_RENDERERFOUNDATION_DLL ezGALPass
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezGALPass);

public:
  ezGALRenderCommandEncoder* BeginRendering(const ezGALRenderingSetup& renderingSetup, const char* szName = "");
  void EndRendering(ezGALRenderCommandEncoder* pCommandEncoder);

  ezGALComputeCommandEncoder* BeginCompute(const char* szName = "");
  void EndCompute(ezGALComputeCommandEncoder* pCommandEncoder);

  // BeginRaytracing() could be here as well (would match Vulkan)

protected:
  virtual ezGALRenderCommandEncoder* BeginRenderingPlatform(const ezGALRenderingSetup& renderingSetup, const char* szName) = 0;
  virtual void EndRenderingPlatform(ezGALRenderCommandEncoder* pCommandEncoder) = 0;

  virtual ezGALComputeCommandEncoder* BeginComputePlatform(const char* szName) = 0;
  virtual void EndComputePlatform(ezGALComputeCommandEncoder* pCommandEncoder) = 0;

  ezGALPass(ezGALDevice& device);
  virtual ~ezGALPass();

  ezGALDevice& m_Device;

  enum class CommandEncoderType
  {
    Invalid,
    Render,
    Compute
  };

  CommandEncoderType m_CurrentCommandEncoderType = CommandEncoderType::Invalid;
  bool m_bMarker = false;
};
