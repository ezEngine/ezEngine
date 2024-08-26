#include <RendererDX11/RendererDX11PCH.h>

EZ_STATICLINK_LIBRARY(RendererDX11)
{
  if (bReturn)
    return;

  EZ_STATICLINK_REFERENCE(RendererDX11_Device_Implementation_DeviceDX11);
}
