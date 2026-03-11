#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/State/State.h>

ezGALBlendState::ezGALBlendState(const ezGALBlendStateCreationDescription& Description)
  : ezGALObject(Description)
{
}

ezGALBlendState::~ezGALBlendState() = default;



ezGALDepthStencilState::ezGALDepthStencilState(const ezGALDepthStencilStateCreationDescription& Description)
  : ezGALObject(Description)
{
}

ezGALDepthStencilState::~ezGALDepthStencilState() = default;



ezGALRasterizerState::ezGALRasterizerState(const ezGALRasterizerStateCreationDescription& Description)
  : ezGALObject(Description)
{
}

ezGALRasterizerState::~ezGALRasterizerState() = default;


ezGALSamplerState::ezGALSamplerState(const ezGALSamplerStateCreationDescription& Description)
  : ezGALResource(Description)
{
}

ezGALSamplerState::~ezGALSamplerState() = default;
