#pragma once

#include <EditorFramework/DocumentWindow3D/EditorInputContext.h>
#include <EditorFramework/Gizmos/GizmoBase.h>
#include <QPoint>

class QWidget;
class ezCamera;

class ezOrthoGizmoContext : public ezEditorInputContext
{
  EZ_ADD_DYNAMIC_REFLECTION(ezOrthoGizmoContext, ezEditorInputContext);

public:

  ezOrthoGizmoContext(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView, const ezCamera* pCamera);

  void SetWindowConfig(const ezVec2I32& viewport)
  {
    m_Viewport = viewport;
  }

  virtual void FocusLost(bool bCancel);

  ezEvent<const ezGizmoEvent&> m_GizmoEvents;

  const ezVec3& GetTranslationResult() const { return m_vTranslationResult; }
  const ezVec3& GetTranslationDiff() const { return m_vTranslationDiff; }
  const ezQuat& GetRotationResult() const { return m_qRotationResult; }
  float GetScalingResult() const { return m_fScalingResult; }

protected:
  virtual ezEditorInut doMousePressEvent(QMouseEvent* e) override;
  virtual ezEditorInut doMouseReleaseEvent(QMouseEvent* e) override;
  virtual ezEditorInut doMouseMoveEvent(QMouseEvent* e) override;

  virtual void OnSetOwner(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView) override {}

private:
  bool IsViewInOthoMode() const;

  ezVec2I32 m_LastMousePos;
  ezVec3 m_vUnsnappedTranslationResult;
  ezVec3 m_vTranslationResult;
  ezVec3 m_vTranslationDiff;
  ezAngle m_UnsnappedRotationResult;
  ezQuat m_qRotationResult;
  float m_fScaleMouseMove;
  float m_fScalingResult;
  float m_fUnsnappedScalingResult;
  bool m_bCanInteract;
  const ezCamera* m_pCamera;
  ezVec2I32 m_Viewport;
};

