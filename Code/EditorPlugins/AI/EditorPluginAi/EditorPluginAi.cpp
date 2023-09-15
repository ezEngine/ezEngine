#include <EditorPluginAi/EditorPluginAiPCH.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Actions/CommonAssetActions.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <EditorPluginAi/Actions/AiActions.h>
#include <EditorPluginAi/Dialogs/AiProjectSettingsDlg.moc.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/StandardMenus.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <GuiFoundation/UIServices/DynamicEnums.h>
#include <GuiFoundation/UIServices/DynamicStringEnum.h>

static void ToolsProjectEventHandler(const ezToolsProjectEvent& e);

void OnLoadPlugin()
{
  ezToolsProject::GetSingleton()->s_Events.AddEventHandler(ToolsProjectEventHandler);

  ezAiActions::RegisterActions();
  ezAiActions::MapMenuActions();
}

void OnUnloadPlugin()
{
  ezAiActions::UnregisterActions();
  ezToolsProject::GetSingleton()->s_Events.RemoveEventHandler(ToolsProjectEventHandler);
}

EZ_PLUGIN_ON_LOADED()
{
  OnLoadPlugin();
}

EZ_PLUGIN_ON_UNLOADED()
{
  OnUnloadPlugin();
}

void UpdateGroundTypeDynamicEnumValues()
{
  ezAiNavigationConfig cfg;
  cfg.Load().IgnoreResult();

  {
    auto& cfe = ezDynamicEnum::GetDynamicEnum("AiGroundType");
    cfe.Clear();

    cfe.SetValueAndName(-1, "<Undefined>");

    // add all names and values that are active
    for (ezInt32 i = 0; i < ezAiNumGroundTypes; ++i)
    {
      if (cfg.m_GroundTypes[i].m_bUsed)
      {
        cfe.SetValueAndName(i, cfg.m_GroundTypes[i].m_sName);
      }
    }
  }

  {
    auto& de = ezDynamicStringEnum::CreateDynamicEnum("AiPathSearchConfig");
    de.Clear();

    for (const auto& pc : cfg.m_PathSearchConfigs)
    {
      de.AddValidValue(pc.m_sName);
    }

    de.SortValues();
  }

  {
    auto& de = ezDynamicStringEnum::CreateDynamicEnum("AiNavmeshConfig");
    de.Clear();

    for (const auto& pc : cfg.m_NavmeshConfigs)
    {
      de.AddValidValue(pc.m_sName);
    }

    de.SortValues();
  }
}

static void ToolsProjectEventHandler(const ezToolsProjectEvent& e)
{
  if (e.m_Type == ezToolsProjectEvent::Type::ProjectSaveState)
  {
  }

  if (e.m_Type == ezToolsProjectEvent::Type::ProjectOpened)
  {
    UpdateGroundTypeDynamicEnumValues();
  }
}
