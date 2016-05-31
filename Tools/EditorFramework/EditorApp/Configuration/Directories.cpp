#include <PCH.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/OSFile.h>
#include <QStandardPaths>
#include <EditorFramework/Preferences/Preferences.h>
#include <EditorFramework/Preferences/EditorPreferences.h>

ezString ezQtEditorApp::GetEditorDataFolder()
{
  ezStringBuilder sAppDir = ezOSFile::GetApplicationDirectory();
  sAppDir.AppendPath("../../../Data/Tools", ezUIServices::GetApplicationName());
  sAppDir.MakeCleanPath();

  return sAppDir;
}

ezString ezQtEditorApp::GetPrecompiledToolsFolder()
{
  ezStringBuilder sPath = ezOSFile::GetApplicationDirectory();
  
  ezEditorPreferencesUser* pPref = ezPreferences::QueryPreferences<ezEditorPreferencesUser>();

  if (pPref->m_bUsePrecompiledTools)
  {
    sPath.AppendPath("../../../Data/Tools/Precompiled");
  }

  sPath.MakeCleanPath();

  return sPath;

}

ezString ezQtEditorApp::GetDocumentDataFolder(const char* szDocument)
{
  ezStringBuilder sPath = szDocument;
  sPath.Append("_data");
  sPath.MakeCleanPath();

  return sPath;
}


ezString ezQtEditorApp::GetEditorPreferencesFolder(bool bUserData)
{
  ezStringBuilder path = GetEditorDataFolder();
  path.AppendPath("Preferences");

  if (bUserData)
  {
    const QString userData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);

    path = userData.toUtf8().data();
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
    const QString userData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);

    ezStringBuilder ProjectName = ezToolsProject::GetSingleton()->GetProjectDirectory();
    ProjectName = ProjectName.GetFileName();

    path = userData.toUtf8().data();
    path.AppendPath("Projects", ProjectName);
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
    const ezStringBuilder sGuid = ezConversionUtils::ToString(pDocument->GetGuid());

    path = GetProjectPreferencesFolder(true);
    path.AppendPath(sGuid);
  }

  path.MakeCleanPath();
  return path;
}

