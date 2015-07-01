#pragma once

#include <Foundation/Time/Time.h>
#include <EditorFramework/DocumentWindow3D/EditorInputContext.h>
#include <QPoint>

class ezCamera;

class ezCameraPositionContext : public ezEditorInputContext
{
public:
  ezCameraPositionContext(QWidget* pParentWidget, ezDocumentBase* pDocument, ezDocumentWindow3D* pDocumentWindow);

  virtual void FocusLost() override;

  void SetCamera(ezCamera* pCamera) { m_pCamera = pCamera; }

  void MoveToTarget(const ezVec3& vPosition, const ezVec3& vDirection);

private:
  virtual void UpdateContext() override;

  QWidget* m_pParentWidget;
  ezDocumentBase* m_pDocument;
  ezDocumentWindow3D* m_pDocumentWindow;

  float m_fLerp;
  ezVec3 m_vStartPosition;
  ezVec3 m_vTargetPosition;
  ezVec3 m_vStartDirection;
  ezVec3 m_vTargetDirection;

  ezCamera* m_pCamera;

  ezTime m_LastUpdate;
};