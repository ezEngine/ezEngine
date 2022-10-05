#pragma once

#include <GameEngine/GameApplication/GameApplication.h>

class ezPlayerApplication : public ezGameApplication
{
public:
  typedef ezGameApplication SUPER;

  ezPlayerApplication();

protected:
  virtual void Run_InputUpdate() override;
  virtual ezResult BeforeCoreSystemsStartup() override;
  virtual void AfterCoreSystemsStartup() override;
  virtual void BeforeHighLevelSystemsShutdown() override;

private:
  ezResult DetermineProjectPath();
  ezResult DetermineScenePath();

  /// \brief Attempts to load an '.ezScene' or '.ezPrefab' file into the main world.
  ezResult LoadScene(const char* szSceneFile);

  /// \brief Attempts to load an '.ezObjectGraph' file into the main world.
  ezResult LoadObjectGraph(const char* szFile);

  /// \brief Finds all available '.ezScene' files in this project.
  ///
  /// Note that only files that are properly 'transformed', ie have corresponding '.ezObjectGraph' files, will be loadable.
  void FindAvailableScenes();

  bool DisplayMenu();

  enum class State
  {
    Ok,
    NoProject,
    BadProject,
    NoScene,
    BadScene,
  };

  enum class Menu
  {
    None,
    SceneSelection,
  };

  State m_State = State::Ok;
  Menu m_Menu = Menu::None;

  ezString m_sSceneFile;
  ezUniquePtr<ezWorld> m_pWorld;
  bool m_bCheckedForScenes = false;
  ezDynamicArray<ezString> m_AvailableScenes;
  ezUInt32 m_uiSelectedScene = 0;
};
