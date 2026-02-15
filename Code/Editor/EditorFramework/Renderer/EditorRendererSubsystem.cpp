#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Renderer/EditorRendererSubsystem.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Memory/Allocator.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Types/UniquePtr.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/DeviceFactory.h>

#ifdef BUILDSYSTEM_ENGINE_PROCESS_SHARED_TEXTURE
namespace
{
  ezUniquePtr<ezGALDevice> g_DeviceInstance;
  ezSet<ezEngineViewWindow*> g_ViewWindows;
  ezMutex g_ViewWindowsMutex;
} // namespace
#endif

ezGALDevice* ezEditorRendererSubsystem::s_pDevice = nullptr;

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(EditorFramework, EditorRendererSubsystem)
  BEGIN_SUBSYSTEM_DEPENDENCIES
    "EditorFrameworkMain"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    ezQtEditorApp::m_Events.AddEventHandler(ezMakeDelegate(&ezEditorRendererSubsystem::EditorAppEventHandler));
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    ezQtEditorApp::m_Events.RemoveEventHandler(ezMakeDelegate(&ezEditorRendererSubsystem::EditorAppEventHandler));
    ezEditorRendererSubsystem::DestroyDevice();
  }
EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

ezGALDevice* ezEditorRendererSubsystem::GetDevice()
{
  return s_pDevice;
}

void ezEditorRendererSubsystem::RegisterViewWindow(ezEngineViewWindow* pWindow)
{
#if defined(BUILDSYSTEM_ENGINE_PROCESS_SHARED_TEXTURE)
  EZ_LOCK(g_ViewWindowsMutex);
  g_ViewWindows.Insert(pWindow);
#else
  EZ_IGNORE_UNUSED(pWindow);
#endif
}

void ezEditorRendererSubsystem::UnregisterViewWindow(ezEngineViewWindow* pWindow)
{
#if defined(BUILDSYSTEM_ENGINE_PROCESS_SHARED_TEXTURE)
  EZ_LOCK(g_ViewWindowsMutex);
  g_ViewWindows.Remove(pWindow);
#else
  EZ_IGNORE_UNUSED(pWindow);
#endif
}

void ezEditorRendererSubsystem::EditorAppEventHandler(const ezEditorAppEvent& e)
{
  switch (e.m_Type)
  {
    case ezEditorAppEvent::Type::AfterCoreSystemStartup:
      CreateDevice();
      break;
    case ezEditorAppEvent::Type::BeforeCoreSystemShutdown:
      DestroyDevice();
      break;
    default:
      break;
  }
}

void ezEditorRendererSubsystem::CreateDevice()
{
#if defined(BUILDSYSTEM_ENGINE_PROCESS_SHARED_TEXTURE)
  if (ezGALDevice::HasDefaultDevice())
  {
    s_pDevice = ezGALDevice::GetDefaultDevice();
    return;
  }

#  ifdef BUILDSYSTEM_ENABLE_VULKAN_SUPPORT
  constexpr const char* szDefaultRenderer = "Vulkan";
#  else
  constexpr const char* szDefaultRenderer = "DX11";
#  endif

  const ezStringView sRendererName = ezCommandLineUtils::GetGlobalInstance()->GetStringOption("-renderer", 0, szDefaultRenderer);
  const char* szShaderModel = "";
  const char* szShaderCompiler = "";
  ezGALDeviceFactory::GetShaderModelAndCompiler(sRendererName, szShaderModel, szShaderCompiler);

  ezShaderManager::Configure(szShaderModel, true);
  ezPlugin::LoadPlugin(szShaderCompiler).IgnoreResult();

  ezGALDeviceCreationDescription deviceInit;
  deviceInit.m_bDebugDevice = false;

  g_DeviceInstance = ezGALDeviceFactory::CreateDevice(sRendererName, ezFoundation::GetDefaultAllocator(), deviceInit);
  if (g_DeviceInstance.Borrow() != nullptr && g_DeviceInstance->Init().Succeeded())
  {
    s_pDevice = g_DeviceInstance.Borrow();
    ezGALDevice::SetDefaultDevice(s_pDevice);

    ezRenderContext::OnEngineStartup();
    return;
  }

  ezLog::Error("Failed to initialize the editor process render device for shared textures");
  if (g_DeviceInstance.Borrow() != nullptr)
  {
    g_DeviceInstance->Shutdown().IgnoreResult();
  }
  g_DeviceInstance = nullptr;
  s_pDevice = nullptr;
#endif
}

void ezEditorRendererSubsystem::DestroyDevice()
{
#if defined(BUILDSYSTEM_ENGINE_PROCESS_SHARED_TEXTURE)
  {
    EZ_LOCK(g_ViewWindowsMutex);
    EZ_ASSERT_DEV(g_ViewWindows.IsEmpty(), "All ezEngineViewWindow instances must be destroyed before the renderer device shuts down.");
  }

  if (g_DeviceInstance.Borrow() == nullptr)
  {
    s_pDevice = nullptr;
    return;
  }

  ezRenderContext* pRenderContext = ezRenderContext::GetDefaultInstance();
  ezRenderContext::DestroyInstance(pRenderContext);

  ezRenderContext::OnEngineShutdown();
  ezResourceManager::FreeAllUnusedResources();

  g_DeviceInstance->WaitIdle();
  g_DeviceInstance->Shutdown().IgnoreResult();
  g_DeviceInstance = nullptr;
  s_pDevice = nullptr;
  ezGALDevice::SetDefaultDevice(nullptr);

#endif
}
