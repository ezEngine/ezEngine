#pragma once

#include <EnginePluginScene/Plugin.h>
#include <EditorFramework/EngineProcess/EngineProcessDocumentContext.h>

class ezObjectSelectionMsgToEngine;
class ezRenderContext;
class ezGameState;

class EZ_ENGINEPLUGINSCENE_DLL ezSceneContext : public ezEngineProcessDocumentContext
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSceneContext, ezEngineProcessDocumentContext);

public:
  ezSceneContext();

  virtual void HandleMessage(const ezEditorEngineDocumentMsg* pMsg) override;

  const ezDeque<ezGameObjectHandle>& GetSelection() const { return m_Selection; }
  const ezDeque<ezGameObjectHandle>& GetSelectionWithChildren() const { return m_SelectionWithChildren; }
  bool GetRenderSelectionOverlay() const { return m_bRenderSelectionOverlay; }
  bool GetRenderShapeIcons() const { return m_bRenderShapeIcons; }
  
  ezGameState* GetGameState() const;

protected:
  virtual void OnInitialize() override;

  virtual ezEngineProcessViewContext* CreateViewContext() override;
  virtual void DestroyViewContext(ezEngineProcessViewContext* pContext) override;
  virtual bool ExportDocument(const ezExportDocumentMsgToEngine* pMsg) override;
  virtual bool UpdateThumbnailViewContext(ezEngineProcessViewContext* pThumbnailViewContext) override;
  virtual void OnThumbnailViewContextCreated() override;
  virtual void OnDestroyThumbnailViewContext() override;

private:
  
  void HandleSelectionMsg(const ezObjectSelectionMsgToEngine* pMsg);
  void HandleGameModeMsg(const ezGameModeMsgToEngine* pMsg);
  void ComputeHierarchyBounds(ezGameObject* pObj, ezBoundingBoxSphere& bounds);
  
  void DrawSelectionBounds();

  void InsertSelectedChildren(const ezGameObject* pObject);
  void QuerySelectionBBox(const ezEditorEngineDocumentMsg* pMsg);
  void OnSimulationEnabled();
  void OnSimulationDisabled();
  void OnPlayTheGameModeStarted();

  bool m_bRenderSelectionOverlay;
  bool m_bRenderShapeIcons;
  bool m_bRenderSelectionBoxes;

  ezDeque<ezGameObjectHandle> m_Selection;
  ezDeque<ezGameObjectHandle> m_SelectionWithChildren;
  ezSet<ezGameObjectHandle> m_SelectionWithChildrenSet;
  ezGameObjectHandle m_hThumbnailLights;
};


