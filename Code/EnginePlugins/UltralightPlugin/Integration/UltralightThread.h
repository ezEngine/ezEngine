
#pragma once

#include <UltralightPlugin/Basics.h>

#include <Foundation/Threading/ThreadWithDispatcher.h>
#include <Foundation/Threading/ThreadSignal.h>
#include <Foundation/Types/UniquePtr.h>

#include <Ultralight/Ultralight.h>

class ezUltralightGPUDriver;
class ezUltralightFileSystem;
class ezUltralightHTMLResource;
struct ezGALDeviceEvent;

class EZ_ULTRALIGHTPLUGIN_DLL ezUltralightThread : public ezThreadWithDispatcher
{
  public:

    ezUltralightThread();
    ~ezUltralightThread();

    virtual ezUInt32 Run() override;

    void DrawCommandLists();

    void Wake();

    void Quit();

    void Register(ezUltralightHTMLResource* pResource);
    void Unregister(ezUltralightHTMLResource* pResource);

    static ezUltralightThread* GetInstance();

    static void AssertUltralightThread();

  private:

    void UpdateForRendering(const ezGALDeviceEvent& event);

    ezThreadSignal m_Signal;
    bool m_bRunning = true;

    ultralight::RefPtr<ultralight::Renderer> m_pRenderer;
    ezUniquePtr<ezUltralightGPUDriver> m_pGPUDriver;
    ezUniquePtr<ezUltralightFileSystem> m_pFileSystem;

    ezMutex m_ResourceMutex;
    ezDynamicArray<ezUltralightHTMLResource*> m_PendingResourceRegistrations;
    ezDynamicArray<ezUltralightHTMLResource*> m_PendingResourceDeletions;
    ezDynamicArray<ezUltralightHTMLResource*> m_RegisteredResources;

    ezMap<ezUltralightHTMLResource*, ezGALTextureHandle> m_TextureHandles;

};


