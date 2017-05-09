#pragma once

#include <EditorPluginFmod/Plugin.h>
#include <GuiFoundation/Basics.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <Foundation/Configuration/CVar.h>

class EZ_EDITORPLUGINFMOD_DLL ezFmodActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapMenuActions();

  static ezActionDescriptorHandle s_hCategoryFmod;
  static ezActionDescriptorHandle s_hProjectSettings;
  static ezActionDescriptorHandle s_hMuteSound;
};


class EZ_EDITORPLUGINFMOD_DLL ezFmodAction : public ezButtonAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezFmodAction, ezButtonAction);
public:

  enum class ActionType
  {
    ProjectSettings,
    MuteSound,
  };

  ezFmodAction(const ezActionContext& context, const char* szName, ActionType type);
  ~ezFmodAction();

  virtual void Execute(const ezVariant& value) override;

private:
  void CVarEventHandler(const ezCVar::CVarEvent& e);

  ActionType m_Type;
};

