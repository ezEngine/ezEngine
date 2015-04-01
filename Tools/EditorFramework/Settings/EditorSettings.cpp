#include <PCH.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <ToolsFoundation/Settings/Settings.h>
#include <Foundation/IO/JSONWriter.h>
#include <Foundation/IO/ExtendedJSONWriter.h>
#include <Foundation/IO/ExtendedJSONReader.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>

void ezEditorApp::RegisterPluginNameForSettings(const char* szPluginName)
{
  s_SettingsPluginNames.Insert(szPluginName);
}

ezSettings& ezEditorApp::GetEditorSettings(const char* szPlugin)
{
  return GetSettings(s_EditorSettings, szPlugin, "");
}

ezSettings& ezEditorApp::GetProjectSettings(const char* szPlugin)
{
  EZ_ASSERT_DEV(ezToolsProject::IsProjectOpen(), "No project is open");

  return GetSettings(s_ProjectSettings, szPlugin, ezEditorApp::GetDocumentDataFolder(ezToolsProject::GetInstance()->GetProjectPath()));
}

ezSettings& ezEditorApp::GetDocumentSettings(const ezDocumentBase* pDocument, const char* szPlugin)
{
  return GetDocumentSettings(pDocument->GetDocumentPath(), szPlugin);
}

ezSettings& ezEditorApp::GetDocumentSettings(const char* szDocument, const char* szPlugin)
{
  return GetSettings(s_DocumentSettings[szDocument], szPlugin, ezEditorApp::GetDocumentDataFolder(szDocument));
}

ezSettings& ezEditorApp::GetSettings(ezMap<ezString, ezSettings>& SettingsMap, const char* szPlugin, const char* szSearchPath)
{
  EZ_ASSERT_DEV(s_SettingsPluginNames.Contains(szPlugin), "The plugin name '%s' has not been registered with 'ezEditorApp::RegisterPluginNameForSettings'", szPlugin);

  bool bExisted = false;

  auto itSett = SettingsMap.FindOrAdd(szPlugin, &bExisted);

  ezSettings& settings = itSett.Value();

  if (!bExisted)
  {
    ezStringBuilder sPath = szSearchPath;

    sPath.AppendPath("Settings", szPlugin);
    sPath.ChangeFileExtension("settings");

    ezFileReader file;
    if (file.Open(sPath.GetData()).Succeeded())
    {
      settings.ReadFromJSON(file);
      file.Close();
    }

    ezStringBuilder sUserFile;
    sUserFile.Append(GetApplicationUserName().GetData(), ".usersettings");
    sPath.ChangeFileExtension(sUserFile.GetData());

    if (file.Open(sPath.GetData()).Succeeded())
    {
      settings.ReadFromJSON(file);
      file.Close();
    }
  }

  return settings;
}

void ezEditorApp::SaveRecentFiles()
{
  s_RecentProjects.Save("Settings/RecentProjects.txt");
  s_RecentDocuments.Save("Settings/RecentDocuments.txt");
}

void ezEditorApp::LoadRecentFiles()
{
  s_RecentProjects.Load("Settings/RecentProjects.txt");
  s_RecentDocuments.Load("Settings/RecentDocuments.txt");
}

void ezEditorApp::StoreSettings(const ezMap<ezString, ezSettings>& settings, const char* szFolder)
{
  for (auto it = settings.GetIterator(); it.IsValid(); ++it)
  {
    const ezSettings& settings = it.Value();

    ezStringBuilder sPath = szFolder;
    sPath.AppendPath("Settings", it.Key().GetData());
    sPath.ChangeFileExtension("settings");

    ezFileWriter file;
    if (file.Open(sPath.GetData()).Succeeded())
    {
      settings.WriteToJSON(file, true, false);
      file.Close();
    }

    ezStringBuilder sUserFile;
    sUserFile.Append(GetApplicationUserName().GetData(), ".usersettings");
    sPath.ChangeFileExtension(sUserFile.GetData());

    if (file.Open(sPath.GetData()).Succeeded())
    {
      settings.WriteToJSON(file, false, true);
      file.Close();
    }
  }
}

void ezEditorApp::SaveSettings()
{
  SaveRecentFiles();

  StoreSettings(s_EditorSettings, "");

  if (ezToolsProject::IsProjectOpen())
  {
    m_FileSystemConfig.Save();
    m_EnginePluginConfig.Save();

    StoreSettings(s_ProjectSettings, GetDocumentDataFolder(ezToolsProject::GetInstance()->GetProjectPath()));
  }
}

void ezEditorApp::SaveDocumentSettings(const ezDocumentBase* pDocument)
{
  auto it = s_DocumentSettings.Find(pDocument->GetDocumentPath());

  if (!it.IsValid())
    return;

  StoreSettings(it.Value(), GetDocumentDataFolder(it.Key()));
}



