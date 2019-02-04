#include <PCH.h>

#include <RendererFoundation/Basics.h>
#include <RendererFoundation/State/State.h>

ezGALBlendState::ezGALBlendState(const ezGALBlendStateCreationDescription& Description)
    : ezGALObject(Description)
{
}

ezGALBlendState::~ezGALBlendState() {}



ezGALDepthStencilState::ezGALDepthStencilState(const ezGALDepthStencilStateCreationDescription& Description)
    : ezGALObject(Description)
{
}

ezGALDepthStencilState::~ezGALDepthStencilState() {}



ezGALRasterizerState::ezGALRasterizerState(const ezGALRasterizerStateCreationDescription& Description)
    : ezGALObject(Description)
{
}

ezGALRasterizerState::~ezGALRasterizerState() {}


ezGALSamplerState::ezGALSamplerState(const ezGALSamplerStateCreationDescription& Description)
    : ezGALObject(Description)
{
}

ezGALSamplerState::~ezGALSamplerState() {}



EZ_STATICLINK_FILE(RendererFoundation, RendererFoundation_State_Implementation_State);

