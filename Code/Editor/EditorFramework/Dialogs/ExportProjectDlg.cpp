#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Dialogs/ExportProjectDlg.moc.h>
#include <EditorFramework/Preferences/Preferences.h>
#include <EditorFramework/Preferences/ProjectPreferences.h>
#include <Foundation/IO/OSFile.h>
#include <QFileDialog>

bool ezQtExportProjectDlg::s_bTransformAll = true;

ezQtExportProjectDlg::ezQtExportProjectDlg(QWidget* parent)
  : QDialog(parent)
{
  setupUi(this);

  // ezProjectPreferencesUser* pPref = ezPreferences::QueryPreferences<ezProjectPreferencesUser>();
}

void ezQtExportProjectDlg::showEvent(QShowEvent* e)
{
  QDialog::showEvent(e);

  TransformAll->setChecked(s_bTransformAll);
}

void ezQtExportProjectDlg::on_ExportProjectButton_clicked()
{
  const char* szDstFolder = "C:/GitHub/ExportTest";

  if (ezOSFile::DeleteFolder(szDstFolder).Failed())
  {
    ezQtUiServices::GetSingleton()->MessageBoxWarning(ezFmt("Failed to remove destination folder:\n'{}'", szDstFolder));
    return;
  }

  if (ezOSFile::CreateDirectoryStructure(szDstFolder).Failed())
  {
    ezQtUiServices::GetSingleton()->MessageBoxWarning(ezFmt("Failed to create destination folder:\n'{}'", szDstFolder));
    return;
  }

  ezStringBuilder sTemp;

  ezOSFile logFile;

  sTemp.Set(szDstFolder, "/ExportLog.txt");
  if (logFile.Open(sTemp, ezFileOpenMode::Write).Failed())
  {
    ezQtUiServices::GetSingleton()->MessageBoxWarning(ezFmt("Failed to create log file '{0}'", sTemp));
    return;
  }

  const auto dataDirs = ezQtEditorApp::GetSingleton()->GetFileSystemConfig();

  ezStringBuilder sPath;

  ezDeque<ezString> fileList;

  auto logToFile = [&](const char* msg)
  {
    logFile.Write(msg, ezStringUtils::GetStringElementCount(msg)).AssertSuccess();
    logFile.Write("\n", 1).AssertSuccess();
  };

  for (const auto& dataDir : dataDirs.m_DataDirs)
  {
    if (ezFileSystem::ResolveSpecialDirectory(dataDir.m_sDataDirSpecialPath, sPath).Failed())
    {
      ezQtUiServices::GetSingleton()->MessageBoxWarning(ezFmt("Failed to get special directory '{0}'", dataDir.m_sDataDirSpecialPath));
      return;
    }

    logToFile(sPath);

    ezFileSystemIterator it;
    for (it.StartSearch(sPath, ezFileSystemIteratorFlags::ReportFilesRecursive); it.IsValid();)
    {
      if (it.GetCurrentPath().EndsWith("/AssetCache/Thumbnails") ||
          it.GetCurrentPath().EndsWith("/Base/Editor"))
      {
        it.SkipFolder();
        continue;
      }

      it.GetStats().GetFullPath(sPath);

      // TODO: filter out files that are not needed (asset source files and such)
      fileList.PushBack(sPath);
      logToFile(sPath);

      it.Next();
    }
  }
}
