#pragma once

#include <RendererCore/Textures/Texture2DResource.h>
#include <UltralightPlugin/Basics.h>
#include <Ultralight/Ultralight.h>

struct ezGALDeviceEvent;

struct EZ_ULTRALIGHTPLUGIN_DLL ezUltralightHTMLResourceDescriptor
{
  ezString m_sHTMLContent;
  ezString m_sHTMLFileName;

  ezUInt16 m_uiWidth = 512;
  ezUInt16 m_uiHeight = 512;

  void Save(ezStreamWriter& stream) const;
  void Load(ezStreamReader& stream);
};

typedef ezTypedResourceHandle<class ezSkeletonResource> ezSkeletonResourceHandle;

class EZ_ULTRALIGHTPLUGIN_DLL ezUltralightHTMLResource : public ezTexture2DResource, public ultralight::LoadListener, public ultralight::ViewListener
{
  EZ_ADD_DYNAMIC_REFLECTION(ezUltralightHTMLResource, ezTexture2DResource);

  EZ_RESOURCE_DECLARE_COMMON_CODE(ezUltralightHTMLResource);
  EZ_RESOURCE_DECLARE_CREATEABLE(ezUltralightHTMLResource, ezUltralightHTMLResourceDescriptor);

public:
  ezUltralightHTMLResource();
  ~ezUltralightHTMLResource();

  const ezUltralightHTMLResourceDescriptor& GetDescriptor() const { return m_Descriptor; }

  // ultralight::LoadListener methods
  virtual void OnBeginLoading(ultralight::View* caller) override;
  virtual void OnFinishLoading(ultralight::View* caller) override;
  virtual void OnUpdateHistory(ultralight::View* caller) override;
  virtual void OnDOMReady(ultralight::View* caller) override;

  // ultralight::ViewListener methods
  virtual void OnChangeTitle(ultralight::View* caller, const ultralight::String& title) override;
  virtual void OnChangeURL(ultralight::View* caller, const ultralight::String& url) override;
  virtual void OnChangeTooltip(ultralight::View* caller, const ultralight::String& tooltip) override;
  virtual void OnChangeCursor(ultralight::View* caller, ultralight::Cursor cursor) override;
  virtual void OnAddConsoleMessage(ultralight::View* caller, ultralight::MessageSource source, ultralight::MessageLevel level, const ultralight::String& message, uint32_t line_number, uint32_t column_number, const ultralight::String& source_id) override;

private:
  virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  void HACK_UPDATE_METHOD(const ezGALDeviceEvent& event);

  ezUltralightHTMLResourceDescriptor m_Descriptor;

  ultralight::RefPtr<ultralight::View> m_View;
};
