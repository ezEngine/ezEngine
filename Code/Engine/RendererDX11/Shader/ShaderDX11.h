
#pragma once

#include <RendererFoundation/Basics.h>
#include <RendererFoundation/Shader/Shader.h>
#include <RendererDX11/Basics.h>

struct ID3D11VertexShader;
struct ID3D11HullShader;
struct ID3D11DomainShader;
struct ID3D11GeometryShader;
struct ID3D11PixelShader;
struct ID3D11ComputeShader;

class EZ_RENDERERDX11_DLL ezGALShaderDX11 : public ezGALShader
{
public:

  EZ_FORCE_INLINE ID3D11VertexShader* GetDXVertexShader() const;

  EZ_FORCE_INLINE ID3D11HullShader* GetDXHullShader() const;

  EZ_FORCE_INLINE ID3D11DomainShader* GetDXDomainShader() const;

  EZ_FORCE_INLINE ID3D11GeometryShader* GetDXGeometryShader() const;

  EZ_FORCE_INLINE ID3D11PixelShader* GetDXPixelShader() const;

  EZ_FORCE_INLINE ID3D11ComputeShader* GetDXComputeShader() const;


protected:

  friend class ezGALDeviceDX11;
  friend class ezMemoryUtils;

  ezGALShaderDX11(const ezGALShaderCreationDescription& description);

  virtual ~ezGALShaderDX11();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  ID3D11VertexShader* m_pVertexShader;
  ID3D11HullShader* m_pHullShader;
  ID3D11DomainShader* m_pDomainShader;
  ID3D11GeometryShader* m_pGeometryShader;
  ID3D11PixelShader* m_pPixelShader;
  ID3D11ComputeShader* m_pComputeShader;

};

#include <RendererDX11/Shader/Implementation/ShaderDX11_inl.h>