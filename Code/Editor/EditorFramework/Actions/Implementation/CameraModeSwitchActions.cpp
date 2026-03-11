#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Actions/CameraModeSwitchActions.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCameraModeSwitchAction, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezActionDescriptorHandle ezCameraModeSwitchActions::s_hCameraMode;

void ezCameraModeSwitchActions::RegisterActions()
{
  s_hCameraMode = EZ_REGISTER_DYNAMIC_MENU("Asset.CameraMode", ezCameraModeSwitchAction, ":/EditorFramework/Icons/Camera.svg");
}

void ezCameraModeSwitchActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hCameraMode);
}

void ezCameraModeSwitchActions::MapToolbarActions(const char* szMapping)
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap(szMapping);
  EZ_ASSERT_DEV(pMap != nullptr, "Toolbar action map '{}' does not exist", szMapping);
  pMap->MapAction(s_hCameraMode, "", 10.0f);
}

//////////////////////////////////////////////////////////////////////////

ezCameraModeSwitchAction::ezCameraModeSwitchAction(const ezActionContext& context, const char* szName, const char* szIconPath)
  : ezDynamicMenuAction(context, szName, szIconPath)
{
}

void ezCameraModeSwitchAction::GetEntries(ezDynamicArray<Item>& out_entries)
{
  out_entries.Clear();

  auto* pWindow = qobject_cast<ezQtEngineDocumentWindow*>(m_Context.m_pWindow);
  if (pWindow == nullptr)
    return;

  const int iMode = pWindow->GetCameraMode();

  ezTempHybridArray<ezString, 8> names;
  pWindow->GetCameraModeNames(names);

  for (ezUInt32 i = 0; i < names.GetCount(); ++i)
  {
    auto& item = out_entries.ExpandAndGetRef();
    item.m_sDisplay = names[i];
    item.m_UserValue = (int)i;
    item.m_CheckState = (iMode == (int)i) ? Item::CheckMark::Checked : Item::CheckMark::Unchecked;
  }
}

void ezCameraModeSwitchAction::Execute(const ezVariant& value)
{
  auto* pWindow = qobject_cast<ezQtEngineDocumentWindow*>(m_Context.m_pWindow);
  if (pWindow)
    pWindow->SetCameraMode(value.ConvertTo<int>());
}
