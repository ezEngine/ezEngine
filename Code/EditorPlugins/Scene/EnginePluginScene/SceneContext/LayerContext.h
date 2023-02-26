#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EnginePluginScene/EnginePluginSceneDLL.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <SharedPluginScene/Common/Messages.h>

class ezDocumentOpenMsgToEngine;

/// \brief Layers that are loaded as sub-documents of a scene share the ezWorld with their main document scene. Thus, this context attaches itself to its parent ezSceneContext.
class EZ_ENGINEPLUGINSCENE_DLL ezLayerContext : public ezEngineProcessDocumentContext
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLayerContext, ezEngineProcessDocumentContext);

public:
  static ezEngineProcessDocumentContext* AllocateContext(const ezDocumentOpenMsgToEngine* pMsg);
  ezLayerContext();
  ~ezLayerContext();

  virtual void HandleMessage(const ezEditorEngineDocumentMsg* pMsg) override;
  void SceneDeinitialized();
  const ezTag& GetLayerTag() const;

protected:
  virtual void OnInitialize() override;
  virtual void OnDeinitialize() override;

  virtual ezEngineProcessViewContext* CreateViewContext() override;
  virtual void DestroyViewContext(ezEngineProcessViewContext* pContext) override;
  virtual ezStatus ExportDocument(const ezExportDocumentMsgToEngine* pMsg) override;

  virtual void UpdateDocumentContext() override;

private:
  ezSceneContext* m_pParentSceneContext = nullptr;
  ezTag m_LayerTag;
};
