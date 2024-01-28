
#pragma once

#include <RendererDX11/RendererDX11DLL.h>
#include <RendererFoundation/RendererFoundationDLL.h>
#include <RendererFoundation/Shader/Shader.h>

struct ID3D11VertexShader;
struct ID3D11HullShader;
struct ID3D11DomainShader;
struct ID3D11GeometryShader;
struct ID3D11PixelShader;
struct ID3D11ComputeShader;

class EZ_RENDERERDX11_DLL ezGALShaderDX11 : public ezGALShader
{
public:
  void SetDebugName(const char* szName) const override;

  EZ_ALWAYS_INLINE ID3D11VertexShader* GetDXVertexShader() const;

  EZ_ALWAYS_INLINE ID3D11HullShader* GetDXHullShader() const;

  EZ_ALWAYS_INLINE ID3D11DomainShader* GetDXDomainShader() const;

  EZ_ALWAYS_INLINE ID3D11GeometryShader* GetDXGeometryShader() const;

  EZ_ALWAYS_INLINE ID3D11PixelShader* GetDXPixelShader() const;

  EZ_ALWAYS_INLINE ID3D11ComputeShader* GetDXComputeShader() const;

protected:
  friend class ezGALDeviceDX11;
  friend class ezMemoryUtils;

  ezGALShaderDX11(const ezGALShaderCreationDescription& description);

  virtual ~ezGALShaderDX11();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  ID3D11VertexShader* m_pVertexShader = nullptr;
  ID3D11HullShader* m_pHullShader = nullptr;
  ID3D11DomainShader* m_pDomainShader = nullptr;
  ID3D11GeometryShader* m_pGeometryShader = nullptr;
  ID3D11PixelShader* m_pPixelShader = nullptr;
  ID3D11ComputeShader* m_pComputeShader = nullptr;
};

#include <RendererDX11/Shader/Implementation/ShaderDX11_inl.h>
