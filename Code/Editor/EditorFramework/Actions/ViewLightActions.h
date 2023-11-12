#pragma once

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <GuiFoundation/Action/BaseActions.h>

/// \brief Actions for configuring the engine view light settings.
class EZ_EDITORFRAMEWORK_DLL ezViewLightActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapToolbarActions(ezStringView sMapping);

  static ezActionDescriptorHandle s_hLightMenu;
  static ezActionDescriptorHandle s_hSkyBox;
  static ezActionDescriptorHandle s_hSkyLight;
  static ezActionDescriptorHandle s_hSkyLightCubeMap;
  static ezActionDescriptorHandle s_hSkyLightIntensity;
  static ezActionDescriptorHandle s_hDirLight;
  static ezActionDescriptorHandle s_hDirLightAngle;
  static ezActionDescriptorHandle s_hDirLightShadows;
  static ezActionDescriptorHandle s_hDirLightIntensity;
  static ezActionDescriptorHandle s_hFog;
  static ezActionDescriptorHandle s_hSetAsDefault;
};

class EZ_EDITORFRAMEWORK_DLL ezViewLightButtonAction : public ezButtonAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezViewLightButtonAction, ezButtonAction);

public:
  ezViewLightButtonAction(const ezActionContext& context, const char* szName, ezEngineViewLightSettingsEvent::Type button);
  ~ezViewLightButtonAction();

  virtual void Execute(const ezVariant& value) override;
  void LightSettingsEventHandler(const ezEngineViewLightSettingsEvent& e);
  void UpdateAction();

private:
  ezEngineViewLightSettingsEvent::Type m_ButtonType;
  ezEngineViewLightSettings* m_pSettings = nullptr;
  ezEventSubscriptionID m_SettingsID;
};

class EZ_EDITORFRAMEWORK_DLL ezViewLightSliderAction : public ezSliderAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezViewLightSliderAction, ezSliderAction);

public:
  ezViewLightSliderAction(const ezActionContext& context, const char* szName, ezEngineViewLightSettingsEvent::Type button);
  ~ezViewLightSliderAction();

  virtual void Execute(const ezVariant& value) override;
  void LightSettingsEventHandler(const ezEngineViewLightSettingsEvent& e);
  void UpdateAction();

private:
  ezEngineViewLightSettingsEvent::Type m_ButtonType;
  ezEngineViewLightSettings* m_pSettings = nullptr;
  ezEventSubscriptionID m_SettingsID;
};
