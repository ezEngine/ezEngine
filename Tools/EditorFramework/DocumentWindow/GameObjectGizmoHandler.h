#pragma once

#include <EditorFramework/Plugin.h>
#include <EditorFramework/Gizmos/TranslateGizmo.h>
#include <EditorFramework/Gizmos/RotateGizmo.h>
#include <EditorFramework/Gizmos/ScaleGizmo.h>
#include <EditorFramework/Gizmos/DragToPositionGizmo.h>

class ezGameObjectDocument;
class ezQtGameObjectDocumentWindow;
class ezDocumentObject;
class ezObjectAccessorBase;
struct ezEngineWindowEvent;
struct ezGameObjectEvent;
struct ezDocumentObjectStructureEvent;
struct ezManipulatorManagerEvent;
struct ezSelectionManagerEvent;
struct ezCommandHistoryEvent;

class EZ_EDITORFRAMEWORK_DLL ezGameObjectGizmoInterface
{
public:
  virtual ezObjectAccessorBase* GetObjectAccessor() = 0;
  virtual bool CanDuplicateSelection() const = 0;
  virtual void DuplicateSelection() = 0;
};

class EZ_EDITORFRAMEWORK_DLL ezGameObjectGizmoHandler
{
public:
  ezGameObjectGizmoHandler(ezGameObjectDocument* pDocument, ezQtGameObjectDocumentWindow* pWindow,
    ezGameObjectGizmoInterface* pInterface);
  ~ezGameObjectGizmoHandler();

  ezGameObjectDocument* GetDocument() const;
  ezTranslateGizmo& GetTranslateGizmo() { return m_TranslateGizmo; }
  ezRotateGizmo& GetRotateGizmo() { return m_RotateGizmo; }
  ezScaleGizmo& GetScaleGizmo() { return m_ScaleGizmo; }
  ezDragToPositionGizmo& GetDragToPosGizmo() { return m_DragToPosGizmo; }

  struct SelectedGO
  {
    const ezDocumentObject* m_pObject;
    ezVec3 m_vLocalScaling;
    float m_fLocalUniformScaling;
    ezTransform m_GlobalTransform;
  };

  ezDeque<SelectedGO> GetSelectedGizmoObjects();

private:
  void TransformationGizmoEventHandler(const ezGizmoEvent& e);
  void CommandHistoryEventHandler(const ezCommandHistoryEvent& e);
  void SelectionManagerEventHandler(const ezSelectionManagerEvent& e);
  void ManipulatorManagerEventHandler(const ezManipulatorManagerEvent& e);
  void ObjectStructureEventHandler(const ezDocumentObjectStructureEvent& e);
  void GameObjectEventHandler(const ezGameObjectEvent& e);
  void EngineWindowEventHandler(const ezEngineWindowEvent& e);

  void UpdateGizmoSelectionList();
  void UpdateGizmoVisibility();
  void UpdateManipulatorVisibility();
  void UpdateGizmoPosition();

private:
  ezGameObjectDocument* m_pDocument = nullptr;
  ezQtGameObjectDocumentWindow* m_pWindow = nullptr;
  ezGameObjectGizmoInterface* m_pInterface = nullptr;

  ezTranslateGizmo m_TranslateGizmo;
  ezRotateGizmo m_RotateGizmo;
  ezScaleGizmo m_ScaleGizmo;
  ezDragToPositionGizmo m_DragToPosGizmo;

  bool m_bInGizmoInteraction = false;
  bool m_bMergeTransactions;
  ezDeque<SelectedGO> m_GizmoSelection;
};

