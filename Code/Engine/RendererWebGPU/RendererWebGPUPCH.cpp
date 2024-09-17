#include <RendererWebGPU/RendererWebGPUPCH.h>

#include <RendererWebGPU/RendererWebGPUDLL.h>

bool g_bEzTraceEnabled = true;

EZ_RENDERERWEBGPU_DLL void EnableWebGPUTrace(bool bEnable)
{
  g_bEzTraceEnabled = bEnable;
}

EZ_RENDERERWEBGPU_DLL bool IsWebGPUTraceEnabled()
{
  return g_bEzTraceEnabled;
}

EZ_STATICLINK_LIBRARY(RendererWebGPU)
{
  if (bReturn)
    return;

  EZ_STATICLINK_REFERENCE(RendererWebGPU_Device_Implementation_DeviceWebGPU);
}
