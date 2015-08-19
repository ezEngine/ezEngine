#pragma once

#include <ToolsFoundation/Basics.h>
#include <EditorFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/DocumentWindow3D/EditorInputContext.h>
#include <Foundation/Logging/Log.h>

class ezCamera;

class EZ_EDITORFRAMEWORK_DLL ezGizmoBase : public ezEditorInputContext
{
  EZ_ADD_DYNAMIC_REFLECTION(ezGizmoBase);

public:
  ezGizmoBase();

  void SetVisible(bool bVisible);
  bool IsVisible() const { return m_bVisible; }

  void SetTransformation(const ezMat4& transform);
  const ezMat4& GetTransformation() const { return m_Transformation; }

  void ConfigureInteraction(ezGizmoHandleBase* pHandle, const ezCamera* pCamera, const ezVec3& vInteractionPivot, const ezVec2I32& viewport)
  {
    m_pInteractionGizmoHandle = pHandle;
    m_pCamera = pCamera;
    m_vInteractionPivot = vInteractionPivot;
    m_Viewport = viewport;
  }

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

  const ezCamera* m_pCamera;
  ezGizmoHandleBase* m_pInteractionGizmoHandle;
  ezVec3 m_vInteractionPivot;
  ezVec2I32 m_Viewport;

private:
  bool m_bVisible;
  ezMat4 m_Transformation;

};
