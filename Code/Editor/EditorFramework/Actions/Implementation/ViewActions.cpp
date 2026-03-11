#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Actions/ViewActions.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>

ezActionDescriptorHandle ezViewActions::s_hRenderMode;
ezActionDescriptorHandle ezViewActions::s_hPerspective;
ezActionDescriptorHandle ezViewActions::s_hActivateRemoteProcess;
ezActionDescriptorHandle ezViewActions::s_hLinkDeviceCamera;


void ezViewActions::RegisterActions()
{
  s_hRenderMode = EZ_REGISTER_DYNAMIC_MENU("View.RenderMode", ezRenderModeAction, ":/EditorFramework/Icons/RenderMode.svg");
  s_hPerspective = EZ_REGISTER_DYNAMIC_MENU("View.RenderPerspective", ezPerspectiveAction, ":/EditorFramework/Icons/Perspective.svg");
  s_hActivateRemoteProcess = EZ_REGISTER_ACTION_1("View.ActivateRemoteProcess", ezActionScope::Window, "View", "", ezViewAction, ezViewAction::ButtonType::ActivateRemoteProcess);
  s_hLinkDeviceCamera = EZ_REGISTER_ACTION_1("View.LinkDeviceCamera", ezActionScope::Window, "View", "", ezViewAction, ezViewAction::ButtonType::LinkDeviceCamera);
}

void ezViewActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hRenderMode);
  ezActionManager::UnregisterAction(s_hPerspective);
  ezActionManager::UnregisterAction(s_hActivateRemoteProcess);
  ezActionManager::UnregisterAction(s_hLinkDeviceCamera);
}

void ezViewActions::MapToolbarActions(ezStringView sMapping, ezUInt32 uiFlags)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(sMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  if (uiFlags & Flags::PerspectiveMode)
    pMap->MapAction(s_hPerspective, "", 1.0f);

  if (uiFlags & Flags::RenderMode)
    pMap->MapAction(s_hRenderMode, "", 2.0f);

  if (uiFlags & Flags::ActivateRemoteProcess)
  {
    pMap->MapAction(s_hActivateRemoteProcess, "", 4.0f);
    pMap->MapAction(s_hLinkDeviceCamera, "", 5.0f);
  }
}

////////////////////////////////////////////////////////////////////////
// ezRenderModeAction
////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRenderModeAction, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

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

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPerspectiveAction, 1, ezRTTINoAllocator)
  ;
EZ_END_DYNAMIC_REFLECTED_TYPE;

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

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezViewAction, 1, ezRTTINoAllocator)
  ;
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezViewAction::ezViewAction(const ezActionContext& context, const char* szName, ButtonType button)
  : ezButtonAction(context, szName, false, "")
{
  m_ButtonType = button;
  ezQtEngineViewWidget* pView = qobject_cast<ezQtEngineViewWidget*>(m_Context.m_pWindow);

  switch (m_ButtonType)
  {
    case ezViewAction::ButtonType::ActivateRemoteProcess:
      SetIconPath(":/EditorFramework/Icons/SwitchToRemoteProcess.svg");
      break;
    case ezViewAction::ButtonType::LinkDeviceCamera:
      SetIconPath(":/EditorFramework/Icons/LinkDeviceCamera.svg");
      SetCheckable(true);
      SetChecked(pView->m_pViewConfig->m_bUseCameraTransformOnDevice);
      break;
  }
}

ezViewAction::~ezViewAction() = default;

void ezViewAction::Execute(const ezVariant& value)
{
  ezQtEngineViewWidget* pView = qobject_cast<ezQtEngineViewWidget*>(m_Context.m_pWindow);

  switch (m_ButtonType)
  {
    case ezViewAction::ButtonType::ActivateRemoteProcess:
    {
      ezEditorEngineProcessConnection::GetSingleton()->ActivateRemoteProcess(ezDynamicCast<ezAssetDocument*>(m_Context.m_pDocument), pView->GetViewID());
    }
    break;

    case ezViewAction::ButtonType::LinkDeviceCamera:
    {
      pView->m_pViewConfig->m_bUseCameraTransformOnDevice = !pView->m_pViewConfig->m_bUseCameraTransformOnDevice;
      SetChecked(pView->m_pViewConfig->m_bUseCameraTransformOnDevice);
      ezEditorEngineProcessConnection::GetSingleton()->ActivateRemoteProcess(ezDynamicCast<ezAssetDocument*>(m_Context.m_pDocument), pView->GetViewID());
    }
    break;
  }
}
