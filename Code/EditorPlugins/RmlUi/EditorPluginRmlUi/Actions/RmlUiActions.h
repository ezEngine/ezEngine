#pragma once

#include <EditorPluginRmlUi/EditorPluginRmlUiDLL.h>

#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

class ezRmlUiAssetDocument;
struct ezRmlUiAssetEvent;

class ezRmlUiActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActionsMenu(ezStringView sMapping);
  static void MapActionsToolbar(ezStringView sMapping);

  static ezActionDescriptorHandle s_hCategory;
  static ezActionDescriptorHandle s_hOpenInVSC;
};

class ezRmlUiAction : public ezButtonAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRmlUiAction, ezButtonAction);

public:
  enum class ActionType
  {
    OpenInVSC,
  };

  ezRmlUiAction(const ezActionContext& context, const char* szName, ActionType type);

  virtual void Execute(const ezVariant& value) override;

private:
  ezRmlUiAssetDocument* m_pDocument = nullptr;
  ActionType m_Type;
};
