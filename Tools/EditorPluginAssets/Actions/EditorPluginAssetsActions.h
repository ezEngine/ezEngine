#pragma once

#include <EditorFramework/Plugin.h>
#include <GuiFoundation/Action/BaseActions.h>

struct ezToolsProjectEvent;

class ezAssetPluginActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(const char* szMapping);

  static ezActionDescriptorHandle s_hImportScene;
};

class ezImportAssetAction : public ezButtonAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezImportAssetAction, ezButtonAction);
public:
  ezImportAssetAction(const ezActionContext& context, const char* name);
  ~ezImportAssetAction();

  void ProjectEventHandler(const ezToolsProjectEvent& e);
  virtual void Execute(const ezVariant& value) override;
};
