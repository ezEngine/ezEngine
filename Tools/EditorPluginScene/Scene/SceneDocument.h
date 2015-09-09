#pragma once

#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <EditorFramework/DocumentWindow3D/DocumentWindow3D.moc.h>

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
  void DuplicateSelection();
  void TriggerHideSelectedObjects();
  void TriggerHideUnselectedObjects();
  void TriggerShowHiddenObjects();
  void SetGizmoWorldSpace(bool bWorldSpace);
  bool GetGizmoWorldSpace() const;

  virtual bool Copy(ezAbstractObjectGraph& out_objectGraph) override;
  virtual bool Paste(const ezArrayPtr<PasteInfo>& info) override;
  bool Duplicate(const ezArrayPtr<PasteInfo>& info);
  bool Copy(ezAbstractObjectGraph& graph, ezMap<ezUuid, ezUuid>* out_pParents);
  bool PasteAt(const ezArrayPtr<PasteInfo>& info, const ezVec3& vPos);
  bool PasteAtOrignalPosition(const ezArrayPtr<PasteInfo>& info);

  const ezTransform& GetGlobalTransform(const ezDocumentObjectBase* pObject);
  void SetGlobalTransform(const ezDocumentObjectBase* pObject, const ezTransform& t);

  void SetPickingResult(const ezObjectPickingResult& res) { m_PickingResult = res; }

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
  ezObjectPickingResult m_PickingResult;

  ezHashTable<const ezDocumentObjectBase*, ezTransform> m_GlobalTransforms;
};
