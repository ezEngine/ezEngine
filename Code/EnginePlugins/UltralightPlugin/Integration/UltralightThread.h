
#pragma once

#include <UltralightPlugin/Basics.h>

#include <Foundation/Threading/ThreadWithDispatcher.h>
#include <Foundation/Threading/ThreadSignal.h>

#include <Ultralight/Ultralight.h>

class ezUltralightGPUDriver;
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

  private:

    void UpdateForRendering(const ezGALDeviceEvent& event);

    ezThreadSignal m_Signal;
    bool m_bRunning = true;

    ultralight::RefPtr<ultralight::Renderer> m_pRenderer;
    ezUltralightGPUDriver* m_pGPUDriver = nullptr;

    ezMutex m_ResourceMutex;
    ezDynamicArray<ezUltralightHTMLResource*> m_PendingResourceRegistrations;
    ezDynamicArray<ezUltralightHTMLResource*> m_PendingResourceDeletions;
    ezDynamicArray<ezUltralightHTMLResource*> m_RegisteredResources;

    ezMap<ezUltralightHTMLResource*, ezGALTextureHandle> m_TextureHandles;

};


