#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/EditTools/EditTool.h>

struct ezEngineWindowEvent;
struct ezGameObjectEvent;
struct ezDocumentObjectStructureEvent;
struct ezManipulatorManagerEvent;
struct ezSelectionManagerEvent;
struct ezCommandHistoryEvent;
struct ezGizmoEvent;

class EZ_EDITORFRAMEWORK_DLL ezGameObjectGizmoEditTool : public ezGameObjectEditTool
{
  EZ_ADD_DYNAMIC_REFLECTION(ezGameObjectGizmoEditTool, ezGameObjectEditTool);

public:
  ezGameObjectGizmoEditTool();
  ~ezGameObjectGizmoEditTool();

  void TransformationGizmoEventHandler(const ezGizmoEvent& e);

protected:
  virtual void OnConfigured() override;

  void UpdateGizmoSelectionList();

  void UpdateGizmoVisibleState();
  virtual void ApplyGizmoVisibleState(bool visible) = 0;

  void UpdateGizmoTransformation();
  virtual void ApplyGizmoTransformation(const ezTransform& transform) = 0;

  virtual void TransformationGizmoEventHandlerImpl(const ezGizmoEvent& e) = 0;

  ezDeque<ezSelectedGameObject> m_GizmoSelection;
  bool m_bInGizmoInteraction = false;
  bool m_bMergeTransactions = false;

private:
  void DocumentWindowEventHandler(const ezQtDocumentWindowEvent& e);
  void UpdateManipulatorVisibility();
  void GameObjectEventHandler(const ezGameObjectEvent& e);
  void CommandHistoryEventHandler(const ezCommandHistoryEvent& e);
  void SelectionManagerEventHandler(const ezSelectionManagerEvent& e);
  void ManipulatorManagerEventHandler(const ezManipulatorManagerEvent& e);
  void EngineWindowEventHandler(const ezEngineWindowEvent& e);
  void ObjectStructureEventHandler(const ezDocumentObjectStructureEvent& e);
};

