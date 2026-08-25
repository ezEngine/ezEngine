#pragma once

#include <EditorPluginJolt/EditorPluginJoltDLL.h>
#include <EditorPluginJolt/Utils/MeshColliderCreator.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Types/Uuid.h>
#include <GuiFoundation/Action/BaseActions.h>

/// Creates Jolt collision mesh assets from mesh assets. Hides itself unless the target is one or
/// more mesh assets.
class ezMeshColliderActions
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
  static ezActionDescriptorHandle s_hCreateCollider;
  static ezActionDescriptorHandle s_hCreateColliderDoc;
};

class ezMeshColliderAction : public ezButtonAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMeshColliderAction, ezButtonAction);

public:
  ezMeshColliderAction(const ezActionContext& context, const char* szName);

  virtual void Execute(const ezVariant& value) override;
  virtual void RefreshState() override;

private:
  /// The action's own document, or else every mesh asset in the asset browser selection. Empty when
  /// neither names a mesh asset.
  ///
  /// Non-mesh assets in the selection are dropped rather than disabling the action.
  void GetTargetAssets(ezDynamicArray<ezUuid>& out_assets) const;
};
