#pragma once

#include <EditorPluginProceduralPlacement/Plugin.h>
#include <GuiFoundation/Basics.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <Foundation/Configuration/CVar.h>

#if 0
class ezPreferences;

class EZ_EDITORPLUGINFMOD_DLL ezFmodActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapMenuActions();

  static ezActionDescriptorHandle s_hCategoryFmod;
  static ezActionDescriptorHandle s_hProjectSettings;
  static ezActionDescriptorHandle s_hMuteSound;
  static ezActionDescriptorHandle s_hMasterVolume;
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
  void OnPreferenceChange(ezPreferences* pref);

  ActionType m_Type;
};

class EZ_EDITORPLUGINFMOD_DLL ezFmodSliderAction : public ezSliderAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezFmodSliderAction, ezSliderAction);

public:

  enum class ActionType
  {
    MasterVolume,
  };

  ezFmodSliderAction(const ezActionContext& context, const char* szName, ActionType type);
  ~ezFmodSliderAction();

  virtual void Execute(const ezVariant& value) override;

private:
  void OnPreferenceChange(ezPreferences* pref);
  void UpdateState();

  ActionType m_Type;
};
#endif
