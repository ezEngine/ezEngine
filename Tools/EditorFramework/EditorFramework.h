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

class EZ_EDITORFRAMEWORK_DLL ezEditorFramework
{
public:
  static QMainWindow* GetMainWindow();

  static const ezString& GetApplicationName() { return s_sApplicationName; }
  static const ezString& GetApplicationUserName() { return s_sUserName; }

  static const ezPluginSet& GetEditorPluginsAvailable();
  static const ezPluginSet& GetEditorPluginsActive() { return s_EditorPluginsActive; }
  static const ezPluginSet& GetEditorPluginsToBeLoaded() { return s_EditorPluginsToBeLoaded; }
  static void SetEditorPluginsToBeLoaded(const ezPluginSet& plugins);

  static void AddRestartRequiredReason(const char* szReason) { s_RestartRequiredReasons.Insert(szReason); }
  static const ezSet<ezString>& GetRestartRequiredReasons() { return s_RestartRequiredReasons; }

  static void ShowPluginConfigDialog();

  static void RegisterPluginNameForSettings(const char* szPluginName);
  static const ezSet<ezString>& GetRegisteredPluginNamesForSettings() { return s_SettingsPluginNames; }
  static ezSettings& GetEditorSettings(const char* szPluginName = "-Main-");
  static ezSettings& GetProjectSettings(const char* szPluginName = "-Main-");
  static ezSettings& GetDocumentSettings(const ezDocumentBase* pDocument, const char* szPluginName = "-Main-");
  static ezSettings& GetDocumentSettings(const char* szDocument, const char* szPlugin = "-Main-");
  static void SaveSettings();

  static void StartupEditor(const char* szAppName, const char* szUserName, int argc, char** argv);
  static void ShutdownEditor();
  static ezInt32 RunEditor();

  static void LoadPlugins();
  static void UnloadPlugins();

  static ezDocumentWindow* GetDocumentWindow(const char* szUniqueName);
  static void AddDocumentWindow(ezDocumentWindow* pWindow);

  static ezRecentFilesList& GetRecentProjectsList()   { return s_RecentProjects;  }
  static ezRecentFilesList& GetRecentDocumentsList()  { return s_RecentDocuments; }

  static ezString GetDocumentDataFolder(const char* szDocument);

private:
  static ezSettings& GetSettings(ezMap<ezString, ezSettings>& SettingsMap, const char* szPlugin, const char* szSearchPath);

  static void DocumentManagerRequestHandler(ezDocumentManagerBase::Request& r);
  static void DocumentManagerEventHandler(const ezDocumentManagerBase::Event& r);
  static void DocumentEventHandler(const ezDocumentBase::Event& e);
  static void ProjectRequestHandler(ezEditorProject::Request& r);
  static void ProjectEventHandler(const ezEditorProject::Event& r);

  static ezHybridArray<ezContainerWindow*, 4> s_ContainerWindows;
  static ezMap<ezString, ezDocumentWindow*> s_DocumentWindows;

  static void ReadPluginsToBeLoaded();

  static ezString s_sApplicationName;
  static ezString s_sUserName;

  static ezSet<ezString> s_RestartRequiredReasons;

  static ezPluginSet s_EditorPluginsAvailable;
  static ezPluginSet s_EditorPluginsActive;
  static ezPluginSet s_EditorPluginsToBeLoaded;

  static ezSet<ezString> s_SettingsPluginNames;
  static ezMap<ezString, ezSettings> s_EditorSettings;
  static ezMap<ezString, ezSettings> s_ProjectSettings;
  static ezMap<ezString, ezMap<ezString, ezSettings> > s_DocumentSettings;

  static void StoreSettings(const ezMap<ezString, ezSettings>& settings, const char* szFolder);
  static void SaveDocumentSettings(const ezDocumentBase* pDocument);

  static void SaveRecentFiles();
  static void LoadRecentFiles();

  static ezRecentFilesList s_RecentProjects;
  static ezRecentFilesList s_RecentDocuments;

  static QApplication* s_pQtApplication;
};