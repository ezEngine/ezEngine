
#include <PCH.h>

#include <Foundation/Utilities/CommandLineUtils.h>

#include <RendererFoundation/Device/Device.h>

#include <UltralightPlugin/Integration/GPUDriverEz.h>
#include <UltralightPlugin/Integration/UltralightFileSystem.h>
#include <UltralightPlugin/Integration/UltralightResourceManager.h>
#include <UltralightPlugin/Resources/UltralightHTMLResource.h>

static ezUltralightResourceManager* s_pInstance = nullptr;

ezUltralightResourceManager::ezUltralightResourceManager()
{
  s_pInstance = this;
}

ezUltralightResourceManager::~ezUltralightResourceManager()
{
  s_pInstance = nullptr;
}

void ezUltralightResourceManager::Update(ultralight::Renderer* pRenderer)
{
  EZ_ASSERT_DEV(ezThreadUtils::IsMainThread(), "Ultralight operations need to happen on the mainthread");

  // Do pending resource deletions and registrations
  {
    EZ_LOCK(m_ResourceMutex);

    // First remove any pending deletions from the pending
    // registration queue (e.g. if they happened in the same frame)
    for (auto pResource : m_PendingResourceDeletions)
    {
      m_PendingResourceRegistrations.RemoveAndSwap(pResource);
      m_ResourcesWhichNeedUpdates.RemoveAndSwap(pResource);

      pResource->DestroyView();

      m_RegisteredResources.RemoveAndCopy(pResource);
    }

    for (auto pResource : m_PendingResourceRegistrations)
    {
      pResource->CreateView(pRenderer);

      m_RegisteredResources.PushBack(pResource);
    }

    for (auto pResource : m_ResourcesWhichNeedUpdates)
    {
      pResource->DestroyView();
      pResource->CreateView(pRenderer);
    }

    m_PendingResourceDeletions.Clear();
    m_PendingResourceRegistrations.Clear();
    m_ResourcesWhichNeedUpdates.Clear();
  }

  for (auto pResource : m_RegisteredResources)
  {
    auto hTex = static_cast<ezUltralightGPUDriver*>(ultralight::Platform::instance().gpu_driver())
                  ->GetTextureHandleForTextureId(pResource->GetView()->render_target().texture_id);
    pResource->SetTextureHandle(hTex);
  }
}

void ezUltralightResourceManager::Register(ezUltralightHTMLResource* pResource)
{
  EZ_LOCK(m_ResourceMutex);

  EZ_ASSERT_DEV(m_RegisteredResources.IndexOf(pResource) == ezInvalidIndex, "");

  if (m_PendingResourceRegistrations.IndexOf(pResource) == ezInvalidIndex)
    m_PendingResourceRegistrations.PushBack(pResource);
}

void ezUltralightResourceManager::Unregister(ezUltralightHTMLResource* pResource)
{
  EZ_LOCK(m_ResourceMutex);

  EZ_ASSERT_DEV(m_RegisteredResources.IndexOf(pResource) != ezInvalidIndex, "");

  if (m_PendingResourceDeletions.IndexOf(pResource) == ezInvalidIndex)
    m_PendingResourceDeletions.PushBack(pResource);
}

bool ezUltralightResourceManager::IsRegistered(ezUltralightHTMLResource* pResource) const
{
  EZ_LOCK(m_ResourceMutex);

  return m_RegisteredResources.IndexOf(pResource) != ezInvalidIndex;
}

void ezUltralightResourceManager::UpdateResource(ezUltralightHTMLResource* pResource)
{
  EZ_LOCK(m_ResourceMutex);

  EZ_ASSERT_DEV(m_RegisteredResources.IndexOf(pResource) != ezInvalidIndex, "");
  EZ_ASSERT_DEV(m_ResourcesWhichNeedUpdates.IndexOf(pResource) == ezInvalidIndex, "");

  m_ResourcesWhichNeedUpdates.PushBack(pResource);
}

ezUltralightResourceManager* ezUltralightResourceManager::GetInstance()
{
  return s_pInstance;
}
