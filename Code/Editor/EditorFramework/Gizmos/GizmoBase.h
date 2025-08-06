#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/InputContexts/EditorInputContext.h>
#include <Foundation/Logging/Log.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

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

  const ezEditorInputContext* m_pGizmo = nullptr;
  Type m_Type;
};

class EZ_EDITORFRAMEWORK_DLL ezGizmo : public ezEditorInputContext
{
  EZ_ADD_DYNAMIC_REFLECTION(ezGizmo, ezEditorInputContext);

public:
  ezGizmo();

  void SetVisible(bool bVisible);
  bool IsVisible() const { return m_bVisible; }

  void SetTransformation(const ezTransform& transform);
  const ezTransform& GetTransformation() const { return m_Transformation; }

  void ConfigureInteraction(ezGizmoHandle* pHandle, const ezCamera* pCamera, const ezVec3& vInteractionPivot, const ezVec2I32& vViewport)
  {
    m_pInteractionGizmoHandle = pHandle;
    m_pCamera = pCamera;
    m_vInteractionPivot = vInteractionPivot;
    m_vViewport = vViewport;
  }

  ezEvent<const ezGizmoEvent&> m_GizmoEvents;

protected:
  virtual void OnVisibleChanged(bool bVisible) = 0;
  virtual void OnTransformationChanged(const ezTransform& transform) = 0;

  void GetInverseViewProjectionMatrix(ezMat4& out_mInvViewProj) const;
  ezResult GetPointOnPlane(const ezPlane& plane, const ezVec2I32& vScreenPos, const ezMat4& mInvViewProj, ezVec3& out_Result) const;
  ezResult GetPointOnAxis(const ezVec3& vStartPos, const ezVec3& vAxis, const ezVec2I32& vScreenPos, const ezMat4& mInvViewProj, ezVec3& out_Result, float* out_pProjectedLength = nullptr) const;

  const ezCamera* m_pCamera;
  ezGizmoHandle* m_pInteractionGizmoHandle;
  ezVec3 m_vInteractionPivot;
  ezVec2I32 m_vViewport;

private:
  bool m_bVisible;
  ezTransform m_Transformation;
};
