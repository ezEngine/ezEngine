#pragma once

#include <EditorFramework/Plugin.h>
#include <GuiFoundation/Action/BaseActions.h>

///
class EZ_EDITORFRAMEWORK_DLL ezAssetActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(const char* szMapping, bool bDocument);

  static ezActionDescriptorHandle s_hAssetCategory;
  static ezActionDescriptorHandle s_hTransformAsset;
  static ezActionDescriptorHandle s_hTransformAllAssets;
  static ezActionDescriptorHandle s_hCheckFileSystem;
  static ezActionDescriptorHandle s_hWriteLookupTable;
  static ezActionDescriptorHandle s_hRetrieveAssetInfo;
};

///
class EZ_EDITORFRAMEWORK_DLL ezAssetAction : public ezButtonAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAssetAction);
public:
  enum class ButtonType
  {
    TransformAsset,
    TransformAllAssets,
    CheckFileSystem,
    WriteLookupTable,
    RetrieveAssetInfo,
  };

  ezAssetAction(const ezActionContext& context, const char* szName, ButtonType button);
  ~ezAssetAction();

  virtual void Execute(const ezVariant& value) override;

private:
  ButtonType m_ButtonType;
};