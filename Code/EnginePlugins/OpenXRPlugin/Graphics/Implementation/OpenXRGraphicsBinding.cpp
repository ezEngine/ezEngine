#include <OpenXRPlugin/OpenXRPluginPCH.h>

#include <OpenXRPlugin/Graphics/OpenXRGraphicsBinding.h>
#include <OpenXRPlugin/Graphics/OpenXRGraphicsBindingD3D11.h>
#include <OpenXRPlugin/Graphics/OpenXRGraphicsBindingVulkan.h>

#include <Foundation/Logging/Log.h>
#include <RendererFoundation/Device/Device.h>

ezUniquePtr<ezOpenXRGraphicsBinding> ezOpenXRGraphicsBinding::Create(ezOpenXR* pOpenXR, ezGALDevice* pDevice)
{
  const ezStringView sRenderer = pDevice->GetRenderer();
#ifdef BUILDSYSTEM_ENABLE_VULKAN_SUPPORT
  if (sRenderer == "Vulkan")
  {
    ezLog::Info("OpenXR: Creating Vulkan graphics binding");
    return EZ_DEFAULT_NEW(ezOpenXRGraphicsBindingVulkan, pOpenXR);
  }
#endif

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  if (sRenderer == "DX11")
  {
    ezLog::Info("OpenXR: Creating D3D11 graphics binding");
    return EZ_DEFAULT_NEW(ezOpenXRGraphicsBindingD3D11);
  }
#endif

  ezLog::Error("OpenXR: No graphics binding available for renderer '{}'", sRenderer);
  return nullptr;
}
