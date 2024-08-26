#include <RendererFoundation/RendererFoundationPCH.h>

EZ_STATICLINK_LIBRARY(RendererFoundation)
{
  if (bReturn)
    return;

  EZ_STATICLINK_REFERENCE(RendererFoundation_Device_Implementation_ImmutableSamplers);
  EZ_STATICLINK_REFERENCE(RendererFoundation_Device_Implementation_SharedTextureSwapChain);
  EZ_STATICLINK_REFERENCE(RendererFoundation_Device_Implementation_SwapChain);
  EZ_STATICLINK_REFERENCE(RendererFoundation_Profiling_Implementation_Profiling);
  EZ_STATICLINK_REFERENCE(RendererFoundation_RendererReflection);
}
