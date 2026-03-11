#pragma once

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

class ezQtAnimationClipAssetDocumentWindow;

class ezAnimationClipActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(ezStringView sActionMap, ezStringView sSubPath);

  static ezActionDescriptorHandle s_hCategory;
  static ezActionDescriptorHandle s_hRootMotionFromFeet;
};

class ezAnimationClipAction : public ezButtonAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimationClipAction, ezButtonAction);

public:
  enum class ActionType
  {
    RootMotionFromFeet,
  };

  ezAnimationClipAction(const ezActionContext& context, const char* szName, ActionType type);
  ~ezAnimationClipAction();

  virtual void Execute(const ezVariant& value) override;

private:
  ezQtAnimationClipAssetDocumentWindow* m_pAssetWindow = nullptr;
  ActionType m_Type;
};
