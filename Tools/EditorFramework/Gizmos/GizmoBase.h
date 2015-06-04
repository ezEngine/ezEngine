#pragma once

#include <ToolsFoundation/Basics.h>
#include <EditorFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/DocumentWindow3D/EditorInputContext.h>

class EZ_EDITORFRAMEWORK_DLL ezGizmoBase : public ezEditorInputContext
{
  EZ_ADD_DYNAMIC_REFLECTION(ezGizmoBase);

public:
  ezGizmoBase();

  virtual void SetDocumentGuid(const ezUuid& guid) = 0;

  void SetVisible(bool bVisible);
  bool IsVisible() const { return m_bVisible; }

  void SetTransformation(const ezMat4& transform);
  const ezMat4& GetTransformation() const { return m_Transformation; }

  void ConfigureInteraction(ezGizmoHandleBase* pHandle) { m_pInteractionGizmoHandle = pHandle; }

  struct BaseEvent
  {
    enum class Type
    {
      BeginInteractions,
      EndInteractions,
      Interaction,
    };

    const ezGizmoBase* m_pGizmo;
    Type m_Type;
  };

  ezEvent<const BaseEvent&> m_BaseEvents;

protected:
  virtual void OnVisibleChanged(bool bVisible) = 0;
  virtual void OnTransformationChanged(const ezMat4& transform) = 0;

  ezGizmoHandleBase* m_pInteractionGizmoHandle;

private:
  bool m_bVisible;
  ezMat4 m_Transformation;

};
