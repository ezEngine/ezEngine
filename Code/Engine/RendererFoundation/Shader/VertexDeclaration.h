
#pragma once

#include <RendererFoundation/Basics.h>
#include <RendererFoundation/Descriptors/Descriptors.h>

class EZ_RENDERERFOUNDATION_DLL ezGALVertexDeclaration : public ezGALObjectBase<ezGALVertexDeclarationCreationDescription>
{
public:

protected:
  friend class ezGALDevice;

  virtual ezResult InitPlatform(ezGALDevice* pDevice) = 0;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) = 0;

  ezGALVertexDeclaration(const ezGALVertexDeclarationCreationDescription& Description);

  virtual ~ezGALVertexDeclaration();
};