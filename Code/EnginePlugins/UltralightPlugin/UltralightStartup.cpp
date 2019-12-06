#include <PCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Utilities/CommandLineUtils.h>

#include <RendererFoundation/Device/Device.h>

#include <GameEngine/GameApplication/GameApplication.h>

#include <UltralightPlugin/Resources/UltralightHTMLResource.h>

#include <UltralightPlugin/Integration/UltralightThread.h>

static ezUltralightThread* s_pThread = nullptr;

static void UpdateUltralightRendering(const ezGALDeviceEvent& DeviceEvent)
{
  if (DeviceEvent.m_Type == ezGALDeviceEvent::BeforeBeginFrame)
  {
    s_pThread->Wake();
  }

  if (DeviceEvent.m_Type == ezGALDeviceEvent::AfterBeginFrame)
  {
    if (s_pThread->IsRunning())
    {
      s_pThread->DrawCommandLists();
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
    s_pThread = EZ_DEFAULT_NEW(ezUltralightThread);
    s_pThread->Start();

    ezGALDevice::GetDefaultDevice()->m_Events.AddEventHandler(ezMakeDelegate(UpdateUltralightRendering));
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    ezGALDevice::GetDefaultDevice()->m_Events.RemoveEventHandler(ezMakeDelegate(UpdateUltralightRendering));

    s_pThread->Quit();
    s_pThread->Join();

    EZ_DEFAULT_DELETE(s_pThread);
    s_pThread = nullptr;
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

EZ_STATICLINK_FILE(UltralightPlugin, UltralightPlugin_UltralightStartup);
