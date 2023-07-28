#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

///
class EZ_EDITORFRAMEWORK_DLL ezQuadViewActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapToolbarActions(ezStringView sMapping);

  static ezActionDescriptorHandle s_hToggleViews;
  static ezActionDescriptorHandle s_hSpawnView;
};

///
class EZ_EDITORFRAMEWORK_DLL ezQuadViewAction : public ezButtonAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezQuadViewAction, ezButtonAction);

public:
  enum class ButtonType
  {
    ToggleViews,
    SpawnView,
  };

  ezQuadViewAction(const ezActionContext& context, const char* szName, ButtonType button);
  ~ezQuadViewAction();

  virtual void Execute(const ezVariant& value) override;

private:
  ButtonType m_ButtonType;
};
