#pragma once

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

class ezAssetDocument;

class EZ_EDITORFRAMEWORK_DLL ezCommonAssetActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(const char* szMapping, const char* szPath, ezUInt32 stateMask);

  static ezActionDescriptorHandle s_hCategory;
  static ezActionDescriptorHandle s_hPause;
  static ezActionDescriptorHandle s_hRestart;
  static ezActionDescriptorHandle s_hLoop;
  static ezActionDescriptorHandle s_hSimulationSpeedMenu;
  static ezActionDescriptorHandle s_hSimulationSpeed[10];
  static ezActionDescriptorHandle s_hGrid;
  static ezActionDescriptorHandle s_hVisualizers;
};

class EZ_EDITORFRAMEWORK_DLL ezCommonAssetAction : public ezButtonAction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCommonAssetAction, ezButtonAction);

public:
  enum class ActionType
  {
    Pause,
    Restart,
    Loop,
    SimulationSpeed,
    Grid,
    Visualizers,
  };

  ezCommonAssetAction(const ezActionContext& context, const char* szName, ActionType type, float fSimSpeed = 1.0f);
  ~ezCommonAssetAction();

  virtual void Execute(const ezVariant& value) override;

private:
  void CommonUiEventHandler(const ezCommonAssetUiState& e);
  void UpdateState();

  ezAssetDocument* m_pAssetDocument = nullptr;
  ActionType m_Type;
  float m_fSimSpeed;
};
