#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EnginePluginRmlUi/EnginePluginRmlUiDLL.h>
#include <RmlUiPlugin/Components/RmlUiCanvas2DComponent.h>

class ezObjectSelectionMsgToEngine;
class ezRenderContext;

class EZ_ENGINEPLUGINRMLUI_DLL ezRmlUiDocumentContext : public ezEngineProcessDocumentContext
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRmlUiDocumentContext, ezEngineProcessDocumentContext);

public:
  ezRmlUiDocumentContext();
  ~ezRmlUiDocumentContext();

  const ezRmlUiResourceHandle& GetResource() const { return m_hMainResource; }

protected:
  virtual void OnInitialize() override;

  virtual ezEngineProcessViewContext* CreateViewContext() override;
  virtual void DestroyViewContext(ezEngineProcessViewContext* pContext) override;
  virtual bool UpdateThumbnailViewContext(ezEngineProcessViewContext* pThumbnailViewContext) override;

private:
  ezGameObject* m_pMainObject = nullptr;
  ezRmlUiResourceHandle m_hMainResource;
};
