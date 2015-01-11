
#include <RendererFoundation/PCH.h>
#include <RendererFoundation/Basics.h>
#include <RendererFoundation/State/State.h>

ezGALBlendState::ezGALBlendState(const ezGALBlendStateCreationDescription& Description)
  : ezGALObjectBase(Description)
{
}

ezGALBlendState::~ezGALBlendState()
{
}



ezGALDepthStencilState::ezGALDepthStencilState(const ezGALDepthStencilStateCreationDescription& Description)
  : ezGALObjectBase(Description)
{
}

ezGALDepthStencilState::~ezGALDepthStencilState()
{
}



ezGALRasterizerState::ezGALRasterizerState(const ezGALRasterizerStateCreationDescription& Description)
  : ezGALObjectBase(Description)
{
}

ezGALRasterizerState::~ezGALRasterizerState()
{
}


ezGALSamplerState::ezGALSamplerState(const ezGALSamplerStateCreationDescription& Description)
: ezGALObjectBase(Description)
{
}

ezGALSamplerState::~ezGALSamplerState()
{
}



EZ_STATICLINK_FILE(RendererFoundation, RendererFoundation_State_Implementation_State);

