#pragma once

#include <EditorPluginScene/Plugin.h>
#include <GuiFoundation/Basics.h>
#include <GuiFoundation/Action/BaseActions.h>

///
class EZ_EDITORPLUGINSCENE_DLL ezSceneViewActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(const char* szMapping, const char* szPath);

  static ezActionDescriptorHandle s_hToggleViews;
  static ezActionDescriptorHandle s_hSpawnView;
};

///
class EZ_EDITORPLUGINSCENE_DLL ezSceneViewAction : public ezButtonAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSceneViewAction);
public:
  enum class ButtonType
  {
    ToggleViews,
    SpawnView,
  };

  ezSceneViewAction(const ezActionContext& context, const char* szName, ButtonType button);
  ~ezSceneViewAction();

  virtual void Execute(const ezVariant& value) override;

private:
  ButtonType m_ButtonType;
};