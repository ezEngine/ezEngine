#pragma once

#include <EditorPluginAssets/EditorPluginAssetsDLL.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/Action/BaseActions.h>

class ezVisualScriptActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(const char* szMapping, const char* szPath);

  static ezActionDescriptorHandle s_hCategory;
  static ezActionDescriptorHandle s_hPickDebugTarget;
};

class ezVisualScriptAction : public ezButtonAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezVisualScriptAction, ezButtonAction);

public:

  enum class ActionType
  {
    PickDebugTarget,
  };

  ezVisualScriptAction(const ezActionContext& context, const char* szName, ActionType type);
  ~ezVisualScriptAction();

  virtual void Execute(const ezVariant& value) override;

private:
  ActionType m_Type;

};




