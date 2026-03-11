#include <EditorPluginAi/EditorPluginAiPCH.h>

#include <EditorPluginAi/Actions/AiActions.h>
#include <EditorPluginAi/Dialogs/AiProjectSettingsDlg.moc.h>
#include <GuiFoundation/Action/ActionMapManager.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAiAction, 0, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezActionDescriptorHandle ezAiActions::s_hCategoryAi;
ezActionDescriptorHandle ezAiActions::s_hProjectSettings;

void ezAiActions::RegisterActions()
{
  s_hCategoryAi = EZ_REGISTER_CATEGORY("Ai");
  s_hProjectSettings = EZ_REGISTER_ACTION_1("Ai.Settings.Project", ezActionScope::Document, "Ai", "", ezAiAction, ezAiAction::ActionType::ProjectSettings);
}

void ezAiActions::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hCategoryAi);
  ezActionManager::UnregisterAction(s_hProjectSettings);
}

void ezAiActions::MapMenuActions()
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap("AssetMenuBar");
  EZ_ASSERT_DEV(pMap != nullptr, "Mapping the actions failed!");

  pMap->MapAction(s_hCategoryAi, "G.Plugins.Settings", 10.0f);
  pMap->MapAction(s_hProjectSettings, "G.Plugins.Settings", "Ai", 1.0f);
}

ezAiAction::ezAiAction(const ezActionContext& context, const char* szName, ActionType type)
  : ezButtonAction(context, szName, false, "")
{
  m_Type = type;

  switch (m_Type)
  {
    case ActionType::ProjectSettings:
      SetIconPath(":/AiPlugin/ezAiPlugin.svg");
      break;
  }
}

ezAiAction::~ezAiAction() = default;

void ezAiAction::Execute(const ezVariant& value)
{
  if (m_Type == ActionType::ProjectSettings)
  {
    ezQtAiProjectSettingsDlg dlg(nullptr);
    if (dlg.exec() == QDialog::Accepted)
    {
      ezToolsProject::BroadcastConfigChanged();
    }
  }
}
