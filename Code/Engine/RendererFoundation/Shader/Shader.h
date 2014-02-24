
#pragma once

#include <RendererFoundation/Basics.h>
#include <RendererFoundation/Descriptors/Descriptors.h>

class EZ_RENDERERFOUNDATION_DLL ezGALShader : public ezGALObjectBase<ezGALShaderCreationDescription>
{
public:

protected:

  friend class ezGALDevice;

  virtual ezResult InitPlatform(ezGALDevice* pDevice) = 0;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) = 0;

  ezGALShader(const ezGALShaderCreationDescription& Description);

  virtual ~ezGALShader();
};

#include <RendererFoundation/Shader/Implementation/Shader_inl.h>
