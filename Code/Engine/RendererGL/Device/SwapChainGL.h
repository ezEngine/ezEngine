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

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)

  /// \brief Returns the window's device context.
  HDC GetWindowDC() const { return m_hDC; }

  /// \brief Returns the window's OpenGL context. This might not be initialized, if the window was set up with a Direct3D context.
  HGLRC GetOpenGLRC() const { return m_hRC; }

#endif

  /// Enables or disables vertical synchronization.
  void SetVSync(bool active);

protected:

  friend class ezGALDeviceGL;
  friend class ezMemoryUtils;

  ezGALSwapChainGL(const ezGALSwapChainCreationDescription& Description);

  virtual ~ezGALSwapChainGL();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  /// \brief Copies backbuffer texture to actual backbuffer and performs OS dependent swap.
  void SwapBuffers(ezGALDevice* pDevice);

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  HDC m_hDC;
  HGLRC m_hRC;

  ezResult CreateContextWindows();
  ezResult DestroyContextWindows();
#endif
};

#include <RendererGL/Device/Implementation/SwapChainGL_inl.h>
