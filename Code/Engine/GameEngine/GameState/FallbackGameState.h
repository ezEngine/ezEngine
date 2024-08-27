#pragma once

#include <Core/Graphics/Camera.h>
#include <Foundation/Types/UniquePtr.h>
#include <GameEngine/GameState/GameState.h>
#include <GameEngine/Utils/SceneLoadUtil.h>

class ezCameraComponent;

/// \brief ezFallbackGameState is an ezGameState that can handle existing worlds when no other game state is available.
///
/// This game state returns a priority of 'Fallback' in DeterminePriority() and therefore only takes over when
/// no other game state is available.
/// It implements a simple first person camera to fly around a scene.
///
/// This game state cannot be used in stand-alone applications that require the game state to create
/// a new world. It is mainly for ezEditor and ezPlayer which make sure that a world already exists.
class EZ_GAMEENGINE_DLL ezFallbackGameState : public ezGameState
{
  EZ_ADD_DYNAMIC_REFLECTION(ezFallbackGameState, ezGameState)

public:
  ezFallbackGameState();

  virtual void ProcessInput() override;

  virtual void OnActivation(ezWorld* pWorld, ezStringView sStartPosition, const ezTransform& startPositionOffset) override;

  /// \brief Reports true for ezFallbackGameState only, not for derived types.
  virtual bool IsFallbackGameState() const override;

protected:
  /// \brief Called by SwitchToLoadingScreen() to setup a new world that acts as the loading screen while waiting for another scene to finish loading.
  virtual void ConfigureInputActions() override;
  virtual ezResult SpawnPlayer(ezStringView sStartPosition, const ezTransform& startPositionOffset) override;

  virtual const ezCameraComponent* FindActiveCameraComponent();

  ezInt32 m_iActiveCameraComponentIndex = -3;

  //////////////////////////////////////////////////////////////////////////

  enum class State
  {
    Ok,
    NoProject,
    BadProject,
    NoScene,
    BadScene,
  };

  State m_State = State::Ok;
  bool m_bShowMenu = false;

  void FindAvailableScenes();
  bool DisplayMenu();

  bool m_bCheckedForScenes = false;
  ezDynamicArray<ezString> m_AvailableScenes;
  ezUInt32 m_uiSelectedScene = 0;
  ezString m_sTitleOfScene;

  virtual void OnBackgroundSceneLoadingFinished(ezUniquePtr<ezWorld>&& pWorld) override;
  virtual void OnBackgroundSceneLoadingFailed(ezStringView sReason) override;

  virtual void ConfigureMainCamera() override;
};
