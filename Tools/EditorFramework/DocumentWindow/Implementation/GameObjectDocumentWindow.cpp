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

ezGridSettingsMsgToEngine ezQtGameObjectDocumentWindow::GetGridSettings() const
{
  ezGridSettingsMsgToEngine msg;

  if (auto pTool = GetGameObjectDocument()->GetActiveEditTool())
  {
    pTool->GetGridSettings(msg);
  }

  return msg;
}
