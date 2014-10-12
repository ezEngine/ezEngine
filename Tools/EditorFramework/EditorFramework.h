#pragma once

#include <EditorFramework/Plugin.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Strings/String.h>

class QMainWindow;

struct EZ_EDITORFRAMEWORK_DLL ezPluginSet
{
  ezSet<ezString> m_Plugins;
};

class EZ_EDITORFRAMEWORK_DLL ezEditorFramework
{
public:

  static QMainWindow* GetMainWindow() { return s_pMainWindow; }
  static void SetMainWindow(QMainWindow* pWindow) { s_pMainWindow = pWindow; UpdateEditorWindowTitle(); }

  static void SetApplicationName(const char* szProfile) { s_sApplicationName = szProfile; UpdateEditorWindowTitle(); }
  static const char* GetApplicationName() { return s_sApplicationName.GetData(); }

  static void LoadPlugins();

  static const ezPluginSet& GetEditorPluginsAvailable();
  static const ezPluginSet& GetEditorPluginsActive() { return s_EditorPluginsActive; }
  static const ezPluginSet& GetEditorPluginsToBeLoaded() { return s_EditorPluginsToBeLoaded; }
  static void SetEditorPluginsToBeLoaded(const ezPluginSet& plugins);

  static void AddRestartRequiredReason(const char* szReason) { s_RestartRequiredReasons.Insert(szReason); }
  static const ezSet<ezString>& GetRestartRequiredReasons() { return s_RestartRequiredReasons; }

  static void SetEditorWindowTitle(const char* szTitle);

  static void SaveWindowLayout();
  static void RestoreWindowLayout();

private:
  static void UpdateEditorWindowTitle();

  static void ReadPluginsToBeLoaded();

  static ezString s_sApplicationName;
  static ezString s_sWindowTitle;
  static bool s_bContentModified;

  static ezSet<ezString> s_RestartRequiredReasons;

  static ezPluginSet s_EditorPluginsAvailable;
  static ezPluginSet s_EditorPluginsActive;
  static ezPluginSet s_EditorPluginsToBeLoaded;

  static QMainWindow* s_pMainWindow;
};