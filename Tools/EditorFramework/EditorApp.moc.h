#pragma once

#include <EditorFramework/Plugin.h>
#include <EditorFramework/Settings/Settings.h>
#include <EditorFramework/ContainerWindow/ContainerWindow.moc.h>
#include <EditorFramework/Project/EditorProject.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Communication/Event.h>
#include <QApplication>

class QMainWindow;
class QWidget;
class ezEditorEngineViewProcess;

struct EZ_EDITORFRAMEWORK_DLL ezPluginSet
{
  ezSet<ezString> m_Plugins;
};

class EZ_EDITORFRAMEWORK_DLL ezRecentFilesList
{
public:
  ezRecentFilesList(ezUInt32 uiMaxElements) { m_uiMaxElements = uiMaxElements; }

  void Insert(const char* szFile);
  const ezDeque<ezString>& GetFileList() const { return m_Files; }
  void Clear() { m_Files.Clear(); }

  void Save(const char* szFile);
  void Load(const char* szFile);

private:
  ezUInt32 m_uiMaxElements;
  ezDeque<ezString> m_Files;
};

class EZ_EDITORFRAMEWORK_DLL ezEditorApp : public QObject
{
  Q_OBJECT

  static ezEditorApp* s_pInstance;

public:
  ezEditorApp();
  ~ezEditorApp();

  static ezEditorApp* GetInstance() { return s_pInstance; }

  QMainWindow* GetMainWindow();

  const ezString& GetApplicationName() { return s_sApplicationName; }
  const ezString& GetApplicationUserName() { return s_sUserName; }

  const ezPluginSet& GetEditorPluginsAvailable();
  const ezPluginSet& GetEditorPluginsActive() { return s_EditorPluginsActive; }
  const ezPluginSet& GetEditorPluginsToBeLoaded() { return s_EditorPluginsToBeLoaded; }
  void SetEditorPluginsToBeLoaded(const ezPluginSet& plugins);

  void AddRestartRequiredReason(const char* szReason) { s_RestartRequiredReasons.Insert(szReason); }
  const ezSet<ezString>& GetRestartRequiredReasons() { return s_RestartRequiredReasons; }

  void ShowPluginConfigDialog();

  void RegisterPluginNameForSettings(const char* szPluginName);
  const ezSet<ezString>& GetRegisteredPluginNamesForSettings() { return s_SettingsPluginNames; }
  ezSettings& GetEditorSettings(const char* szPluginName = "-Main-");
  ezSettings& GetProjectSettings(const char* szPluginName = "-Main-");
  ezSettings& GetDocumentSettings(const ezDocumentBase* pDocument, const char* szPluginName = "-Main-");
  ezSettings& GetDocumentSettings(const char* szDocument, const char* szPlugin = "-Main-");
  void SaveSettings();

  void StartupEditor(const char* szAppName, const char* szUserName, int argc, char** argv);
  void ShutdownEditor();
  ezInt32 RunEditor();

  void LoadPlugins();
  void UnloadPlugins();

  ezDocumentWindow* GetDocumentWindow(const char* szUniqueName);
  void AddDocumentWindow(ezDocumentWindow* pWindow);

  ezRecentFilesList& GetRecentProjectsList()   { return s_RecentProjects;  }
  ezRecentFilesList& GetRecentDocumentsList()  { return s_RecentDocuments; }

  ezString GetDocumentDataFolder(const char* szDocument);

  ezEditorEngineViewProcess* GetEngineViewProcess() { return s_pEngineViewProcess; }

private slots:
  void SlotTimedUpdate();

private:
  ezSettings& GetSettings(ezMap<ezString, ezSettings>& SettingsMap, const char* szPlugin, const char* szSearchPath);

  void DocumentManagerRequestHandler(ezDocumentManagerBase::Request& r);
  void DocumentManagerEventHandler(const ezDocumentManagerBase::Event& r);
  void DocumentEventHandler(const ezDocumentBase::Event& e);
  void ProjectRequestHandler(ezEditorProject::Request& r);
  void ProjectEventHandler(const ezEditorProject::Event& r);

  ezHybridArray<ezContainerWindow*, 4> s_ContainerWindows;
  ezMap<ezString, ezDocumentWindow*> s_DocumentWindows;

  void ReadPluginsToBeLoaded();

  ezString s_sApplicationName;
  ezString s_sUserName;

  ezSet<ezString> s_RestartRequiredReasons;

  ezPluginSet s_EditorPluginsAvailable;
  ezPluginSet s_EditorPluginsActive;
  ezPluginSet s_EditorPluginsToBeLoaded;

  ezSet<ezString> s_SettingsPluginNames;
  ezMap<ezString, ezSettings> s_EditorSettings;
  ezMap<ezString, ezSettings> s_ProjectSettings;
  ezMap<ezString, ezMap<ezString, ezSettings> > s_DocumentSettings;

  void StoreSettings(const ezMap<ezString, ezSettings>& settings, const char* szFolder);
  void SaveDocumentSettings(const ezDocumentBase* pDocument);

  void SaveRecentFiles();
  void LoadRecentFiles();

  ezRecentFilesList s_RecentProjects;
  ezRecentFilesList s_RecentDocuments;

  QApplication* s_pQtApplication;
  ezEditorEngineViewProcess* s_pEngineViewProcess;
};