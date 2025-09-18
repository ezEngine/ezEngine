#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <GameEngine/GameState/FallbackGameState.h>
#include <RTSPlugin/GameMode/BattleMode/BattleMode.h>
#include <RTSPlugin/GameMode/EditLevelMode/EditLevelMode.h>
#include <RTSPlugin/GameMode/MainMenuMode/MainMenuMode.h>
#include <RTSPlugin/RTSPluginDLL.h>
#include <Utilities/DataStructures/ObjectSelection.h>

class RtsGameMode;
using ezCollectionResourceHandle = ezTypedResourceHandle<class ezCollectionResource>;

enum class RtsActiveGameMode
{
  None,
  MainMenuMode,
  BattleMode,
  EditLevelMode,
};

// the ezFallbackGameState adds a free flying camera and a scene switching menu, so can be useful in the very beginning
// but generally it's better to use ezGameState instead
// using RTSGameStateBase = ezFallbackGameState;
using RTSGameStateBase = ezGameState;

class RTSGameState : public RTSGameStateBase
{
  EZ_ADD_DYNAMIC_REFLECTION(RTSGameState, RTSGameStateBase);

  static RTSGameState* s_pSingleton;

public:
  RTSGameState();
  ~RTSGameState();

  static RTSGameState* GetSingleton() { return s_pSingleton; }

  virtual void RequestQuit(ezStringView sRequestedBy) override;

protected:
  virtual void ConfigureMainCamera() override;
  virtual void OnChangedMainWorld(ezWorld* pPrevWorld, ezWorld* pNewWorld, ezStringView sStartPosition, const ezTransform& startPositionOffset) override;
  virtual ezString GetStartupSceneFile() override;

private:
  virtual void OnActivation(ezWorld* pWorld, ezStringView sStartPosition, const ezTransform& startPositionOffset) override;
  virtual void OnDeactivation() override;
  virtual void BeforeWorldUpdate() override;
  void PreloadAssets();

  ezCollectionResourceHandle m_hCollectionSpace;
  ezCollectionResourceHandle m_hCollectionFederation;
  ezCollectionResourceHandle m_hCollectionKlingons;

  ezDeque<ezGameObjectHandle> m_SpawnedObjects;

  //////////////////////////////////////////////////////////////////////////
  // Camera
public:
  float GetCameraZoom() const;
  float SetCameraZoom(float fZoom);

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
