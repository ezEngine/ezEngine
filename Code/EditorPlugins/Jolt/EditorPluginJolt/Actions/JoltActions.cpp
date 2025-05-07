#include <EditorPluginJolt/EditorPluginJoltPCH.h>

#include <EditorPluginJolt/Actions/JoltActions.h>
#include <EditorPluginJolt/Dialogs/JoltProjectSettingsDlg.moc.h>
#include <GuiFoundation/Action/ActionMapManager.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezJoltAction, 0, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezActionDescriptorHandle ezJoltActions::s_hCategoryJolt;
ezActionDescriptorHandle ezJoltActions::s_hProjectSettings;

void ezJoltActions::RegisterActions()
{
  s_hCategoryJolt = EZ_REGISTER_CATEGORY("Jolt");
  s_hProjectSettings = EZ_REGISTER_ACTION_1("Jolt.Settings.Project", ezActionScope::Document, "Jolt", "", ezJoltAction, ezJoltAction::ActionType::ProjectSettings);
}

void ezJoltActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hCategoryJolt);
  ezActionManager::UnregisterAction(s_hProjectSettings);
}

void ezJoltActions::MapMenuActions()
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap("AssetMenuBar");
  EZ_ASSERT_DEV(pMap != nullptr, "Mapping the actions failed!");

  pMap->MapAction(s_hCategoryJolt, "G.Plugins.Settings", 10.0f);
  pMap->MapAction(s_hProjectSettings, "G.Plugins.Settings", "Jolt", 1.0f);
}

ezJoltAction::ezJoltAction(const ezActionContext& context, const char* szName, ActionType type)
  : ezButtonAction(context, szName, false, "")
{
  m_Type = type;

  switch (m_Type)
  {
    case ActionType::ProjectSettings:
      SetIconPath(":/JoltPlugin/JoltPlugin.svg");
      break;
  }
}

ezJoltAction::~ezJoltAction() = default;

void ezJoltAction::Execute(const ezVariant& value)
{
  if (m_Type == ActionType::ProjectSettings)
  {
    ezQtJoltProjectSettingsDlg dlg(value);
    if (dlg.exec() == QDialog::Accepted)
    {
      ezToolsProject::BroadcastConfigChanged();
    }
  }
}
