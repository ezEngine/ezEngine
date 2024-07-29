#pragma once

#include <RtsGamePlugin/GameMode/BattleMode/BattleMode.h>
#include <RtsGamePlugin/GameMode/EditLevelMode/EditLevelMode.h>
#include <RtsGamePlugin/GameMode/MainMenuMode/MainMenuMode.h>
#include <RtsGamePlugin/RtsGamePluginDLL.h>

class RtsGameMode;
using ezCollectionResourceHandle = ezTypedResourceHandle<class ezCollectionResource>;

enum class RtsActiveGameMode
{
  None,
  MainMenuMode,
  BattleMode,
  EditLevelMode,
};

class EZ_RTSGAMEPLUGIN_DLL RtsGameState : public ezFallbackGameState
{
  EZ_ADD_DYNAMIC_REFLECTION(RtsGameState, ezFallbackGameState);

  static RtsGameState* s_pSingleton;

public:
  RtsGameState();

  static RtsGameState* GetSingleton() { return s_pSingleton; }

  //////////////////////////////////////////////////////////////////////////
  // Initialization & Setup
public:
  virtual ezGameStatePriority DeterminePriority(ezWorld* pWorld) const override;

private:
  virtual void OnActivation(ezWorld* pWorld, const ezTransform* pStartPosition) override;
  virtual void OnDeactivation() override;
  void PreloadAssets();

  ezCollectionResourceHandle m_hCollectionSpace;
  ezCollectionResourceHandle m_hCollectionFederation;
  ezCollectionResourceHandle m_hCollectionKlingons;

  //////////////////////////////////////////////////////////////////////////
  // World Updates
private:
  virtual void BeforeWorldUpdate() override;

  //////////////////////////////////////////////////////////////////////////
  // Camera
public:
  float GetCameraZoom() const;
  float SetCameraZoom(float fZoom);

protected:
  virtual void ConfigureMainCamera() override;

  virtual void OnChangedMainWorld() override;

  //////////////////////////////////////////////////////////////////////////
  // Game Mode
public:
  void SwitchToGameMode(RtsActiveGameMode mode);
  RtsActiveGameMode GetActiveGameMode() const { return m_GameModeToSwitchTo; }

private:
  void ActivateQueuedGameMode();
  void SetActiveGameMode(RtsGameMode* pMode);

  RtsActiveGameMode m_GameModeToSwitchTo = RtsActiveGameMode::None;
  RtsGameMode* m_pActiveGameMode = nullptr;

  // all the modes that the game has
  RtsMainMenuMode m_MainMenuMode;
  RtsBattleMode m_BattleMode;
  RtsEditLevelMode m_EditLevelMode;

  //////////////////////////////////////////////////////////////////////////
  // Input Handling
private:
  virtual void ConfigureMainWindowInputDevices(ezWindow* pWindow) override;
  virtual void ConfigureInputActions() override;
  virtual void ProcessInput() override;
  void UpdateMousePosition();

  RtsMouseInputState m_MouseInputState;
  float m_fCameraZoom = 10.0f;

  //////////////////////////////////////////////////////////////////////////
  // Picking
public:
  ezResult PickGroundPlanePosition(ezVec3& out_vPositon) const;
  ezGameObject* PickSelectableObject() const;
  void InspectObjectsInArea(const ezVec2& vPosition, float fRadius, ezSpatialSystem::QueryCallback callback) const;

private:
  ezResult ComputePickingRay();

  ezVec3 m_vCurrentPickingRayStart;
  ezVec3 m_vCurrentPickingRayDir;

  //////////////////////////////////////////////////////////////////////////
  // Spawning Objects
public:
  ezGameObject* SpawnNamedObjectAt(const ezTransform& transform, const char* szObjectName, ezUInt16 uiTeamID);

  //////////////////////////////////////////////////////////////////////////
  // Units
public:
  ezGameObject* DetectHoveredSelectable();
  void SelectUnits();
  void RenderUnitSelection() const;
  void RenderUnitHealthbar(ezGameObject* pObject, float fSelectableRadius) const;

  ezGameObjectHandle m_hHoveredSelectable;
  ezObjectSelection m_SelectedUnits;
};
