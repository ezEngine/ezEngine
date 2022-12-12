#pragma once

#include <Core/Graphics/Camera.h>
#include <Foundation/Types/UniquePtr.h>
#include <GameEngine/GameState/GameState.h>
#include <GameEngine/Utils/SceneLoadUtil.h>

class ezCameraComponent;

using ezCollectionResourceHandle = ezTypedResourceHandle<class ezCollectionResource>;

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

  /// \brief If disabled, pressing the Windows key won't show an onscreen menu to switch to a different scene.
  void EnableSceneSelectionMenu(bool bEnable);

  virtual void ProcessInput() override;
  virtual void AfterWorldUpdate() override;

  /// \brief Returns ezGameStatePriority::Fallback.
  virtual ezGameStatePriority DeterminePriority(ezWorld* pWorld) const override;

  virtual void OnActivation(ezWorld* pWorld, const ezTransform* pStartPosition) override;
  virtual void OnDeactivation() override;

  /// \brief Returns the path to the scene file to load at startup. By default this is taken from the command line '-scene' option.
  virtual ezString GetStartupSceneFile();

  /// \brief Creates a new world that's used as a temporary loading screen while waiting for loading of another world to finish.
  ///
  /// Usually this world would be set up in code and would be very quick to create. By default an entirely empty world is created.
  void SwitchToLoadingScreen();

  ezResult StartSceneLoading(ezStringView sSceneFile, ezStringView sPreloadCollection);
  void CancelSceneLoading();

  bool IsLoadingScene() const;
  bool IsInLoadingScreen() const;
  void SwitchToLoadedScene();

protected:
  virtual ezUniquePtr<ezWorld> CreateLoadingScreenWorld();
  virtual void ConfigureInputActions() override;
  virtual ezResult SpawnPlayer(const ezTransform* pStartPosition) override;

  virtual const ezCameraComponent* FindActiveCameraComponent();

  ezInt32 m_iActiveCameraComponentIndex;

  ezUniquePtr<ezWorld> m_pActiveWorld;

  ezUniquePtr<ezSceneLoadUtility> m_pSceneToLoad;

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
  bool m_bEnableSceneSelectionMenu = true;
  bool m_bIsInLoadingScreen = false;

  void FindAvailableScenes();
  bool DisplayMenu();

  bool m_bCheckedForScenes = false;
  ezDynamicArray<ezString> m_AvailableScenes;
  ezUInt32 m_uiSelectedScene = 0;
  ezString m_sTitleOfLoadingScene;
  ezString m_sTitleOfActiveScene;
};

