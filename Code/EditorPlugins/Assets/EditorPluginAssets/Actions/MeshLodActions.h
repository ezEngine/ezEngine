#pragma once

#include <EditorPluginAssets/EditorPluginAssetsDLL.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Types/Uuid.h>
#include <GuiFoundation/Action/BaseActions.h>

/// Creates LOD mesh assets from mesh assets. Hides itself unless the target is one or more mesh
/// assets that are not themselves LODs.
class ezMeshLodActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  /// Pass bDocumentScope for a map belonging to a document window, so that the action is given that
  /// document; leave it off for the asset browser, which has none and uses ezAssetBrowserSelection.
  ///
  /// Fails if the action map does not exist, i.e. the plugin that owns it is not loaded.
  static ezResult MapActions(ezStringView sActionMap, ezStringView sSubPath, bool bDocumentScope = false);

  static ezActionDescriptorHandle s_hCategory;
  static ezActionDescriptorHandle s_hCreateLods;
  static ezActionDescriptorHandle s_hCreateLodsDoc;
};

class ezMeshLodAction : public ezButtonAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMeshLodAction, ezButtonAction);

public:
  ezMeshLodAction(const ezActionContext& context, const char* szName);

  virtual void Execute(const ezVariant& value) override;
  virtual void RefreshState() override;

private:
  /// The action's own document, or else every mesh asset in the asset browser selection. Empty when
  /// neither names a mesh asset.
  ///
  /// Non-mesh assets in the selection are dropped rather than disabling the action.
  void GetTargetAssets(ezDynamicArray<ezUuid>& out_assets) const;

  /// A LOD asset must not get LODs of its own, or the _data folders nest without end.
  static bool IsLodAsset(const ezUuid& assetGuid);
};
