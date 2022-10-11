#pragma once

#include <EditorPluginJolt/EditorPluginJoltDLL.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

class EZ_EDITORPLUGINJOLT_DLL ezJoltActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapMenuActions();

  static ezActionDescriptorHandle s_hCategoryJolt;
  static ezActionDescriptorHandle s_hProjectSettings;
};

class EZ_EDITORPLUGINJOLT_DLL ezJoltAction : public ezButtonAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezJoltAction, ezButtonAction);

public:
  enum class ActionType
  {
    ProjectSettings,
  };

  ezJoltAction(const ezActionContext& context, const char* szName, ActionType type);
  ~ezJoltAction();

  virtual void Execute(const ezVariant& value) override;

private:
  ActionType m_Type;
};
