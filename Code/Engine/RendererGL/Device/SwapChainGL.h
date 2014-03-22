#pragma once

#include <RendererGL/Basics.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Device/SwapChain.h>

class ezGALRenderTargetConfigGL;

/// \brief 
///
/// Each "SwapChain" of the same device needs to have the same pixel format, since they will use the same rendering context.
/// This way the more complicated resource sharing between contexts is avoided.
class ezGALSwapChainGL : public ezGALSwapChain
{
public:

protected:

  friend class ezGALDeviceGL;
  friend class ezMemoryUtils;

  ezGALSwapChainGL(const ezGALSwapChainCreationDescription& Description);

  virtual ~ezGALSwapChainGL();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) EZ_OVERRIDE;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) EZ_OVERRIDE;

  /// \brief Copies backbuffer texture to actual backbuffer and performs OS dependent swap.
  void SwapBuffers(ezGALDevice* pDevice);

  ezGALRenderTargetConfigHandle m_BackBufferRTConfig;
};

#include <RendererGL/Device/Implementation/SwapChainGL_inl.h>
