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
  void SetGizmoWorldSpace(bool bWorldSpace);
  bool GetGizmoWorldSpace() const;

  static ezTransform QueryLocalTransform(const ezDocumentObjectBase* pObject);
  static ezTransform QueryGlobalTransform(const ezDocumentObjectBase* pObject);

  static ezTransform ComputeGlobalTransform(const ezDocumentObjectBase* pObject);

  struct SceneEvent
  {
    enum class Type
    {
      ActiveGizmoChanged,
      ShowSelectionInScenegraph,
      FocusOnSelection,
    };

    Type m_Type;
  };

  ezEvent<const SceneEvent&> m_SceneEvents;

protected:
  virtual void InitializeAfterLoading() override;

  virtual ezDocumentInfo* CreateDocumentInfo() override { return EZ_DEFAULT_NEW(ezDocumentInfo); }

private:
  void ObjectPropertyEventHandler(const ezDocumentObjectPropertyEvent& e);
  void CommandHistoryEventHandler(const ezCommandHistory::Event& e);
  void UpdateObjectGlobalPosition(const ezDocumentObjectBase* pObject);
  void UpdateObjectGlobalPosition(const ezDocumentObjectBase* pObject, const ezTransform& tParent);
  void UpdateObjectLocalPosition(const ezDocumentObjectBase* pObject);

  bool m_bInObjectTransformFixup; // prevent queuing of more objects for local/global transform update, while we are in the process of updating the current object queue
  bool m_bGizmoWorldSpace; // whether the gizmo is in local/global space mode
  ActiveGizmo m_ActiveGizmo;

  ezDeque<const ezDocumentObjectBase*> m_UpdateGlobalTransform; // queue of objects whose global transform must be updated, because their local transform changed
  ezDeque<const ezDocumentObjectBase*> m_UpdateLocalTransform; // queue of objects whose local transform must be updated, because their global transform changed
};
