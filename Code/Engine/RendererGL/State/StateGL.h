
#pragma once

#include <RendererFoundation/State/State.h>

class EZ_RENDERERGL_DLL ezGALBlendStateGL : public ezGALBlendState
{
protected:

  friend class ezGALDeviceGL;
  friend class ezMemoryUtils;

  ezGALBlendStateGL(const ezGALBlendStateCreationDescription& Description);

  ~ezGALBlendStateGL();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) EZ_OVERRIDE;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) EZ_OVERRIDE;
};

class EZ_RENDERERGL_DLL ezGALDepthStencilStateGL : public ezGALDepthStencilState
{
protected:

  friend class ezGALDeviceGL;
  friend class ezMemoryUtils;

  ezGALDepthStencilStateGL(const ezGALDepthStencilStateCreationDescription& Description);

  ~ezGALDepthStencilStateGL();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) EZ_OVERRIDE;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) EZ_OVERRIDE;
};

class EZ_RENDERERGL_DLL ezGALRasterizerStateGL : public ezGALRasterizerState
{
protected:

  friend class ezGALDeviceGL;
  friend class ezMemoryUtils;

  ezGALRasterizerStateGL(const ezGALRasterizerStateCreationDescription& Description);

  ~ezGALRasterizerStateGL();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) EZ_OVERRIDE;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) EZ_OVERRIDE;

  static const ezUInt32 s_GALCullModeToGL[ezGALCullMode::Enum::ENUM_COUNT];
};

class EZ_RENDERERGL_DLL ezGALSamplerStateGL : public ezGALSamplerState
{
public:

  EZ_FORCE_INLINE glSamplerId GetGLSamplerState() const;

protected:

  friend class ezGALDeviceGL;
  friend class ezMemoryUtils;

  ezGALSamplerStateGL(const ezGALSamplerStateCreationDescription& Description);

  ~ezGALSamplerStateGL();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) EZ_OVERRIDE;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) EZ_OVERRIDE;

  glSamplerId m_SamplerStateID;

  static const ezUInt32 s_GALAdressModeToGL[ezGALTextureAddressMode::Enum::ENUM_COUNT];
};


#include <RendererGL/State/Implementation/StateGL_inl.h>
