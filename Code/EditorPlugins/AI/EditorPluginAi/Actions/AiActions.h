#pragma once

#include <EditorPluginAi/EditorPluginAiDLL.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

class EZ_EDITORPLUGINAI_DLL ezAiActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapMenuActions();

  static ezActionDescriptorHandle s_hCategoryAi;
  static ezActionDescriptorHandle s_hProjectSettings;
};

class EZ_EDITORPLUGINAI_DLL ezAiAction : public ezButtonAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAiAction, ezButtonAction);

public:
  enum class ActionType
  {
    ProjectSettings,
  };

  ezAiAction(const ezActionContext& context, const char* szName, ActionType type);
  ~ezAiAction();

  virtual void Execute(const ezVariant& value) override;

private:
  ActionType m_Type;
};
