#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <GuiFoundation/Action/BaseActions.h>

///
class EZ_EDITORFRAMEWORK_DLL ezAssetActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapMenuActions(ezStringView sMapping);
  static void MapToolBarActions(ezStringView sMapping, bool bDocument);

  static ezActionDescriptorHandle s_hAssetCategory;
  static ezActionDescriptorHandle s_hTransformAsset;
  static ezActionDescriptorHandle s_hAssetHelp;
  static ezActionDescriptorHandle s_hTransformAllAssets;
  static ezActionDescriptorHandle s_hCheckFileSystem;
  static ezActionDescriptorHandle s_hWriteDependencyDGML;
  static ezActionDescriptorHandle s_hCopyAssetGuid;
};

///
class EZ_EDITORFRAMEWORK_DLL ezAssetAction : public ezButtonAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAssetAction, ezButtonAction);

public:
  enum class ButtonType
  {
    TransformAsset,
    AssetHelp,
    TransformAllAssets,
    CheckFileSystem,
    WriteDependencyDGML,
    CopyAssetGuid,
  };

  ezAssetAction(const ezActionContext& context, const char* szName, ButtonType button);
  ~ezAssetAction();

  virtual void Execute(const ezVariant& value) override;

private:
  ButtonType m_ButtonType;
};
