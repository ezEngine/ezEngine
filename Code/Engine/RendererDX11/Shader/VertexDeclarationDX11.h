
#pragma once

#include <RendererFoundation/Basics.h>
#include <RendererFoundation/Shader/VertexDeclaration.h>
#include <RendererDX11/Basics.h>

struct ID3D11InputLayout;

class ezGALVertexDeclarationDX11 : public ezGALVertexDeclaration
{
public:

  EZ_FORCE_INLINE ID3D11InputLayout* GetDXInputLayout() const;

protected:

  friend class ezGALDeviceDX11;
  friend class ezMemoryUtils;

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  ezGALVertexDeclarationDX11(const ezGALVertexDeclarationCreationDescription& Description);

  virtual ~ezGALVertexDeclarationDX11();

  ID3D11InputLayout* m_pDXInputLayout;
};

#include <RendererDX11/Shader/Implementation/VertexDeclarationDX11_inl.h>
