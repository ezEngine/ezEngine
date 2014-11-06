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
  static const ezString& GetApplicationUserName() { return s_sUserName; }

  static const ezPluginSet& GetEditorPluginsAvailable();
  static const ezPluginSet& GetEditorPluginsActive() { return s_EditorPluginsActive; }
  static const ezPluginSet& GetEditorPluginsToBeLoaded() { return s_EditorPluginsToBeLoaded; }
  static void SetEditorPluginsToBeLoaded(const ezPluginSet& plugins);

  static void AddRestartRequiredReason(const char* szReason) { s_RestartRequiredReasons.Insert(szReason); }
  static const ezSet<ezString>& GetRestartRequiredReasons() { return s_RestartRequiredReasons; }

  static void ShowPluginConfigDialog();

  static ezSettings& GetEditorSettings(const char* szPlugin = "Main");
  static ezSettings& GetProjectSettings(const char* szPlugin = "Main");
  static ezSettings& GetDocumentSettings(const char* szDocument, const char* szPlugin = "Main");
  static void SaveSettings();

  //static const ezString& GetProjectPath() { return s_sProjectPath; }

  static void StartupEditor(const char* szAppName, const char* szUserName, int argc, char** argv);
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
  static ezSettings& GetSettings(ezMap<ezString, ezSettings>& SettingsMap, const char* szPlugin, const char* szSearchPath);

  static ezHybridArray<ezContainerWindow*, 4> s_ContainerWindows;
  static ezMap<ezString, ezDocumentWindow*> s_DocumentWindows;

  //static ezResult LoadProject();
  //static void UnloadProject();

  //static ezResult LoadScene();
  //static void UnloadScene();

  //static void UpdateEditorWindowTitle();

  static void ReadPluginsToBeLoaded();

  static ezString s_sApplicationName;
  static ezString s_sUserName;
  //static bool s_bContentModified;

  //static ezString s_sProjectPath;
  //static ezString s_sScenePath;

  static ezSet<ezString> s_RestartRequiredReasons;

  static ezPluginSet s_EditorPluginsAvailable;
  static ezPluginSet s_EditorPluginsActive;
  static ezPluginSet s_EditorPluginsToBeLoaded;

  //static void ClearSettingsProject();
  //static void ClearSettingsScene();

  static ezMap<ezString, ezSettings> s_EditorSettings;
  static ezMap<ezString, ezSettings> s_ProjectSettings;
  static ezMap<ezString, ezMap<ezString, ezSettings> > s_DocumentSettings;

  static QApplication* s_pQtApplication;
};