
#pragma once

#include <UltralightPlugin/Basics.h>

#include <Foundation/Threading/ThreadWithDispatcher.h>
#include <Foundation/Threading/ThreadSignal.h>
#include <Foundation/Types/UniquePtr.h>

#include <RendererFoundation/Resources/Texture.h>

#include <Ultralight/Ultralight.h>

class ezUltralightHTMLResource;

/// \brief A helper class to manage deferred Ultralight view creations etc.
class EZ_ULTRALIGHTPLUGIN_DLL ezUltralightResourceManager final
{
public:

  ezUltralightResourceManager();
  ~ezUltralightResourceManager();

  void Update(ultralight::Renderer* pRenderer);

  static ezUltralightResourceManager* GetInstance();

private:
  friend ezUltralightHTMLResource;

  void Register(ezUltralightHTMLResource* pResource);
  void Unregister(ezUltralightHTMLResource* pResource);

  bool IsRegistered(ezUltralightHTMLResource* pResource) const;

  void UpdateResource(ezUltralightHTMLResource* pResource);

private:

    mutable ezMutex m_ResourceMutex;
    ezDynamicArray<ezUltralightHTMLResource*> m_PendingResourceRegistrations;
    ezDynamicArray<ezUltralightHTMLResource*> m_PendingResourceDeletions;
    ezDynamicArray<ezUltralightHTMLResource*> m_RegisteredResources;
    ezDynamicArray<ezUltralightHTMLResource*> m_ResourcesWhichNeedUpdates;
};
