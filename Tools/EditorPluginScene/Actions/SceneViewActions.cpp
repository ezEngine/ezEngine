#include <PCH.h>
#include <EditorPluginScene/Actions/SceneViewActions.h>
#include <EditorPluginScene/Scene/SceneViewWidget.moc.h>
#include <EditorPluginScene/Scene/SceneDocumentWindow.moc.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/ActionManager.h>

ezActionDescriptorHandle ezSceneViewActions::s_hToggleViews;
ezActionDescriptorHandle ezSceneViewActions::s_hSpawnView;


void ezSceneViewActions::RegisterActions()
{
  s_hToggleViews = EZ_REGISTER_ACTION_1("ToggleViews", "Toggle Views", ezActionScope::Window, "Document", "", ezSceneViewAction, ezSceneViewAction::ButtonType::ToggleViews);
  s_hSpawnView = EZ_REGISTER_ACTION_1("SpawnView", "Spawn View", ezActionScope::Window, "Document", "", ezSceneViewAction, ezSceneViewAction::ButtonType::SpawnView);
}

void ezSceneViewActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hToggleViews);
  ezActionManager::UnregisterAction(s_hSpawnView);
}

void ezSceneViewActions::MapActions(const char* szMapping, const char* szPath)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(szMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('%s') does not exist, mapping the actions failed!", szMapping);

  pMap->MapAction(s_hToggleViews, szPath, 3.0f);
  //pMap->MapAction(s_hSpawnView, szPath, 4.0f);
}

////////////////////////////////////////////////////////////////////////
// ezSceneViewAction
////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSceneViewAction, ezButtonAction, 0, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezSceneViewAction::ezSceneViewAction(const ezActionContext& context, const char* szName, ButtonType button)
  : ezButtonAction(context, szName, false, "")
{
  m_ButtonType = button;
  ezSceneViewWidget* pView = qobject_cast<ezSceneViewWidget*>(context.m_pWindow);
  EZ_ASSERT_DEV(pView != nullptr, "context.m_pWindow must be derived from type 'ezSceneViewWidget'!");
  switch (m_ButtonType)
  {
  case ButtonType::ToggleViews:
    SetIconPath(":/GuiFoundation/Icons/FocusOnSelection16.png");
    break;
  case ButtonType::SpawnView:
    SetIconPath(":/GuiFoundation/Icons/FocusOnSelection16.png");
    break;
  }
}

ezSceneViewAction::~ezSceneViewAction()
{

}

void ezSceneViewAction::Execute(const ezVariant& value)
{
  ezSceneViewWidget* pView = qobject_cast<ezSceneViewWidget*>(m_Context.m_pWindow);
  ezSceneDocumentWindow* pWindow = static_cast<ezSceneDocumentWindow*>(pView->GetDocumentWindow());

  switch (m_ButtonType)
  {
  case ButtonType::ToggleViews:
    QMetaObject::invokeMethod(pWindow, "ToggleViews", Qt::ConnectionType::QueuedConnection, Q_ARG(QWidget*, pView));
    //pWindow->ToggleViews(pView);
    break;
  case ButtonType::SpawnView:
    
    break;
  }
}

