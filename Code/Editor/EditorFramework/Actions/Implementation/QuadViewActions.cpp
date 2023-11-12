#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Actions/QuadViewActions.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>

ezActionDescriptorHandle ezQuadViewActions::s_hToggleViews;
ezActionDescriptorHandle ezQuadViewActions::s_hSpawnView;

void ezQuadViewActions::RegisterActions()
{
  s_hToggleViews = EZ_REGISTER_ACTION_1("Scene.View.Toggle", ezActionScope::Window, "Scene", "", ezQuadViewAction, ezQuadViewAction::ButtonType::ToggleViews);
  s_hSpawnView = EZ_REGISTER_ACTION_1("Scene.View.Span", ezActionScope::Window, "Scene", "", ezQuadViewAction, ezQuadViewAction::ButtonType::SpawnView);
}

void ezQuadViewActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hToggleViews);
  ezActionManager::UnregisterAction(s_hSpawnView);
}

void ezQuadViewActions::MapToolbarActions(ezStringView sMapping)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(sMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hToggleViews, "", 3.0f);
}

////////////////////////////////////////////////////////////////////////
// ezSceneViewAction
////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezQuadViewAction, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezQuadViewAction::ezQuadViewAction(const ezActionContext& context, const char* szName, ButtonType button)
  : ezButtonAction(context, szName, false, "")
{
  m_ButtonType = button;
  ezQtEngineViewWidget* pView = qobject_cast<ezQtEngineViewWidget*>(context.m_pWindow);
  EZ_ASSERT_DEV(pView != nullptr, "context.m_pWindow must be derived from type 'ezQtGameObjectViewWidget'!");
  switch (m_ButtonType)
  {
    case ButtonType::ToggleViews:
      SetIconPath(":/EditorFramework/Icons/ToggleViews.svg");
      break;
    case ButtonType::SpawnView:
      SetIconPath(":/EditorFramework/Icons/SpawnView.svg");
      break;
  }
}

ezQuadViewAction::~ezQuadViewAction() = default;

void ezQuadViewAction::Execute(const ezVariant& value)
{
  ezQtEngineViewWidget* pView = qobject_cast<ezQtEngineViewWidget*>(m_Context.m_pWindow);
  ezQtEngineDocumentWindow* pWindow = static_cast<ezQtEngineDocumentWindow*>(pView->GetDocumentWindow());

  switch (m_ButtonType)
  {
    case ButtonType::ToggleViews:
      // Duck-typing to the rescue!
      QMetaObject::invokeMethod(pWindow, "ToggleViews", Qt::ConnectionType::QueuedConnection, Q_ARG(QWidget*, pView));
      break;
    case ButtonType::SpawnView:
      break;
  }
}
