#pragma once

#include <ToolsFoundation/Document/Document.h>

enum class ActiveGizmo
{
  None,
  Translate,
  Rotate,
  Scale,
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
  void SetGizmoWorldSpace(bool bWorldSpace);
  bool GetGizmoWorldSpace() const { return m_bGizmoWorldSpace; }

  struct SceneEvent
  {
    enum class Type
    {
      ActiveGizmoChanged,
      ShowSelectionInScenegraph,
    };

    Type m_Type;
  };

  ezEvent<const SceneEvent&> m_SceneEvents;

protected:
  virtual void InitializeAfterLoading() override;

  virtual ezDocumentInfo* CreateDocumentInfo() override { return EZ_DEFAULT_NEW(ezDocumentInfo); }

private:
  bool m_bGizmoWorldSpace;
  ActiveGizmo m_ActiveGizmo;
};
