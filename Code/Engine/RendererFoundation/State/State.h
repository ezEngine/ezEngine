
#pragma once

#include <RendererFoundation/Descriptors/Descriptors.h>

class EZ_RENDERERFOUNDATION_DLL ezGALBlendState : public ezGALObject<ezGALBlendStateCreationDescription>
{
public:

protected:

  ezGALBlendState(const ezGALBlendStateCreationDescription& Description);

  virtual ~ezGALBlendState();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) = 0;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) = 0;

};

class EZ_RENDERERFOUNDATION_DLL ezGALDepthStencilState : public ezGALObject<ezGALDepthStencilStateCreationDescription>
{
public:

protected:

  ezGALDepthStencilState(const ezGALDepthStencilStateCreationDescription& Description);

  virtual ~ezGALDepthStencilState();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) = 0;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) = 0;
};

class EZ_RENDERERFOUNDATION_DLL ezGALRasterizerState : public ezGALObject<ezGALRasterizerStateCreationDescription>
{
public:

protected:

  ezGALRasterizerState(const ezGALRasterizerStateCreationDescription& Description);

  virtual ~ezGALRasterizerState();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) = 0;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) = 0;
};

class EZ_RENDERERFOUNDATION_DLL ezGALSamplerState : public ezGALObject<ezGALSamplerStateCreationDescription>
{
public:

protected:

  ezGALSamplerState(const ezGALSamplerStateCreationDescription& Description);

  virtual ~ezGALSamplerState();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) = 0;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) = 0;
};