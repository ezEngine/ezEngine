#include <PCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Utilities/CommandLineUtils.h>

#include <RendererFoundation/Device/Device.h>

#include <UltralightPlugin/Integration/GPUDriverEz.h>
#include <UltralightPlugin/Integration/UltralightFileSystem.h>
#include <UltralightPlugin/Integration/UltralightResourceManager.h>
#include <UltralightPlugin/Resources/UltralightHTMLResource.h>

#include <GameEngine/GameApplication/GameApplication.h>

#include <Ultralight/Ultralight.h>


ultralight::RefPtr<ultralight::Renderer> s_pRenderer = nullptr;
static ezUltralightFileSystem* s_pFileSystem = nullptr;
static ezUltralightGPUDriver* s_pGPUDriver = nullptr;

static ezUltralightResourceManager* s_pResourceManager = nullptr;

static void UpdateUltralightRendering(const ezGALDeviceEvent& DeviceEvent)
{
  EZ_PROFILE_SCOPE("ezUltralightThread::UpdateForRendering::AfterBeginFrame");

  if (DeviceEvent.m_Type == ezGALDeviceEvent::BeforeBeginFrame)
  {
    ezUltralightResourceManager::GetInstance()->Update(s_pRenderer.get());
  }

  if (DeviceEvent.m_Type == ezGALDeviceEvent::AfterBeginFrame)
  {
    s_pRenderer->Update();

    s_pGPUDriver->BeginSynchronize();
    s_pRenderer->Render();
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
    if (!s_pResourceManager)
    {
      s_pResourceManager = EZ_DEFAULT_NEW(ezUltralightResourceManager);
    }

    if (!s_pFileSystem)
    {
      s_pFileSystem = EZ_DEFAULT_NEW(ezUltralightFileSystem);
    }

    if (!s_pGPUDriver)
    {
      s_pGPUDriver = EZ_DEFAULT_NEW(ezUltralightGPUDriver);
    }

    ultralight::Config config;
    config.face_winding = ultralight::kFaceWinding_Clockwise; // CW in D3D, CCW in OGL
    config.device_scale_hint = 1.0;                           // Set DPI to monitor DPI scale
    config.font_family_standard = "Segoe UI";                 // Default font family
    config.force_repaint = ezCommandLineUtils::GetGlobalInstance()->GetBoolOption("-ultralight-force-repaint");

    ultralight::Platform::instance().set_config(config);
    ultralight::Platform::instance().set_file_system(s_pFileSystem);

    ultralight::Platform::instance().set_gpu_driver(s_pGPUDriver);

    s_pRenderer = ultralight::Renderer::Create();

    ezGALDevice::GetDefaultDevice()->m_Events.AddEventHandler(ezMakeDelegate(UpdateUltralightRendering));
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    ezGALDevice::GetDefaultDevice()->m_Events.RemoveEventHandler(ezMakeDelegate(UpdateUltralightRendering));

    s_pRenderer = nullptr;

    EZ_DEFAULT_DELETE(s_pGPUDriver);
    s_pGPUDriver = nullptr;

    EZ_DEFAULT_DELETE(s_pFileSystem);
    s_pFileSystem = nullptr;

    EZ_DEFAULT_DELETE(s_pResourceManager);
    s_pResourceManager = nullptr;
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

EZ_STATICLINK_FILE(UltralightPlugin, UltralightPlugin_UltralightStartup);
