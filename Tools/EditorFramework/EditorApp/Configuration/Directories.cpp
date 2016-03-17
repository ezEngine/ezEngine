#include <PCH.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/OSFile.h>

ezString ezQtEditorApp::GetEditorDataFolder()
{
  ezStringBuilder sAppDir = ezOSFile::GetApplicationDirectory();
  sAppDir.AppendPath("../../../Data/Tools", ezUIServices::GetApplicationName());

  return sAppDir;
}

ezString ezQtEditorApp::GetProjectUserDataFolder()
{
  ezStringBuilder sFile = ezToolsProject::GetSingleton()->GetProjectDataFolder();
  sFile.AppendPath(GetApplicationUserName());
  sFile.Append(".usersettings");

  return sFile;
}

ezString ezQtEditorApp::GetDocumentDataFolder(const char* szDocument)
{
  ezStringBuilder sPath = szDocument;
  sPath.Append("_data");

  return sPath;
}
