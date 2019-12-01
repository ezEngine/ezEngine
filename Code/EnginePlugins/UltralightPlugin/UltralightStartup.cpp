#include <PCH.h>

#include <Foundation/Configuration/Startup.h>

#include <RendererFoundation/Device/Device.h>

#include <GameEngine/GameApplication/GameApplication.h>

#include <UltralightPlugin/Resources/UltralightHTMLResource.h>
#include <UltralightPlugin/Integration/FileSystem.h>
#include <UltralightPlugin/Integration/GPUDriverEz.h>

// TODO: These should be replaced with EZ implementations
#include <UltralightPlugin/Integration/FileSystemWin.h>
#include <UltralightPlugin/Integration/FontLoaderWin.h>

#include <Ultralight/Ultralight.h>

ultralight::RefPtr<ultralight::Renderer> s_renderer = nullptr;
static ezUltralightFileSystem* s_pFileSystem = nullptr;
static ezUltralightGPUDriver* s_pGPUDriver = nullptr;

static void UpdateUltralightRendering(const ezGALDeviceEvent& DeviceEvent)
{
  if (DeviceEvent.m_Type == ezGALDeviceEvent::AfterBeginFrame)
  {
    s_renderer->Update();

    s_pGPUDriver->BeginSynchronize();
    s_renderer->Render();
    s_pGPUDriver->EndSynchronize();

    if (s_pGPUDriver->HasCommandsPending())
    {
      s_pGPUDriver->DrawCommandList();
    }
  }
}

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(Ultralight, UltralightPlugin)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core",
    "RendererCore"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    ezResourceManager::RegisterResourceOverrideType(ezGetStaticRTTI<ezUltralightHTMLResource>(), [](const ezStringBuilder& sResourceID) -> bool  {
        return sResourceID.HasExtension(".ezUltralightHTML");
      });

  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    ezResourceManager::UnregisterResourceOverrideType(ezGetStaticRTTI<ezUltralightHTMLResource>());
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    if (!s_pFileSystem)
    {
      //s_fileSystem = EZ_DEFAULT_NEW(ezUltralightFileSystem);
    }

    if (!s_pGPUDriver)
    {
      s_pGPUDriver = EZ_DEFAULT_NEW(ezUltralightGPUDriver);
    }

    ultralight::Config config;
    config.face_winding = ultralight::kFaceWinding_Clockwise; // CW in D3D, CCW in OGL
    config.device_scale_hint = 1.0;               // Set DPI to monitor DPI scale
    config.font_family_standard = "Segoe UI";     // Default font family

    ultralight::Platform::instance().set_config(config);
    ultralight::Platform::instance().set_file_system(new ultralight::FileSystemWin(L""));
    ultralight::Platform::instance().set_font_loader(new ultralight::FontLoaderWin);

    //ultralight::Platform::instance().set_file_system(s_fileSystem);
    ultralight::Platform::instance().set_gpu_driver(s_pGPUDriver);

    s_renderer = ultralight::Renderer::Create();

    ezGALDevice::GetDefaultDevice()->m_Events.AddEventHandler(ezMakeDelegate(UpdateUltralightRendering));
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    ezGALDevice::GetDefaultDevice()->m_Events.RemoveEventHandler(ezMakeDelegate(UpdateUltralightRendering));

    s_renderer = nullptr;

    EZ_DEFAULT_DELETE(s_pGPUDriver);
    s_pGPUDriver = nullptr;

    EZ_DEFAULT_DELETE(s_pFileSystem);
    s_pFileSystem = nullptr;
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

EZ_STATICLINK_FILE(UltralightPlugin, UltralightPlugin_UltralightStartup);
