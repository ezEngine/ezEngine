#pragma once

#include <RmlUiPlugin/Resources/RmlUiResource.h>
#include <RmlUiPlugin/RmlUiPluginDLL.h>

#include <RmlUi/Include/RmlUi/Core.h>

#include <Foundation/Types/UniquePtr.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class ezBlackboard;

namespace ezRmlUiInternal
{
  class RenderInterface;
  class EventListener;
} // namespace ezRmlUiInternal

class EZ_RMLUIPLUGIN_DLL ezRmlUiContext final : public Rml::Context
{
public:
  ezRmlUiContext(const Rml::String& sName, Rml::RenderManager* render_manager, Rml::TextInputHandler* text_input_handler);
  ~ezRmlUiContext();

public:
  ezResult LoadDocumentFromResource(const ezRmlUiResourceHandle& hResource);
  ezResult LoadDocumentFromString(const ezStringView& sContent);

  void UnloadDocument();
  ezResult ReloadDocumentFromResource(const ezRmlUiResourceHandle& hResource);

  void ShowDocument();
  void HideDocument();

  void UpdateInput(const ezVec2& vMousePos);
  bool WantsInput() const { return m_bWantsInput; }

  void SetSize(const ezVec2U32& vSize);
  void SetDpiScale(float fScale);

  using EventHandler = ezDelegate<void(Rml::Event&)>;

  void RegisterEventHandler(const char* szIdentifier, EventHandler handler);
  void DeregisterEventHandler(const char* szIdentifier);

private:
  bool HasDocument() { return GetNumDocuments() > 0; }

  friend class ezRmlUi;
  void ExtractRenderData(ezRmlUiInternal::RenderInterface& renderInterface, ezGALTextureHandle hTexture);

  friend class ezRmlUiInternal::EventListener;
  void ProcessEvent(const ezHashedString& sIdentifier, Rml::Event& event);

  ezHashTable<ezHashedString, EventHandler> m_EventHandler;

  ezUInt64 m_uiExtractedFrame = 0;

  bool m_bWantsInput = false;
};

namespace ezRmlUiInternal
{
  class ContextInstancer : public Rml::ContextInstancer
  {
  public:
    virtual Rml::ContextPtr InstanceContext(const Rml::String& sName, Rml::RenderManager* render_manager, Rml::TextInputHandler* text_input_handler) override;
    virtual void ReleaseContext(Rml::Context* pContext) override;

  private:
    virtual void Release() override;
  };
} // namespace ezRmlUiInternal
