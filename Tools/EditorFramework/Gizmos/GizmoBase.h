#pragma once

#include <ToolsFoundation/Basics.h>
#include <EditorFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/DocumentWindow3D/EditorInputContext.h>
#include <Foundation/Logging/Log.h>

class ezCamera;

struct ezGizmoEvent
{
  enum class Type
  {
    BeginInteractions,
    EndInteractions,
    Interaction,
    CancelInteractions,
  };

  const ezEditorInputContext* m_pGizmo;
  Type m_Type;
};

class EZ_EDITORFRAMEWORK_DLL ezGizmo : public ezEditorInputContext
{
  EZ_ADD_DYNAMIC_REFLECTION(ezGizmo, ezEditorInputContext);

public:
  ezGizmo();

  void SetVisible(bool bVisible);
  bool IsVisible() const { return m_bVisible; }

  void SetTransformation(const ezMat4& transform);
  const ezMat4& GetTransformation() const { return m_Transformation; }

  void ConfigureInteraction(ezGizmoHandle* pHandle, const ezCamera* pCamera, const ezVec3& vInteractionPivot, const ezVec2I32& viewport)
  {
    m_pInteractionGizmoHandle = pHandle;
    m_pCamera = pCamera;
    m_vInteractionPivot = vInteractionPivot;
    m_Viewport = viewport;
  }

  ezEvent<const ezGizmoEvent&> m_GizmoEvents;

protected:
  virtual void OnVisibleChanged(bool bVisible) = 0;
  virtual void OnTransformationChanged(const ezMat4& transform) = 0;

  const ezCamera* m_pCamera;
  ezGizmoHandle* m_pInteractionGizmoHandle;
  ezVec3 m_vInteractionPivot;
  ezVec2I32 m_Viewport;

private:
  bool m_bVisible;
  ezMat4 m_Transformation;

};
