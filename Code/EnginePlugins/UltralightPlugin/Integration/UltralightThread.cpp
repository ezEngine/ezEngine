
#include <PCH.h>

#include <Foundation/Utilities/CommandLineUtils.h>

#include <RendererFoundation/Device/Device.h>

#include <UltralightPlugin/Integration/UltralightThread.h>
#include <UltralightPlugin/Integration/FileSystemWin.h>
#include <UltralightPlugin/Integration/FontLoaderWin.h>
#include <UltralightPlugin/Integration/GPUDriverEz.h>
#include <UltralightPlugin/Resources/UltralightHTMLResource.h>

static ezUltralightThread* s_pInstance = nullptr;

ezUltralightThread::ezUltralightThread()
  : ezThreadWithDispatcher("ezUltralightThread")
{
  s_pInstance = this;

  ezGALDevice::GetDefaultDevice()->m_Events.AddEventHandler(ezMakeDelegate(&ezUltralightThread::UpdateForRendering, this));
}

ezUltralightThread::~ezUltralightThread()
{
  s_pInstance = nullptr;
}

ezUInt32 ezUltralightThread::Run()
{
  ultralight::Config config;
  config.face_winding = ultralight::kFaceWinding_Clockwise; // CW in D3D, CCW in OGL
  config.device_scale_hint = 1.0;                           // Set DPI to monitor DPI scale
  config.font_family_standard = "Segoe UI";                 // Default font family
  config.force_repaint = ezCommandLineUtils::GetGlobalInstance()->GetBoolOption("-ultralight-force-repaint");

  ultralight::Platform::instance().set_config(config);
  ultralight::Platform::instance().set_file_system(new ultralight::FileSystemWin(L""));
  ultralight::Platform::instance().set_font_loader(new ultralight::FontLoaderWin);

  m_pGPUDriver = EZ_DEFAULT_NEW(ezUltralightGPUDriver);

  ultralight::Platform::instance().set_gpu_driver(m_pGPUDriver);

  m_pRenderer = ultralight::Renderer::Create();

  while (m_bRunning)
  {
    // Do pending resource deletions and registrations
    {
      EZ_LOCK(m_ResourceMutex);

      // First remove any pending deletions from the pending
      // registration queue (e.g. if they happened in the same frame)
      for (auto pResource : m_PendingResourceDeletions)
      {
        m_PendingResourceRegistrations.RemoveAndSwap(pResource);

        pResource->DestroyView();

        m_RegisteredResources.RemoveAndCopy(pResource);
        m_TextureHandles.Remove(pResource);
      }

      for (auto pResource : m_PendingResourceRegistrations)
      {
        pResource->CreateView(m_pRenderer.get());

        m_RegisteredResources.PushBack(pResource);

        m_TextureHandles.Insert(pResource, ezGALTextureHandle());
      }

      m_PendingResourceDeletions.Clear();
      m_PendingResourceRegistrations.Clear();
    }

    DispatchQueue();

    m_pRenderer->Update();

    for (auto pResource : m_RegisteredResources)
    {
      auto hTex = static_cast<ezUltralightGPUDriver*>(ultralight::Platform::instance().gpu_driver())->GetTextureHandleForTextureId(pResource->GetView()->render_target().texture_id);

      m_TextureHandles[pResource] = hTex;
    }

    m_Signal.WaitForSignal();
  }

  ezGALDevice::GetDefaultDevice()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezUltralightThread::UpdateForRendering, this));

  EZ_DEFAULT_DELETE(m_pGPUDriver);

  return 0;
}

void ezUltralightThread::DrawCommandLists()
{
  m_pGPUDriver->DrawCommandList();
}

void ezUltralightThread::Wake()
{
  m_Signal.RaiseSignal();
}

void ezUltralightThread::Quit()
{
  m_bRunning = false;
  Wake();
}

void ezUltralightThread::Register(ezUltralightHTMLResource* pResource)
{
  EZ_LOCK(m_ResourceMutex);

  m_PendingResourceRegistrations.PushBack(pResource);
}

void ezUltralightThread::Unregister(ezUltralightHTMLResource* pResource)
{
  EZ_LOCK(m_ResourceMutex);

  m_PendingResourceDeletions.PushBack(pResource);
}

ezUltralightThread* ezUltralightThread::GetInstance()
{
  return s_pInstance;
}

void ezUltralightThread::UpdateForRendering(const ezGALDeviceEvent& event)
{
  if (event.m_Type == ezGALDeviceEvent::AfterBeginFrame)
  {
    EZ_PROFILE_SCOPE("ezUltralightThread::UpdateForRendering::AfterBeginFrame");

    m_pGPUDriver->BeginSynchronize();
    m_pRenderer->Render();
    m_pGPUDriver->EndSynchronize();
  }
  else if (event.m_Type == ezGALDeviceEvent::BeforeBeginFrame)
  {
    EZ_LOCK(m_ResourceMutex);

    for (auto& Association : m_TextureHandles)
    {
      Association.Key()->SetTextureHandle(Association.Value());
    }
  }
}
