#pragma once

#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>


enum class ActiveGizmo
{
  None,
  Translate,
  Rotate,
  Scale,
  DragToPosition,
};

class ezSceneDocument : public ezDocumentBase
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSceneDocument);

public:
  ezSceneDocument(const char* szDocumentPath);
  ~ezSceneDocument();

  virtual const char* GetDocumentTypeDisplayString() const override { return "Scene"; }

  virtual ezStatus InternalSaveDocument() override;

  void SetActiveGizmo(ActiveGizmo gizmo);
  ActiveGizmo GetActiveGizmo() const;

  void TriggerShowSelectionInScenegraph();
  void TriggerFocusOnSelection();
  void TriggerSnapPivotToGrid();
  void TriggerSnapEachObjectToGrid();
  void GroupSelection();
  void TriggerHideSelectedObjects();
  void TriggerHideUnselectedObjects();
  void TriggerShowHiddenObjects();
  void SetGizmoWorldSpace(bool bWorldSpace);
  bool GetGizmoWorldSpace() const;

  void Copy();
  void Paste();

  const ezTransform& GetGlobalTransform(const ezDocumentObjectBase* pObject);
  void SetGlobalTransform(const ezDocumentObjectBase* pObject, const ezTransform& t);

  static ezTransform QueryLocalTransform(const ezDocumentObjectBase* pObject);
  static ezTransform ComputeGlobalTransform(const ezDocumentObjectBase* pObject);

  struct SceneEvent
  {
    enum class Type
    {
      ActiveGizmoChanged,
      ShowSelectionInScenegraph,
      FocusOnSelection,
      SnapSelectionPivotToGrid,
      SnapEachSelectedObjectToGrid,
      HideSelectedObjects,
      HideUnselectedObjects,
      ShowHiddenObjects,
    };

    Type m_Type;
  };

  ezEvent<const SceneEvent&> m_SceneEvents;

protected:
  virtual void InitializeAfterLoading() override;

  virtual ezDocumentInfo* CreateDocumentInfo() override { return EZ_DEFAULT_NEW(ezDocumentInfo); }

private:
  void ObjectPropertyEventHandler(const ezDocumentObjectPropertyEvent& e);
  void ObjectStructureEventHandler(const ezDocumentObjectStructureEvent& e);

  void InvalidateGlobalTransformValue(const ezDocumentObjectBase* pObject);

  bool m_bGizmoWorldSpace; // whether the gizmo is in local/global space mode
  ActiveGizmo m_ActiveGizmo;

  ezHashTable<const ezDocumentObjectBase*, ezTransform> m_GlobalTransforms;
};
