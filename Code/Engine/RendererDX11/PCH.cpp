#include <RendererDX11/PCH.h>





EZ_STATICLINK_LIBRARY(RendererDX11)
{
  if (bReturn)
    return;

  EZ_STATICLINK_REFERENCE(RendererDX11_Context_Implementation_ContextDX11);
  EZ_STATICLINK_REFERENCE(RendererDX11_Device_Implementation_DeviceDX11);
  EZ_STATICLINK_REFERENCE(RendererDX11_Device_Implementation_SwapChainDX11);
  EZ_STATICLINK_REFERENCE(RendererDX11_Resources_Implementation_BufferDX11);
  EZ_STATICLINK_REFERENCE(RendererDX11_Resources_Implementation_RenderTargetConfigDX11);
  EZ_STATICLINK_REFERENCE(RendererDX11_Resources_Implementation_RenderTargetViewDX11);
  EZ_STATICLINK_REFERENCE(RendererDX11_Resources_Implementation_ResourceViewDX11);
  EZ_STATICLINK_REFERENCE(RendererDX11_Resources_Implementation_TextureDX11);
  EZ_STATICLINK_REFERENCE(RendererDX11_Shader_Implementation_ShaderDX11);
  EZ_STATICLINK_REFERENCE(RendererDX11_Shader_Implementation_VertexDeclarationDX11);
  EZ_STATICLINK_REFERENCE(RendererDX11_State_Implementation_StateDX11);
}

