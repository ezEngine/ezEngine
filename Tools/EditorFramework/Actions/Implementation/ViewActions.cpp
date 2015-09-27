#include <PCH.h>
#include <EditorFramework/Actions/ViewActions.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <EditorFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/DocumentWindow3D/3DViewWidget.moc.h>

ezActionDescriptorHandle ezViewActions::s_hRenderMode;
ezActionDescriptorHandle ezViewActions::s_hPerspective;


void ezViewActions::RegisterActions()
{
  s_hRenderMode = EZ_REGISTER_LRU_MENU("RenderMode", "Render Mode", ezRenderModeAction, ":/GuiFoundation/Icons/RenderMode.png");
  s_hPerspective = EZ_REGISTER_LRU_MENU("Perspective", "Perspective", ezPerspectiveAction, ":/GuiFoundation/Icons/Perspective.png");
}

void ezViewActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hRenderMode);
  ezActionManager::UnregisterAction(s_hPerspective);
}

void ezViewActions::MapActions(const char* szMapping, const char* szPath)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(szMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('%s') does not exist, mapping the actions failed!", szMapping);

  pMap->MapAction(s_hPerspective, szPath, 1.0f);
  pMap->MapAction(s_hRenderMode, szPath, 2.0f);
}

////////////////////////////////////////////////////////////////////////
// ezRenderModeAction
////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRenderModeAction, ezEnumerationMenuAction, 0, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezRenderModeAction::ezRenderModeAction(const ezActionContext& context, const char* szName, const char* szIconPath)
  : ezEnumerationMenuAction(context, szName, szIconPath)
{
  ezEngineViewWidget* pView = qobject_cast<ezEngineViewWidget*>(context.m_pWindow);
  EZ_ASSERT_DEV(pView != nullptr, "context.m_pWindow must be derived from type 'ezEngineViewWidget'!");
  InitEnumerationType(ezGetStaticRTTI<ezViewRenderMode>());
}

ezInt64 ezRenderModeAction::GetValue() const
{
  ezEngineViewWidget* pView = qobject_cast<ezEngineViewWidget*>(m_Context.m_pWindow);
  return (ezInt64)pView->m_pViewConfig->m_RenderMode;
}

void ezRenderModeAction::Execute(const ezVariant& value)
{
  ezEngineViewWidget* pView = qobject_cast<ezEngineViewWidget*>(m_Context.m_pWindow);
  pView->m_pViewConfig->m_RenderMode = (ezViewRenderMode::Enum)value.ConvertTo<ezInt64>();
  TriggerUpdate();
}

////////////////////////////////////////////////////////////////////////
// ezPerspectiveAction
////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPerspectiveAction, ezEnumerationMenuAction, 0, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezPerspectiveAction::ezPerspectiveAction(const ezActionContext& context, const char* szName, const char* szIconPath)
  : ezEnumerationMenuAction(context, szName, szIconPath)
{
  ezEngineViewWidget* pView = qobject_cast<ezEngineViewWidget*>(context.m_pWindow);
  EZ_ASSERT_DEV(pView != nullptr, "context.m_pWindow must be derived from type 'ezEngineViewWidget'!");
  InitEnumerationType(ezGetStaticRTTI<ezSceneViewPerspective>());
}

ezInt64 ezPerspectiveAction::GetValue() const
{
  ezEngineViewWidget* pView = qobject_cast<ezEngineViewWidget*>(m_Context.m_pWindow);
  return (ezInt64)pView->m_pViewConfig->m_Perspective;
}

void ezPerspectiveAction::Execute(const ezVariant& value)
{
  ezEngineViewWidget* pView = qobject_cast<ezEngineViewWidget*>(m_Context.m_pWindow);
  pView->m_pViewConfig->m_Perspective = (ezSceneViewPerspective::Enum)value.ConvertTo<ezInt64>();
  pView->m_pViewConfig->ApplyPerspectiveSetting();
  TriggerUpdate();
}
