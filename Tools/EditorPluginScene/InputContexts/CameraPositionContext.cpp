#include <PCH.h>
#include <EditorPluginScene/InputContexts/CameraPositionContext.h>
#include <EditorFramework/DocumentWindow3D/DocumentWindow3D.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <CoreUtils/Graphics/Camera.h>
#include <EditorFramework/DocumentWindow3D/3DViewWidget.moc.h>

ezCameraPositionContext::ezCameraPositionContext(ezDocumentWindow3D* pOwnerWindow, ezEngineViewWidget* pOwnerView)
{
  m_pCamera = nullptr;

  m_LastUpdate = ezTime::Now();

  m_fLerp = 1.0f;

  // while the camera moves, ignore all other shortcuts
  SetShortcutsDisabled(true);

  SetOwner(pOwnerWindow, pOwnerView);
}

void ezCameraPositionContext::FocusLost()
{
  m_fLerp = 1.0f;
  GetOwnerView()->setCursor(QCursor(Qt::ArrowCursor));
  MakeActiveInputContext(false);
}

void ezCameraPositionContext::MoveToTarget(const ezVec3& vPosition, const ezVec3& vDirection)
{
  // prevent restarting this in the middle of a move
  if (m_fLerp < 1.0f)
    return;

  m_vStartPosition = m_pCamera->GetPosition();
  m_vTargetPosition = vPosition;

  m_vStartDirection = m_pCamera->GetCenterDirForwards();
  m_vTargetDirection = vDirection;

  m_vStartDirection.Normalize();
  m_vTargetDirection.Normalize();

  if (m_vStartPosition == m_vTargetPosition &&
      m_vStartDirection == m_vTargetDirection)
    return;

  m_LastUpdate = ezTime::Now();

  m_fLerp = 0.0f;
  GetOwnerView()->setCursor(QCursor(Qt::BlankCursor));
  MakeActiveInputContext();
}

void ezCameraPositionContext::UpdateContext()
{
  ezTime tNow = ezTime::Now();
  ezTime tDiff = tNow - m_LastUpdate;
  m_LastUpdate = tNow;

  m_fLerp += tDiff.GetSeconds() * 2.0f;

  if (m_fLerp >= 1.0f)
  {
    FocusLost(); // done
    m_fLerp = 1.0f;
  }

  const float fLerpValue = ezMath::Sin(ezAngle::Degree(90.0f * m_fLerp));

  ezQuat qRot, qRotFinal;
  qRot.SetShortestRotation(m_vStartDirection, m_vTargetDirection);
  qRotFinal.SetSlerp(ezQuat::IdentityQuaternion(), qRot, fLerpValue);

  const ezVec3 vNewDirection = qRotFinal * m_vStartDirection;
  const ezVec3 vNewPosition = ezMath::Lerp(m_vStartPosition, m_vTargetPosition, fLerpValue);

  m_pCamera->LookAt(vNewPosition, vNewPosition + vNewDirection, ezVec3(0.0f, 0.0f, 1.0f));
}


