#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Types/UniquePtr.h>
#include <Foundation/Utilities/Progress.h>
#include <GameEngine/GameEngineDLL.h>

using ezCollectionResourceHandle = ezTypedResourceHandle<class ezCollectionResource>;

/// \brief This class allows to load a scene in the background and switch to it, once loading has finished.
class EZ_GAMEENGINE_DLL ezSceneLoadUtility
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezSceneLoadUtility);

public:
  ezSceneLoadUtility();
  ~ezSceneLoadUtility();

  enum class LoadingState
  {
    NotStarted,
    Ongoing,
    FinishedSuccessfully,
    Failed,
  };

  /// \brief Returns whether loading is still ongoing or finished.
  LoadingState GetLoadingState() const { return m_LoadingState; }

  /// \brief Returns a loading progress value in 0 to 1 range.
  float GetLoadingProgress() const { return m_fLoadingProgress; }

  /// \brief In case loading failed, this returns what went wrong.
  ezStringView GetLoadingFailureReason() const { return m_sFailureReason; }

  /// \brief Starts loading a scene. If provided, the assets in the collection are loaded first and then the scene is instantiated.
  ///
  /// Using a collection will make loading in the background much smoother. Without it, most assets will be loaded once the scene gets updated
  /// for the first time, resulting in very long delays.
  void StartSceneLoading(ezStringView sSceneFile, ezStringView sPreloadCollectionFile);

  /// \brief This has to be called periodically (usually once per frame) to progress the scene loading.
  ///
  /// Call GetLoadingState() afterwards to check whether loading has finished or failed.
  void TickSceneLoading();

  /// \brief Once loading is finished successfully, call this to take ownership of the loaded scene.
  ///
  /// Afterwards there is no point in keeping the ezSceneLoadUtility around anymore and it should be deleted.
  ezUniquePtr<ezWorld> RetrieveLoadedScene();

  /// \brief Returns the path to the scene file as it was originally requested.
  ezStringView GetRequestedScene() const { return m_sRequestedFile; }

  /// \brief Returns the path to the scene file after it was redirected.
  ezStringView GetRedirectedScene() const { return m_sRedirectedFile; }

private:
  void LoadingFailed(const ezFormatString& reason);

  LoadingState m_LoadingState = LoadingState::NotStarted;
  float m_fLoadingProgress = 0.0f;
  ezString m_sFailureReason;

  ezString m_sRequestedFile;
  ezString m_sRedirectedFile;
  ezCollectionResourceHandle m_hPreloadCollection;
  ezFileReader m_FileReader;
  ezWorldReader m_WorldReader;
  ezUniquePtr<ezWorld> m_pWorld;
  ezUniquePtr<ezWorldReader::InstantiationContextBase> m_pInstantiationContext;
  ezProgress m_InstantiationProgress;
};
