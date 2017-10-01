#pragma once

#include <EnginePluginScene/Plugin.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>

class ezObjectSelectionMsgToEngine;
class ezRenderContext;
class ezGameState;
class ezGameModeMsgToEngine;
class ezSceneSettingsMsgToEngine;
class ezObjectsForDebugVisMsgToEngine;
struct ezVisualScriptComponentActivityEvent;

class EZ_ENGINEPLUGINSCENE_DLL ezSceneContext : public ezEngineProcessDocumentContext
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSceneContext, ezEngineProcessDocumentContext);

public:
  ezSceneContext();
  ~ezSceneContext();

  virtual void HandleMessage(const ezEditorEngineDocumentMsg* pMsg) override;

  const ezDeque<ezGameObjectHandle>& GetSelection() const { return m_Selection; }
  const ezDeque<ezGameObjectHandle>& GetSelectionWithChildren() const { return m_SelectionWithChildren; }
  bool GetRenderSelectionOverlay() const { return m_bRenderSelectionOverlay; }
  bool GetRenderShapeIcons() const { return m_bRenderShapeIcons; }
  float GetGridDensity() const { return ezMath::Abs(m_fGridDensity); }
  bool IsGridInGlobalSpace() const { return m_fGridDensity >= 0.0f; }
  ezTransform GetGridTransform() const { return m_GridTransform; }

  ezGameState* GetGameState() const;
  bool IsPlayTheGameActive() const { return GetGameState() != nullptr; }

protected:
  virtual void OnInitialize() override;
  virtual void OnDeinitialize() override;

  virtual ezEngineProcessViewContext* CreateViewContext() override;
  virtual void DestroyViewContext(ezEngineProcessViewContext* pContext) override;
  virtual bool ExportDocument(const ezExportDocumentMsgToEngine* pMsg) override;
  virtual bool UpdateThumbnailViewContext(ezEngineProcessViewContext* pThumbnailViewContext) override;
  virtual void OnThumbnailViewContextCreated() override;
  virtual void OnDestroyThumbnailViewContext() override;
  virtual void UpdateDocumentContext() override;

private:
  void AddAmbientLight(bool bSetEditorTag);
  void RemoveAmbientLight();
  void HandleSelectionMsg(const ezObjectSelectionMsgToEngine* pMsg);
  void HandleGameModeMsg(const ezGameModeMsgToEngine* pMsg);
  void HandleSceneSettingsMsg(const ezSceneSettingsMsgToEngine* msg);
  void HandleObjectsForDebugVisMsg(const ezObjectsForDebugVisMsgToEngine* pMsg);
  void ComputeHierarchyBounds(ezGameObject* pObj, ezBoundingBoxSphere& bounds);

  void DrawSelectionBounds();

  void InsertSelectedChildren(const ezGameObject* pObject);
  void QuerySelectionBBox(const ezEditorEngineDocumentMsg* pMsg);
  void OnSimulationEnabled();
  void OnSimulationDisabled();
  void OnPlayTheGameModeStarted();

  void OnVisualScriptActivity(const ezVisualScriptComponentActivityEvent& e);

  bool m_bRenderSelectionOverlay;
  bool m_bRenderShapeIcons;
  bool m_bRenderSelectionBoxes;
  float m_fGridDensity;
  ezTransform m_GridTransform;

  ezDeque<ezGameObjectHandle> m_Selection;
  ezDeque<ezGameObjectHandle> m_SelectionWithChildren;
  ezSet<ezGameObjectHandle> m_SelectionWithChildrenSet;
  ezGameObjectHandle m_hAmbientLight[3];
};


