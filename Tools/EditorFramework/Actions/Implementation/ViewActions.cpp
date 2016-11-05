#include <PCH.h>
#include <EditorFramework/Actions/ViewActions.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <EditorFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/Assets/AssetBrowserDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Preferences/ProjectPreferences.h>
#include <RendererCore/Pipeline/Declarations.h>

ezActionDescriptorHandle ezViewActions::s_hRenderMode;
ezActionDescriptorHandle ezViewActions::s_hPerspective;
ezActionDescriptorHandle ezViewActions::s_hCameraUsageHint;


void ezViewActions::RegisterActions()
{
  s_hRenderMode = EZ_REGISTER_DYNAMIC_MENU("View.RenderMode", ezRenderModeAction, ":/EditorFramework/Icons/RenderMode.png");
  s_hPerspective = EZ_REGISTER_DYNAMIC_MENU("View.RenderPerspective", ezPerspectiveAction, ":/EditorFramework/Icons/Perspective.png");
  s_hCameraUsageHint = EZ_REGISTER_DYNAMIC_MENU("View.CameraUsageHint", ezCameraUsageHintAction, ":/EditorFramework/Icons/Tag16.png");
}

void ezViewActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hRenderMode);
  ezActionManager::UnregisterAction(s_hPerspective);
  ezActionManager::UnregisterAction(s_hCameraUsageHint);
}

void ezViewActions::MapActions(const char* szMapping, const char* szPath)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(szMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('%s') does not exist, mapping the actions failed!", szMapping);

  pMap->MapAction(s_hPerspective, szPath, 1.0f);
  pMap->MapAction(s_hRenderMode, szPath, 2.0f);
  pMap->MapAction(s_hCameraUsageHint, szPath, 3.0f);
}

////////////////////////////////////////////////////////////////////////
// ezRenderModeAction
////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRenderModeAction, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezRenderModeAction::ezRenderModeAction(const ezActionContext& context, const char* szName, const char* szIconPath)
  : ezEnumerationMenuAction(context, szName, szIconPath)
{
  ezQtEngineViewWidget* pView = qobject_cast<ezQtEngineViewWidget*>(context.m_pWindow);
  EZ_ASSERT_DEV(pView != nullptr, "context.m_pWindow must be derived from type 'ezQtEngineViewWidget'!");
  InitEnumerationType(ezGetStaticRTTI<ezViewRenderMode>());
}

ezInt64 ezRenderModeAction::GetValue() const
{
  ezQtEngineViewWidget* pView = qobject_cast<ezQtEngineViewWidget*>(m_Context.m_pWindow);
  return (ezInt64)pView->m_pViewConfig->m_RenderMode;
}

void ezRenderModeAction::Execute(const ezVariant& value)
{
  ezQtEngineViewWidget* pView = qobject_cast<ezQtEngineViewWidget*>(m_Context.m_pWindow);
  pView->m_pViewConfig->m_RenderMode = (ezViewRenderMode::Enum)value.ConvertTo<ezInt64>();
  TriggerUpdate();
}

////////////////////////////////////////////////////////////////////////
// ezPerspectiveAction
////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPerspectiveAction, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezPerspectiveAction::ezPerspectiveAction(const ezActionContext& context, const char* szName, const char* szIconPath)
  : ezEnumerationMenuAction(context, szName, szIconPath)
{
  ezQtEngineViewWidget* pView = qobject_cast<ezQtEngineViewWidget*>(context.m_pWindow);
  EZ_ASSERT_DEV(pView != nullptr, "context.m_pWindow must be derived from type 'ezQtEngineViewWidget'!");
  InitEnumerationType(ezGetStaticRTTI<ezSceneViewPerspective>());
}

ezInt64 ezPerspectiveAction::GetValue() const
{
  ezQtEngineViewWidget* pView = qobject_cast<ezQtEngineViewWidget*>(m_Context.m_pWindow);
  return (ezInt64)pView->m_pViewConfig->m_Perspective;
}

void ezPerspectiveAction::Execute(const ezVariant& value)
{
  ezQtEngineViewWidget* pView = qobject_cast<ezQtEngineViewWidget*>(m_Context.m_pWindow);
  auto newValue = (ezSceneViewPerspective::Enum)value.ConvertTo<ezInt64>();

  if (pView->m_pViewConfig->m_Perspective != newValue)
  {
    pView->m_pViewConfig->m_Perspective = newValue;
    pView->m_pViewConfig->ApplyPerspectiveSetting();
    TriggerUpdate();
  }
}

////////////////////////////////////////////////////////////////////////
// ezCameraUsageHintAction
////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCameraUsageHintAction, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezCameraUsageHintAction::ezCameraUsageHintAction(const ezActionContext& context, const char* szName, const char* szIconPath)
  : ezEnumerationMenuAction(context, szName, szIconPath)
{
  ezQtEngineViewWidget* pView = qobject_cast<ezQtEngineViewWidget*>(context.m_pWindow);
  EZ_ASSERT_DEV(pView != nullptr, "context.m_pWindow must be derived from type 'ezQtEngineViewWidget'!");
  InitEnumerationType(ezGetStaticRTTI<ezCameraUsageHint>());
}

ezInt64 ezCameraUsageHintAction::GetValue() const
{
  ezQtEngineViewWidget* pView = qobject_cast<ezQtEngineViewWidget*>(m_Context.m_pWindow);
  return (ezInt64)pView->m_pViewConfig->m_CameraUsageHint;
}

void ezCameraUsageHintAction::Execute(const ezVariant& value)
{
  ezQtEngineViewWidget* pView = qobject_cast<ezQtEngineViewWidget*>(m_Context.m_pWindow);
  auto newValue = (ezCameraUsageHint::Enum)value.ConvertTo<ezInt64>();
  pView->m_pViewConfig->m_CameraUsageHint = newValue;
  TriggerUpdate();
}
