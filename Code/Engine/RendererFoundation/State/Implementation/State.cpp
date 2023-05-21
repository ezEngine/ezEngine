#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/RendererFoundationDLL.h>
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
  : ezGALObject(Description)
{
}

ezGALSamplerState::~ezGALSamplerState() = default;



EZ_STATICLINK_FILE(RendererFoundation, RendererFoundation_State_Implementation_State);
