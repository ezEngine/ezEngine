#pragma once

#include <EditorFramework/Plugin.h>
#include <EditorFramework/Settings/Settings.h>
#include <EditorFramework/ContainerWindow/ContainerWindow.moc.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Communication/Event.h>
#include <QApplication>

class QMainWindow;
class QWidget;

struct EZ_EDITORFRAMEWORK_DLL ezPluginSet
{
  ezSet<ezString> m_Plugins;
};

class EZ_EDITORFRAMEWORK_DLL ezEditorFramework
{
public:
  //struct EditorEvent
  //{
  //  enum class EventType
  //  {
  //    OnCreateProject,
  //    BeforeOpenProject,
  //    AfterOpenProject,
  //    BeforeCloseProject,
  //    AfterCloseProject,
  //    OnCreateScene,
  //    BeforeOpenScene,
  //    AfterOpenScene,
  //    BeforeCloseScene,
  //    AfterCloseScene
  //  };

  //  EventType m_Type;
  //  ezString m_sPath;
  //};

  //static ezEvent<const EditorEvent&> s_EditorEvents;

  struct EditorRequest
  {
    enum class Type
    {
      None,
      RequestDocumentWindow,
    };

    EditorRequest()
    {
      m_Type = Type::None;
      m_pResult = nullptr;
    }

    Type m_Type;
    ezString m_sUniqueDocument;
    void* m_pResult;
  };

  static ezEvent<EditorRequest&> s_EditorRequests;

public:

  static const ezString& GetApplicationName() { return s_sApplicationName; }
  //Sstatic const ezString& GetApplicationUserName() { return s_sUserName; }

  static const ezPluginSet& GetEditorPluginsAvailable();
  static const ezPluginSet& GetEditorPluginsActive() { return s_EditorPluginsActive; }
  static const ezPluginSet& GetEditorPluginsToBeLoaded() { return s_EditorPluginsToBeLoaded; }
  static void SetEditorPluginsToBeLoaded(const ezPluginSet& plugins);

  static void AddRestartRequiredReason(ezStringView sReason) { s_RestartRequiredReasons.Insert(sReason); }
  static const ezSet<ezString>& GetRestartRequiredReasons() { return s_RestartRequiredReasons; }

  static void ShowPluginConfigDialog();

  //enum class SettingsCategory
  //{
  //  Editor,
  //  Project,
  //  Scene
  //};

  //static ezSettings& GetSettings(SettingsCategory category, const char* szPlugin = "Main");
  //static void SaveSettings();

  //static const ezString& GetProjectPath() { return s_sProjectPath; }

  static void StartupEditor(ezStringView sAppName, ezStringView sUserName, int argc, char** argv);
  static void ShutdownEditor();
  static ezInt32 RunEditor();

  //static ezResult OpenProject(ezStringView sProjectPath);
  //static ezResult CreateProject(ezStringView sProjectPath);
  //static void CloseProject();
  //static ezString GetProjectDataFolder();

  //static ezResult OpenScene(ezStringView sScenePath);
  //static ezResult CreateScene(ezStringView sScenePath);
  //static void CloseScene();
  //static ezString GetSceneDataFolder();

  static void LoadPlugins();
  static void UnloadPlugins();

  //static ezContainerWindow* GetContainerWindow(const char* szUniqueName, bool bAllowCreate);

  static ezDocumentWindow* GetDocumentWindow(const char* szUniqueName);
  static void AddDocumentWindow(ezDocumentWindow* pWindow);

private:
  static ezHybridArray<ezContainerWindow*, 4> s_ContainerWindows;
  static ezMap<ezString, ezDocumentWindow*> s_DocumentWindows;

  //static ezResult LoadProject();
  //static void UnloadProject();

  //static ezResult LoadScene();
  //static void UnloadScene();

  //static void UpdateEditorWindowTitle();

  static void ReadPluginsToBeLoaded();

  static ezString s_sApplicationName;
  //static ezString s_sUserName;
  //static bool s_bContentModified;

  //static ezString s_sProjectPath;
  //static ezString s_sScenePath;

  static ezSet<ezString> s_RestartRequiredReasons;

  static ezPluginSet s_EditorPluginsAvailable;
  static ezPluginSet s_EditorPluginsActive;
  static ezPluginSet s_EditorPluginsToBeLoaded;

  //static void ClearSettingsProject();
  //static void ClearSettingsScene();

  //static ezMap<ezString, ezSettings> s_Settings[3];

  static QApplication* s_pQtApplication;
};