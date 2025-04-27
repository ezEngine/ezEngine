#include <RendererDX11/RendererDX11PCH.h>

#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/Resources/RenderTargetViewDX11.h>
#include <RendererDX11/Shader/ShaderDX11.h>
#include <RendererDX11/Shader/VertexDeclarationDX11.h>
#include <RendererDX11/State/GraphicsPipelineDX11.h>
#include <RendererDX11/State/StateDX11.h>

ezGALGraphicsPipelineDX11::ezGALGraphicsPipelineDX11(const ezGALGraphicsPipelineCreationDescription& description)
  : ezGALGraphicsPipeline(description)
{
}

ezGALGraphicsPipelineDX11::~ezGALGraphicsPipelineDX11() = default;

ezResult ezGALGraphicsPipelineDX11::InitPlatform(ezGALDevice*)
{
  return EZ_SUCCESS;
}

ezResult ezGALGraphicsPipelineDX11::DeInitPlatform(ezGALDevice*)
{
  return EZ_SUCCESS;
}

void ezGALGraphicsPipelineDX11::SetDebugName(const char*)
{
}
