#include <PCH.h>
#include <EditorFramework/DocumentWindow/GameObjectDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/GameObjectGizmoHandler.h>
#include <EditorFramework/Document/GameObjectDocument.h>
#include <Preferences/ScenePreferences.h>
#include <Gizmos/SnapProvider.h>
#include <Gizmos/TranslateGizmo.h>
#include <Preferences/EditorPreferences.h>

ezQtGameObjectDocumentWindow::ezQtGameObjectDocumentWindow(ezGameObjectDocument* pDocument)
  : ezQtEngineDocumentWindow(pDocument)
{
}

ezQtGameObjectDocumentWindow::~ezQtGameObjectDocumentWindow()
{
}

ezGameObjectDocument* ezQtGameObjectDocumentWindow::GetGameObjectDocument() const
{
  return static_cast<ezGameObjectDocument*>(GetDocument());
}

ezGlobalSettingsMsgToEngine ezQtGameObjectDocumentWindow::GetGlobalSettings() const
{
  ezGlobalSettingsMsgToEngine msg;
  msg.m_fGizmoScale = ezPreferences::QueryPreferences<ezEditorPreferencesUser>()->m_fGizmoScale;
  return msg;
}

ezWorldSettingsMsgToEngine ezQtGameObjectDocumentWindow::GetWorldSettings() const
{
  ezWorldSettingsMsgToEngine msg;
  auto pGameObjectDoc = GetGameObjectDocument();
  msg.m_bRenderOverlay = pGameObjectDoc->GetRenderSelectionOverlay();
  msg.m_bRenderShapeIcons = pGameObjectDoc->GetRenderShapeIcons();
  msg.m_bRenderSelectionBoxes = pGameObjectDoc->GetRenderVisualizers();
  msg.m_bAddAmbientLight = pGameObjectDoc->GetAddAmbientLight();
  return msg;
}

ezGridSettingsMsgToEngine ezQtGameObjectDocumentWindow::GetGridSettings(ezGameObjectGizmoHandler* handler) const
{
  ezGridSettingsMsgToEngine msg;
  auto pSceneDoc = GetGameObjectDocument();
  ezScenePreferencesUser* pPreferences = ezPreferences::QueryPreferences<ezScenePreferencesUser>(GetDocument());

  msg.m_fGridDensity = ezSnapProvider::GetTranslationSnapValue() * (pSceneDoc->GetGizmoWorldSpace() ? 1.0f : -1.0f); // negative density = local space
  msg.m_vGridTangent1.SetZero(); // indicates that the grid is disabled
  msg.m_vGridTangent2.SetZero(); // indicates that the grid is disabled
  ezTranslateGizmo& translateGizmo = handler->GetTranslateGizmo();
  if (pPreferences->GetShowGrid() && translateGizmo.IsVisible())
  {
    msg.m_vGridCenter = translateGizmo.GetStartPosition();

    if (translateGizmo.GetTranslateMode() == ezTranslateGizmo::TranslateMode::Axis)
      msg.m_vGridCenter = translateGizmo.GetTransformation().m_vPosition;

    if (pSceneDoc->GetGizmoWorldSpace())
    {
      ezSnapProvider::SnapTranslation(msg.m_vGridCenter);

      switch (translateGizmo.GetLastPlaneInteraction())
      {
      case ezTranslateGizmo::PlaneInteraction::PlaneX:
        msg.m_vGridCenter.y = ezMath::Round(msg.m_vGridCenter.y, ezSnapProvider::GetTranslationSnapValue() * 10);
        msg.m_vGridCenter.z = ezMath::Round(msg.m_vGridCenter.z, ezSnapProvider::GetTranslationSnapValue() * 10);
        break;
      case ezTranslateGizmo::PlaneInteraction::PlaneY:
        msg.m_vGridCenter.x = ezMath::Round(msg.m_vGridCenter.x, ezSnapProvider::GetTranslationSnapValue() * 10);
        msg.m_vGridCenter.z = ezMath::Round(msg.m_vGridCenter.z, ezSnapProvider::GetTranslationSnapValue() * 10);
        break;
      case ezTranslateGizmo::PlaneInteraction::PlaneZ:
        msg.m_vGridCenter.x = ezMath::Round(msg.m_vGridCenter.x, ezSnapProvider::GetTranslationSnapValue() * 10);
        msg.m_vGridCenter.y = ezMath::Round(msg.m_vGridCenter.y, ezSnapProvider::GetTranslationSnapValue() * 10);
        break;
      }
    }

    switch (translateGizmo.GetLastPlaneInteraction())
    {
    case ezTranslateGizmo::PlaneInteraction::PlaneX:
      msg.m_vGridTangent1 = translateGizmo.GetTransformation().m_qRotation * ezVec3(0, 1, 0);
      msg.m_vGridTangent2 = translateGizmo.GetTransformation().m_qRotation * ezVec3(0, 0, 1);
      break;
    case ezTranslateGizmo::PlaneInteraction::PlaneY:
      msg.m_vGridTangent1 = translateGizmo.GetTransformation().m_qRotation * ezVec3(1, 0, 0);
      msg.m_vGridTangent2 = translateGizmo.GetTransformation().m_qRotation * ezVec3(0, 0, 1);
      break;
    case ezTranslateGizmo::PlaneInteraction::PlaneZ:
      msg.m_vGridTangent1 = translateGizmo.GetTransformation().m_qRotation * ezVec3(1, 0, 0);
      msg.m_vGridTangent2 = translateGizmo.GetTransformation().m_qRotation * ezVec3(0, 1, 0);
      break;
    }
  }
  return msg;
}
