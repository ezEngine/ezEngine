#pragma once

#include <EditorPluginTypeScript/EditorPluginTypeScriptDLL.h>

#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

class ezTypeScriptAssetDocument;
struct ezTypeScriptAssetEvent;

class ezTypeScriptActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(ezStringView sMapping);

  static ezActionDescriptorHandle s_hCategory;
  static ezActionDescriptorHandle s_hEditScript;
};

class ezTypeScriptAction : public ezButtonAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTypeScriptAction, ezButtonAction);

public:
  enum class ActionType
  {
    EditScript,
  };

  ezTypeScriptAction(const ezActionContext& context, const char* szName, ActionType type);

  virtual void Execute(const ezVariant& value) override;

private:
  ezTypeScriptAssetDocument* m_pDocument = nullptr;
  ActionType m_Type;
};
