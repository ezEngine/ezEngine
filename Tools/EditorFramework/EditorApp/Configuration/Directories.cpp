#include <PCH.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/OSFile.h>

ezString ezQtEditorApp::GetEditorDataFolder()
{
  ezStringBuilder sAppDir = ezOSFile::GetApplicationDirectory();
  sAppDir.AppendPath("../../../Data/Tools", ezUIServices::GetApplicationName());

  return sAppDir;
}

ezString ezQtEditorApp::GetPrecompiledToolsFolder()
{
  ezStringBuilder sPath = ezOSFile::GetApplicationDirectory();
  sPath.AppendPath("../../../Data/Tools/Precompiled");
  return sPath;

}

ezString ezQtEditorApp::GetProjectUserDataFolder()
{
  ezStringBuilder sFile = ezToolsProject::GetSingleton()->GetProjectDataFolder();
  sFile.AppendPath(GetApplicationUserName());
  sFile.Append(".user");

  return sFile;
}

ezString ezQtEditorApp::GetDocumentDataFolder(const char* szDocument)
{
  ezStringBuilder sPath = szDocument;
  sPath.Append("_data");

  return sPath;
}


ezString ezQtEditorApp::GetEditorPreferencesFolder(bool bUserData)
{
  ezStringBuilder path = GetEditorDataFolder();
  path.AppendPath("Preferences");

  if (bUserData)
  {
    /// \todo should maybe be stored in %appdata% instead ?
    path.AppendPath(GetApplicationUserName());
  }

  path.MakeCleanPath();
  return path;
}

ezString ezQtEditorApp::GetProjectPreferencesFolder(bool bUserData)
{
  ezStringBuilder path = ezToolsProject::GetSingleton()->GetProjectDataFolder();
  path.AppendPath("Preferences");

  if (bUserData)
  {
    path.AppendPath(GetApplicationUserName());
    path.Append(".user");
  }

  path.MakeCleanPath();
  return path;
}

ezString ezQtEditorApp::GetDocumentPreferencesFolder(const ezDocument* pDocument, bool bUserData)
{
  ezStringBuilder path = GetDocumentDataFolder(pDocument->GetDocumentPath());
  path.AppendPath("Preferences");

  if (bUserData)
  {
    path.AppendPath(GetApplicationUserName());
    path.Append(".user");
  }

  path.MakeCleanPath();
  return path;
}

