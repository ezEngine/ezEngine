#pragma once

#include <EditorFramework/Plugin.h>
#include <GuiFoundation/Action/BaseActions.h>

class ezVisualShaderActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(const char* szMapping);

  static ezActionDescriptorHandle s_hCleanGraph;
};

class ezVisualShaderAction : public ezButtonAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualShaderAction, ezButtonAction);
public:
  ezVisualShaderAction(const ezActionContext& context, const char* name);
  ~ezVisualShaderAction();

  virtual void Execute(const ezVariant& value) override;
};


