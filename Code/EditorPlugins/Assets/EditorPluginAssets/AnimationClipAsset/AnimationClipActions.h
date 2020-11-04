#pragma once

#include <EditorPluginAssets/EditorPluginAssetsDLL.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

class ezAnimationClipAssetDocument;
struct ezAnimationClipAssetEvent;

class ezAnimationClipActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(const char* szMapping, const char* szPath);

  static ezActionDescriptorHandle s_hCategory;
  static ezActionDescriptorHandle s_hPauseEffect;
  static ezActionDescriptorHandle s_hRestartEffect;
  static ezActionDescriptorHandle s_hAutoRestart;
  static ezActionDescriptorHandle s_hSimulationSpeedMenu;
  static ezActionDescriptorHandle s_hSimulationSpeed[10];
};

class ezAnimationClipAction : public ezButtonAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAnimationClipAction, ezButtonAction);

public:
  enum class ActionType
  {
    Pause,
    Restart,
    Loop,
    SimulationSpeed,
  };

  ezAnimationClipAction(const ezActionContext& context, const char* szName, ActionType type, float fSimSpeed = 1.0f);
  ~ezAnimationClipAction();

  virtual void Execute(const ezVariant& value) override;

private:
  void AnimEventHandler(const ezAnimationClipAssetEvent& e);
  void UpdateState();

  ezAnimationClipAssetDocument* m_pAnimDocument;
  ActionType m_Type;
  float m_fSimSpeed;
};
