
#pragma once

#include <RendererFoundation/Resources/RenderTargetSetup.h>

struct ezGALRenderingSetup
{
  ezGALRenderTargetSetup m_RenderTargetSetup;
  ezColor m_ClearColor = ezColor(0, 0, 0, 0);
  ezUInt32 m_uiRenderTargetClearMask = 0x0;
  float m_fDepthClear = 1.0f;
  ezUInt8 m_uiStencilClear = 0;
  bool m_bClearDepth = false;
  bool m_bClearStencil = false;
  bool m_bDiscardColor = false;
  bool m_bDiscardDepth = false;
};

class EZ_RENDERERFOUNDATION_DLL ezGALPass
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezGALPass);

public:
  ezGALRenderCommandEncoder* BeginRendering(ezGALRenderingSetup&& renderingSetup, const char* szName = "");
  void EndRendering(ezGALRenderCommandEncoder* pCommandEncoder);

  ezGALComputeCommandEncoder* BeginCompute(const char* szName = "");
  void EndCompute(ezGALComputeCommandEncoder* pCommandEncoder);

  // BeginRaytracing() could be here as well (would match Vulkan)

protected:
  virtual ezGALRenderCommandEncoder* BeginRenderingPlatform(ezGALRenderingSetup&& renderingSetup, const char* szName) = 0;
  virtual void EndRenderingPlatform(ezGALRenderCommandEncoder* pCommandEncoder) = 0;

  virtual ezGALComputeCommandEncoder* BeginComputePlatform(const char* szName) = 0;
  virtual void EndComputePlatform(ezGALComputeCommandEncoder* pCommandEncoder) = 0;

  ezGALPass(ezGALDevice& device, const char* szName);
  virtual ~ezGALPass();

  ezGALDevice& m_Device;

  enum class CommandEncoderType
  {
    Invalid,
    Render,
    Compute
  };

  CommandEncoderType m_CurrentCommandEncoderType = CommandEncoderType::Invalid;
};
