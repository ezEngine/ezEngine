#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Types/Uuid.h>
#include <GuiFoundation/Action/ActionManager.h>

/// The asset browser selection that an action from the "AssetBrowserContextMenu" action map operates on.
///
/// ezActionContext carries no selection, and the actions behind the menu entries are cached and reused,
/// so they must read the selection here from ezAction::RefreshState() and Execute() rather than
/// capture it in their constructor.
///
/// Empty except while that context menu is being built or is open, which actions have to handle.
struct EZ_EDITORFRAMEWORK_DLL ezAssetBrowserSelection
{
  /// Guids of the selected sub-assets. For assets that have no sub-assets this is the asset guid.
  ezDynamicArray<ezUuid> m_SubAssetGuids;

  /// Guids of the owning (main) assets, with duplicates removed. Can be shorter than m_SubAssetGuids.
  ezDynamicArray<ezUuid> m_AssetGuids;

  /// Absolute paths of all selected items, including plain files and folders that are not assets.
  ezDynamicArray<ezString> m_AbsolutePaths;

  static const ezAssetBrowserSelection& GetCurrent();

  /// Called by the asset browser before it shows its context menu, and again with an empty selection
  /// once the menu is closed.
  static void SetCurrent(ezAssetBrowserSelection&& ref_selection);
};

/// The sub-menu of the "AssetBrowserContextMenu" action map that holds operations specific to the
/// type of the selected asset, e.g. creating a prefab or a collision mesh from a mesh.
///
/// Registered here rather than by any one plugin, because several plugins map into it. Pass the name
/// to ezActionMap::MapAction() as the sub-path.
struct EZ_EDITORFRAMEWORK_DLL ezAssetBrowserContextMenu
{
  /// The name to map into, and the action name that the localization files give a display name to.
  static constexpr ezStringView s_sAssetMenu = "AssetBrowser.AssetMenu"_ezsv;

  /// Called once during editor startup, before any plugin maps into the menu.
  static void RegisterActions();
  static void UnregisterActions();

  /// Puts the sub-menu into the "AssetBrowserContextMenu" map. Must have happened before a plugin
  /// maps anything into it, so this is called during editor startup as well.
  static void MapActions();

  static ezActionDescriptorHandle s_hAssetMenu;
};
