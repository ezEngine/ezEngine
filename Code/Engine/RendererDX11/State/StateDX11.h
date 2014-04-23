
#pragma once

#include <RendererFoundation/State/State.h>


struct ID3D11BlendState;
struct ID3D11DepthStencilState;
struct ID3D11RasterizerState;
struct ID3D11SamplerState;

class EZ_RENDERERDX11_DLL ezGALBlendStateDX11 : public ezGALBlendState
{
public:

  EZ_FORCE_INLINE ID3D11BlendState* GetDXBlendState() const;

protected:

  friend class ezGALDeviceDX11;
  friend class ezMemoryUtils;

  ezGALBlendStateDX11(const ezGALBlendStateCreationDescription& Description);

  ~ezGALBlendStateDX11();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  ID3D11BlendState* m_pDXBlendState;
};

class EZ_RENDERERDX11_DLL ezGALDepthStencilStateDX11 : public ezGALDepthStencilState
{
public:

  EZ_FORCE_INLINE ID3D11DepthStencilState* GetDXDepthStencilState() const;

protected:

  friend class ezGALDeviceDX11;
  friend class ezMemoryUtils;

  ezGALDepthStencilStateDX11(const ezGALDepthStencilStateCreationDescription& Description);

  ~ezGALDepthStencilStateDX11();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  ID3D11DepthStencilState* m_pDXDepthStencilState;
};

class EZ_RENDERERDX11_DLL ezGALRasterizerStateDX11 : public ezGALRasterizerState
{
public:

  EZ_FORCE_INLINE ID3D11RasterizerState* GetDXRasterizerState() const;

protected:

  friend class ezGALDeviceDX11;
  friend class ezMemoryUtils;

  ezGALRasterizerStateDX11(const ezGALRasterizerStateCreationDescription& Description);

  ~ezGALRasterizerStateDX11();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  ID3D11RasterizerState* m_pDXRasterizerState;
};

class EZ_RENDERERDX11_DLL ezGALSamplerStateDX11 : public ezGALSamplerState
{
public:

  EZ_FORCE_INLINE ID3D11SamplerState* GetDXSamplerState() const;

protected:

  friend class ezGALDeviceDX11;
  friend class ezMemoryUtils;

  ezGALSamplerStateDX11(const ezGALSamplerStateCreationDescription& Description);

  ~ezGALSamplerStateDX11();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  ID3D11SamplerState* m_pDXSamplerState;
};


#include <RendererDX11/State/Implementation/StateDX11_inl.h>
