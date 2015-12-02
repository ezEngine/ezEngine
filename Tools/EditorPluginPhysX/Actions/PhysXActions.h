#pragma once

#include <EditorPluginPhysX/Plugin.h>
#include <GuiFoundation/Basics.h>
#include <GuiFoundation/Action/BaseActions.h>


class EZ_EDITORPLUGINPHYSX_DLL ezPhysXActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapMenuActions();

  static ezActionDescriptorHandle s_hCategoryPhysX;
  static ezActionDescriptorHandle s_hProjectSettings;
  static ezActionDescriptorHandle s_hSceneSettings;
};


class EZ_EDITORPLUGINPHYSX_DLL ezPhysXAction : public ezButtonAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezPhysXAction, ezButtonAction);
public:

  enum class ActionType
  {
    ProjectSettings,
    SceneSettings,
  };

  ezPhysXAction(const ezActionContext& context, const char* szName, ActionType type);
  ~ezPhysXAction();

  virtual void Execute(const ezVariant& value) override;

private:

  ActionType m_Type;
};

