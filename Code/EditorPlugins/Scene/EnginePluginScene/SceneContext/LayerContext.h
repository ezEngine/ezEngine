#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EnginePluginScene/EnginePluginSceneDLL.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <SharedPluginScene/Common/Messages.h>


class EZ_ENGINEPLUGINSCENE_DLL ezLayerContext : public ezEngineProcessDocumentContext
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLayerContext, ezEngineProcessDocumentContext);

public:
  ezLayerContext();
  ~ezLayerContext();

  virtual void HandleMessage(const ezEditorEngineDocumentMsg* pMsg) override;

protected:
  virtual void OnInitialize() override;
  virtual void OnDeinitialize() override;

  virtual ezEngineProcessViewContext* CreateViewContext() override;
  virtual void DestroyViewContext(ezEngineProcessViewContext* pContext) override;
  virtual bool ExportDocument(const ezExportDocumentMsgToEngine* pMsg) override;

  virtual void UpdateDocumentContext() override;

private:
  ezSceneContext* m_pParentSceneContext = nullptr;
};
