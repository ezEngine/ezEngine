#pragma once

#include <EditorPluginMiniAudio/EditorPluginMiniAudioDLL.h>
#include <Foundation/Configuration/CVar.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

class ezPreferences;

class EZ_EDITORPLUGINMINIAUDIO_DLL ezMiniAudioActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapMenuActions(ezStringView sMapping);
  static void MapToolbarActions(ezStringView sMapping);

  static ezActionDescriptorHandle s_hCategoryMiniAudio;
  static ezActionDescriptorHandle s_hMute;
  static ezActionDescriptorHandle s_hVolume;
};


class EZ_EDITORPLUGINMINIAUDIO_DLL ezMiniAudioAction : public ezButtonAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMiniAudioAction, ezButtonAction);

public:
  enum class ActionType
  {
    Mute,
  };

  ezMiniAudioAction(const ezActionContext& context, const char* szName, ActionType type);
  ~ezMiniAudioAction();

  virtual void Execute(const ezVariant& value) override;

private:
  void OnPreferenceChange(ezPreferences* pref);

  ActionType m_Type;
};

class EZ_EDITORPLUGINMINIAUDIO_DLL ezMiniAudioSliderAction : public ezSliderAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMiniAudioSliderAction, ezSliderAction);

public:
  enum class ActionType
  {
    Volume,
  };

  ezMiniAudioSliderAction(const ezActionContext& context, const char* szName, ActionType type);
  ~ezMiniAudioSliderAction();

  virtual void Execute(const ezVariant& value) override;

private:
  void OnPreferenceChange(ezPreferences* pref);
  void UpdateState();

  ActionType m_Type;
};
