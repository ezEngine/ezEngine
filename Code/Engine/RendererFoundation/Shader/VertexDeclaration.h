
#pragma once

#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class EZ_RENDERERFOUNDATION_DLL ezGALVertexDeclaration : public ezGALObject<ezGALVertexDeclarationCreationDescription>
{
public:
protected:
  friend class ezGALDevice;

  virtual ezResult InitPlatform(ezGALDevice* pDevice) = 0;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) = 0;

  ezGALVertexDeclaration(const ezGALVertexDeclarationCreationDescription& Description);

  virtual ~ezGALVertexDeclaration();
};
