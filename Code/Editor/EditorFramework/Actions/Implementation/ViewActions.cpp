#include <EditorFrameworkPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/Actions/ViewActions.h>
#include <EditorFramework/Assets/AssetBrowserDlg.moc.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Preferences/ProjectPreferences.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <RendererCore/Pipeline/Declarations.h>

ezActionDescriptorHandle ezViewActions::s_hRenderMode;
ezActionDescriptorHandle ezViewActions::s_hPerspective;
ezActionDescriptorHandle ezViewActions::s_hCameraUsageHint;
ezActionDescriptorHandle ezViewActions::s_hActivateRemoteProcess;
ezActionDescriptorHandle ezViewActions::s_hLinkDeviceCamera;


void ezViewActions::RegisterActions()
{
  s_hRenderMode = EZ_REGISTER_DYNAMIC_MENU("View.RenderMode", ezRenderModeAction, ":/EditorFramework/Icons/RenderMode.png");
  s_hPerspective = EZ_REGISTER_DYNAMIC_MENU("View.RenderPerspective", ezPerspectiveAction, ":/EditorFramework/Icons/Perspective.png");
  s_hCameraUsageHint = EZ_REGISTER_DYNAMIC_MENU("View.CameraUsageHint", ezCameraUsageHintAction, ":/EditorFramework/Icons/Tag16.png");
  s_hActivateRemoteProcess = EZ_REGISTER_ACTION_1("View.ActivateRemoteProcess", ezActionScope::Window, "View", "", ezViewAction,
                                                  ezViewAction::ButtonType::ActivateRemoteProcess);
  s_hLinkDeviceCamera = EZ_REGISTER_ACTION_1("View.LinkDeviceCamera", ezActionScope::Window, "View", "", ezViewAction,
                                             ezViewAction::ButtonType::LinkDeviceCamera);
}

void ezViewActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hRenderMode);
  ezActionManager::UnregisterAction(s_hPerspective);
  ezActionManager::UnregisterAction(s_hCameraUsageHint);
  ezActionManager::UnregisterAction(s_hActivateRemoteProcess);
  ezActionManager::UnregisterAction(s_hLinkDeviceCamera);
}

void ezViewActions::MapActions(const char* szMapping, const char* szPath, ezUInt32 flags)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(szMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", szMapping);

  if (flags & Flags::PerspectiveMode)
    pMap->MapAction(s_hPerspective, szPath, 1.0f);

  if (flags & Flags::RenderMode)
    pMap->MapAction(s_hRenderMode, szPath, 2.0f);

  if (flags & Flags::UsageHint)
    pMap->MapAction(s_hCameraUsageHint, szPath, 3.0f);

  if (flags & Flags::ActivateRemoteProcess)
  {
    pMap->MapAction(s_hActivateRemoteProcess, szPath, 4.0f);
    pMap->MapAction(s_hLinkDeviceCamera, szPath, 5.0f);
  }
}

////////////////////////////////////////////////////////////////////////
// ezRenderModeAction
////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRenderModeAction, 1, ezRTTINoAllocator);
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

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPerspectiveAction, 1, ezRTTINoAllocator);
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

////////////////////////////////////////////////////////////////////////
// ezCameraUsageHintAction
////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCameraUsageHintAction, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE;

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

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezViewAction, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezViewAction::ezViewAction(const ezActionContext& context, const char* szName, ButtonType button)
    : ezButtonAction(context, szName, false, "")
{
  m_ButtonType = button;
  ezQtEngineViewWidget* pView = qobject_cast<ezQtEngineViewWidget*>(m_Context.m_pWindow);

  switch (m_ButtonType)
  {
    case ezViewAction::ButtonType::ActivateRemoteProcess:
      SetIconPath(":/EditorFramework/Icons/SwitchToRemoteProcess16.png");
      break;
    case ezViewAction::ButtonType::LinkDeviceCamera:
      SetIconPath(":/EditorFramework/Icons/LinkDeviceCamera16.png");
      SetCheckable(true);
      SetChecked(pView->m_pViewConfig->m_bUseCameraTransformOnDevice);
      break;
  }
}

ezViewAction::~ezViewAction() {}

void ezViewAction::Execute(const ezVariant& value)
{
  ezQtEngineViewWidget* pView = qobject_cast<ezQtEngineViewWidget*>(m_Context.m_pWindow);

  switch (m_ButtonType)
  {
    case ezViewAction::ButtonType::ActivateRemoteProcess:
    {
      ezEditorEngineProcessConnection::GetSingleton()->ActivateRemoteProcess(m_Context.m_pDocument, pView->GetViewID());
    }
    break;

    case ezViewAction::ButtonType::LinkDeviceCamera:
    {
      pView->m_pViewConfig->m_bUseCameraTransformOnDevice = !pView->m_pViewConfig->m_bUseCameraTransformOnDevice;
      SetChecked(pView->m_pViewConfig->m_bUseCameraTransformOnDevice);
      ezEditorEngineProcessConnection::GetSingleton()->ActivateRemoteProcess(m_Context.m_pDocument, pView->GetViewID());
    }
    break;
  }
}
