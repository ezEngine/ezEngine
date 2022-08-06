#pragma once

#include <EditorPluginAudioSystem/EditorPluginAudioSystemDLL.h>

#include <Foundation/Configuration/CVar.h>

#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

class ezPreferences;

class EZ_EDITORPLUGINAUDIOSYSTEM_DLL ezAudioSystemActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapMenuActions(const char* szMapping);
  static void MapToolbarActions(const char* szMapping);

  static ezActionDescriptorHandle s_hCategoryAudioSystem;
  static ezActionDescriptorHandle s_hProjectSettings;
  static ezActionDescriptorHandle s_hMuteSound;
  static ezActionDescriptorHandle s_hReloadControls;
  static ezActionDescriptorHandle s_hMasterVolume;
};


class EZ_EDITORPLUGINAUDIOSYSTEM_DLL ezAudioSystemAction : public ezButtonAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAudioSystemAction, ezButtonAction);

public:
  enum class ActionType
  {
    ProjectSettings,
    MuteSound,
    ReloadControls
  };

  ezAudioSystemAction(const ezActionContext& context, const char* szName, ActionType type);
  ~ezAudioSystemAction();

  virtual void Execute(const ezVariant& value) override;

private:
  void OnPreferenceChange(ezPreferences* pref);

  ActionType m_Type;
};

class EZ_EDITORPLUGINAUDIOSYSTEM_DLL ezAudioSystemSliderAction : public ezSliderAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAudioSystemSliderAction, ezSliderAction);

public:
  enum class ActionType
  {
    MasterVolume,
  };

  ezAudioSystemSliderAction(const ezActionContext& context, const char* szName, ActionType type);
  ~ezAudioSystemSliderAction();

  virtual void Execute(const ezVariant& value) override;

private:
  void OnPreferenceChange(ezPreferences* pref);
  void UpdateState();

  ActionType m_Type;
};
