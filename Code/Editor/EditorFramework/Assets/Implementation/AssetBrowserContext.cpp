#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetBrowserContext.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/BaseActions.h>

namespace
{
  ezAssetBrowserSelection s_CurrentAssetBrowserSelection;
} // namespace

const ezAssetBrowserSelection& ezAssetBrowserSelection::GetCurrent()
{
  return s_CurrentAssetBrowserSelection;
}

void ezAssetBrowserSelection::SetCurrent(ezAssetBrowserSelection&& ref_selection)
{
  s_CurrentAssetBrowserSelection = std::move(ref_selection);
}

ezActionDescriptorHandle ezAssetBrowserContextMenu::s_hAssetMenu;

void ezAssetBrowserContextMenu::RegisterActions()
{
  s_hAssetMenu = EZ_REGISTER_MENU_WITH_ICON("AssetBrowser.AssetMenu", ":/GuiFoundation/Icons/Document.svg");
}

void ezAssetBrowserContextMenu::MapActions()
{
  ezActionMap* pMap = ezActionMapManager::GetActionMap("AssetBrowserContextMenu");
  EZ_ASSERT_DEV(pMap != nullptr, "The action map 'AssetBrowserContextMenu' does not exist.");

  pMap->MapAction(s_hAssetMenu, "", 1.0f);
}

void ezAssetBrowserContextMenu::UnregisterActions()
{
  ezActionManager::UnregisterAction(s_hAssetMenu);
}
