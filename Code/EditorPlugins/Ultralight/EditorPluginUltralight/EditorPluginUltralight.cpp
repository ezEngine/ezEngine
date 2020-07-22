#include <PCH.h>

#include <EditorPluginUltralight/Plugin.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/StandardMenus.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>

#include <GuiFoundation/UIServices/DynamicEnums.h>

static void ToolsProjectEventHandler(const ezToolsProjectEvent& e);

void OnLoadPlugin(bool bReloading)
{
  ezQtEditorApp::GetSingleton()->AddRuntimePluginDependency("EditorPluginUltralight", "ezUltralightPlugin");

  ezToolsProject::GetSingleton()->s_Events.AddEventHandler(ToolsProjectEventHandler);

  // Ultralight HTML Asset
  {
    // Menu Bar
    {
      ezActionMapManager::RegisterActionMap("UltralightHTMLAssetMenuBar");
      ezProjectActions::MapActions("UltralightHTMLAssetMenuBar");
      ezStandardMenus::MapActions("UltralightHTMLAssetMenuBar",
        ezStandardMenuTypes::File | ezStandardMenuTypes::Edit | ezStandardMenuTypes::Panels | ezStandardMenuTypes::Help);
      ezDocumentActions::MapActions("UltralightHTMLAssetMenuBar", "Menu.File", false);
      ezCommandHistoryActions::MapActions("UltralightHTMLAssetMenuBar", "Menu.Edit");
    }

    // Tool Bar
    {
      ezActionMapManager::RegisterActionMap("UltralightHTMLAssetToolBar");
      ezDocumentActions::MapActions("UltralightHTMLAssetToolBar", "", true);
      ezCommandHistoryActions::MapActions("UltralightHTMLAssetToolBar", "");
      ezAssetActions::MapActions("UltralightHTMLAssetToolBar", true);
    }
  }
}


void OnUnloadPlugin(bool bReloading) {}

static void ToolsProjectEventHandler(const ezToolsProjectEvent& e)
{
  if (e.m_Type == ezToolsProjectEvent::Type::ProjectOpened)
  {
    // TODO: See if we need project preferences
    // ezFmodProjectPreferences* pPreferences = ezPreferences::QueryPreferences<ezFmodProjectPreferences>();
    // pPreferences->SyncCVars();
  }
}

ezPlugin g_Plugin(false, OnLoadPlugin, OnUnloadPlugin);
