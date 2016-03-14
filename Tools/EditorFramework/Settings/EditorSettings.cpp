#include <PCH.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <ToolsFoundation/Settings/Settings.h>
#include <Foundation/IO/JSONWriter.h>
#include <Foundation/IO/ExtendedJSONWriter.h>
#include <Foundation/IO/ExtendedJSONReader.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>

void ezQtEditorApp::RegisterPluginNameForSettings(const char* szPluginName)
{
  s_SettingsPluginNames.Insert(szPluginName);
}

ezSettings& ezQtEditorApp::GetEditorSettings(const char* szPlugin)
{
  return GetSettings(s_EditorSettings, szPlugin, "");
}

ezSettings& ezQtEditorApp::GetProjectSettings(const char* szPlugin)
{
  EZ_ASSERT_DEV(ezToolsProject::IsProjectOpen(), "No project is open");

  return GetSettings(s_ProjectSettings, szPlugin, ezQtEditorApp::GetDocumentDataFolder(ezToolsProject::GetInstance()->GetProjectFile()));
}

ezSettings& ezQtEditorApp::GetDocumentSettings(const ezDocument* pDocument, const char* szPlugin)
{
  return GetDocumentSettings(pDocument->GetDocumentPath(), szPlugin);
}

ezSettings& ezQtEditorApp::GetDocumentSettings(const char* szDocument, const char* szPlugin)
{
  return GetSettings(s_DocumentSettings[szDocument], szPlugin, ezQtEditorApp::GetDocumentDataFolder(szDocument));
}

ezSettings& ezQtEditorApp::GetSettings(ezMap<ezString, ezSettings>& SettingsMap, const char* szPlugin, const char* szSearchPath)
{
  EZ_ASSERT_DEV(s_SettingsPluginNames.Contains(szPlugin), "The plugin name '%s' has not been registered with 'ezQtEditorApp::RegisterPluginNameForSettings'", szPlugin);

  bool bExisted = false;

  auto itSett = SettingsMap.FindOrAdd(szPlugin, &bExisted);

  ezSettings& settings = itSett.Value();

  if (!bExisted)
  {
    ezStringBuilder sPath = szSearchPath;

    sPath.AppendPath("Settings", szPlugin);
    sPath.ChangeFileExtension("settings");

    ezFileReader file;
    if (file.Open(sPath).Succeeded())
    {
      settings.ReadFromJSON(file);
      file.Close();
    }

    ezStringBuilder sUserFile;
    sUserFile.Append(GetApplicationUserName(), ".usersettings");
    sPath.ChangeFileExtension(sUserFile);

    if (file.Open(sPath).Succeeded())
    {
      settings.ReadFromJSON(file);
      file.Close();
    }
  }

  return settings;
}

void ezQtEditorApp::SaveRecentFiles()
{
  s_RecentProjects.Save("Settings/RecentProjects.txt");
  s_RecentDocuments.Save("Settings/RecentDocuments.txt");
}

void ezQtEditorApp::LoadRecentFiles()
{
  s_RecentProjects.Load("Settings/RecentProjects.txt");
  s_RecentDocuments.Load("Settings/RecentDocuments.txt");
}

void ezQtEditorApp::StoreSettings(const ezMap<ezString, ezSettings>& settings, const char* szFolder)
{
  for (auto it = settings.GetIterator(); it.IsValid(); ++it)
  {
    const ezSettings& settings = it.Value();

    ezStringBuilder sPath = szFolder;
    sPath.AppendPath("Settings", it.Key());
    sPath.ChangeFileExtension("settings");

    ezFileWriter file;

    if (settings.IsEmpty(true, false))
    {
      ezFileSystem::DeleteFile(sPath);
    }
    else
    {
      if (file.Open(sPath).Succeeded())
      {
        settings.WriteToJSON(file, true, false);
        file.Close();
      }
    }

    ezStringBuilder sUserFile;
    sUserFile.Append(GetApplicationUserName(), ".usersettings");
    sPath.ChangeFileExtension(sUserFile);

    if (settings.IsEmpty(false, true))
    {
      ezFileSystem::DeleteFile(sPath);
    }
    else
    {
      if (file.Open(sPath).Succeeded())
      {
        settings.WriteToJSON(file, false, true);
        file.Close();
      }
    }
  }
}

void ezQtEditorApp::SaveOpenDocumentsList()
{
  const ezDynamicArray<ezQtDocumentWindow*>& windows = ezQtDocumentWindow::GetAllDocumentWindows();

  if (windows.IsEmpty())
    return;

  ezRecentFilesList allDocs(10);

  ezMap<ezUInt32, ezQtDocumentWindow*> Sorted;

  for (ezUInt32 w = 0; w < windows.GetCount(); ++w)
  {
    if (windows[w]->GetDocument())
    {
      Sorted[windows[w]->GetWindowIndex()] = windows[w];
    }
  }

  for (auto it = Sorted.GetLastIterator(); it.IsValid(); --it)
  {
    allDocs.Insert(it.Value()->GetDocument()->GetDocumentPath());
  }

  ezStringBuilder sFile = GetProjectUserDataFolder();
  sFile.AppendPath("LastDocuments.txt");

  allDocs.Save(sFile);
}

ezRecentFilesList ezQtEditorApp::LoadOpenDocumentsList()
{
  ezRecentFilesList allDocs(15);

  ezStringBuilder sFile = GetProjectUserDataFolder();
  sFile.AppendPath("LastDocuments.txt");

  allDocs.Load(sFile);

  return allDocs;
}

void ezQtEditorApp::SaveSettings()
{
  SaveRecentFiles();

  StoreSettings(s_EditorSettings, "");

  if (ezToolsProject::IsProjectOpen())
  {
    SaveOpenDocumentsList();

    m_FileSystemConfig.Save();
    m_EnginePluginConfig.Save();

    StoreSettings(s_ProjectSettings, GetDocumentDataFolder(ezToolsProject::GetInstance()->GetProjectFile()));
  }
}

void ezQtEditorApp::SaveDocumentSettings(const ezDocument* pDocument)
{
  auto it = s_DocumentSettings.Find(pDocument->GetDocumentPath());

  if (!it.IsValid())
    return;

  StoreSettings(it.Value(), GetDocumentDataFolder(it.Key()));
}



