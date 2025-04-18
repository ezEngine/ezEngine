
#pragma once

#include <RendererDX11/RendererDX11DLL.h>
#include <RendererFoundation/RendererFoundationDLL.h>
#include <RendererFoundation/Shader/VertexDeclaration.h>

struct ID3D11InputLayout;

class ezGALVertexDeclarationDX11 : public ezGALVertexDeclaration
{
public:
  EZ_ALWAYS_INLINE ID3D11InputLayout* GetDXInputLayout() const;
  EZ_ALWAYS_INLINE ezArrayPtr<const ezUInt32> GetVertexBufferStrides() const;

protected:
  friend class ezGALDeviceDX11;
  friend class ezMemoryUtils;

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  ezGALVertexDeclarationDX11(const ezGALVertexDeclarationCreationDescription& Description);

  virtual ~ezGALVertexDeclarationDX11();

  ID3D11InputLayout* m_pDXInputLayout = nullptr;
  ezHybridArray<ezUInt32, EZ_GAL_MAX_VERTEX_BUFFER_COUNT> m_VertexBufferStrides;
};

#include <RendererDX11/Shader/Implementation/VertexDeclarationDX11_inl.h>
