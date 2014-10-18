#pragma once

#include <EditorFramework/Plugin.h>
#include <EditorFramework/Settings/Settings.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Communication/Event.h>

class QMainWindow;

struct EZ_EDITORFRAMEWORK_DLL ezPluginSet
{
  ezSet<ezString> m_Plugins;
};

class EZ_EDITORFRAMEWORK_DLL ezEditorFramework
{
public:
  struct EditorEvent
  {
    enum class EventType
    {
      OnCreateProject,
      BeforeOpenProject,
      AfterOpenProject,
      BeforeCloseProject,
      AfterCloseProject,
      OnCreateScene,
      BeforeOpenScene,
      AfterOpenScene,
      BeforeCloseScene,
      AfterCloseScene
    };

    EventType m_Type;
    ezString m_sPath;
  };

  static ezEvent<const EditorEvent&> s_EditorEvents;

public:

  static void SetMainWindow(QMainWindow* pWindow) { s_pMainWindow = pWindow; }
  static QMainWindow* GetMainWindow() { return s_pMainWindow; }
  static const ezString& GetApplicationName() { return s_sApplicationName; }

  static const ezPluginSet& GetEditorPluginsAvailable();
  static const ezPluginSet& GetEditorPluginsActive() { return s_EditorPluginsActive; }
  static const ezPluginSet& GetEditorPluginsToBeLoaded() { return s_EditorPluginsToBeLoaded; }
  static void SetEditorPluginsToBeLoaded(const ezPluginSet& plugins);

  static void AddRestartRequiredReason(const ezString& sReason) { s_RestartRequiredReasons.Insert(sReason); }
  static const ezSet<ezString>& GetRestartRequiredReasons() { return s_RestartRequiredReasons; }

  static void SaveWindowLayout();
  static void RestoreWindowLayout();

  enum class SettingsCategory
  {
    Editor,
    Project,
    Scene
  };

  static ezSettings& GetSettings(SettingsCategory category, const char* szPlugin = "Main");
  static void SaveSettings();

  static const ezString& GetProjectPath() { return s_sProjectPath; }
  static const ezString& GetScenePath() { return s_sScenePath; }

  static void StartupEditor(const ezString& sAppName);
  static void ShutdownEditor();

  static ezResult OpenProject(const ezString& sProjectPath);
  static ezResult CreateProject(const ezString& sProjectPath);
  static void CloseProject();
  static ezString GetProjectDataFolder();

  static ezResult OpenScene(const ezString& sScenePath);
  static ezResult CreateScene(const ezString& sScenePath);
  static void CloseScene();
  static ezString GetSceneDataFolder();

  static void LoadPlugins();

private:

  static ezResult LoadProject();
  static void UnloadProject();

  static ezResult LoadScene();
  static void UnloadScene();

  static void UpdateEditorWindowTitle();

  static void ReadPluginsToBeLoaded();

  static ezString s_sApplicationName;
  static bool s_bContentModified;

  static ezString s_sProjectPath;
  static ezString s_sScenePath;

  static ezSet<ezString> s_RestartRequiredReasons;

  static ezPluginSet s_EditorPluginsAvailable;
  static ezPluginSet s_EditorPluginsActive;
  static ezPluginSet s_EditorPluginsToBeLoaded;

  static QMainWindow* s_pMainWindow;

  static void ClearSettingsProject();
  static void ClearSettingsScene();

  static ezMap<ezString, ezSettings> s_Settings[3];
};