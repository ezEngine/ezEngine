#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Dialogs/ExportProjectDlg.moc.h>
#include <EditorFramework/Preferences/Preferences.h>
#include <EditorFramework/Preferences/ProjectPreferences.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Strings/String.h>
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

  ezMap<ezString, ezSet<ezString>> fileList;
  ezSet<ezString> assetInputs;

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

    sPath.Trim("/\\");
    const ezUInt32 uiStrip = sPath.GetElementCount() + 1;

    ezSet<ezString>& ddFileList = fileList[sPath];


    ezFileSystemIterator it;
    for (it.StartSearch(sPath, ezFileSystemIteratorFlags::ReportFilesAndFoldersRecursive); it.IsValid();)
    {
      if (it.GetStats().m_bIsDirectory)
      {
        if (it.GetCurrentPath().EndsWith("/AssetCache/Thumbnails") ||
            it.GetCurrentPath().EndsWith("/AssetCache/Temp") ||
            it.GetCurrentPath().EndsWith("/Base/Editor"))
        {
          it.SkipFolder();
        }
        else
        {
          it.Next();
        }

        continue;
      }

      if (it.GetStats().m_sName == "AssetCurator.ezCache" ||
          it.GetStats().m_sName == "DataDir.ezManifest" ||
          it.GetStats().m_sName == "ezProject")
      {
        it.Next();
        continue;
      }

      it.GetStats().GetFullPath(sPath);

      auto asset = ezAssetCurator::GetSingleton()->FindSubAsset(sPath);
      if (asset.isValid())
      {
        assetInputs.Union(asset->m_pAssetInfo->m_Info->m_AssetTransformDependencies);

        // ignore all asset files
        it.Next();
        continue;
      }

      sPath.Shrink(uiStrip, 0);

      ddFileList.Insert(sPath);
      it.Next();
    }
  }

  for (auto itDir = fileList.GetIterator(); itDir.IsValid(); ++itDir)
  {
    for (auto itFile = itDir.Value().GetIterator(); itFile.IsValid();)
    {
      if (assetInputs.Contains(*itFile))
      {
        itFile = itDir.Value().Remove(itFile);
      }
      else
      {
        ++itFile;
      }
    }
  }

  {
    sPath = ezOSFile::GetApplicationDirectory();
    sPath.Trim("/\\");
    const ezUInt32 uiStrip = sPath.GetElementCount() + 1;

    ezSet<ezString>& ddFileList = fileList[sPath];

    ezFileSystemIterator it;
    for (it.StartSearch(sPath, ezFileSystemIteratorFlags::ReportFilesAndFoldersRecursive); it.IsValid();)
    {
      const ezStringBuilder fileName = it.GetStats().m_sName;

      if (it.GetStats().m_bIsDirectory)
      {
        if (fileName == "iconengines" ||
            fileName == "imageformats" ||
            fileName == "platforms")
        {
          it.SkipFolder();
          continue;
        }

        it.Next();
        continue;
      }

      if (fileName.HasExtension("exe") ||
          fileName.HasExtension("pdb") ||
          fileName.HasExtension("loaded"))
      {
        it.Next();
        continue;
      }

      if (fileName.HasExtension("dll"))
      {
        if (fileName.StartsWith("ezEditorPlugin") ||
            fileName.StartsWith("ezEnginePlugin") ||
            fileName.StartsWith("ezSharedPlugin") ||
            fileName.StartsWith("Qt5") ||
            fileName.StartsWith("Qt6"))
        {
          it.Next();
          continue;
        }
      }

      it.GetStats().GetFullPath(sPath);
      sPath.Shrink(uiStrip, 0);
      ddFileList.Insert(sPath);
      it.Next();
    }
  }

  for (auto itDir = fileList.GetIterator(); itDir.IsValid(); ++itDir)
  {
    for (auto itFile = itDir.Value().GetIterator(); itFile.IsValid(); ++itFile)
    {
      sPath.Set(itDir.Key(), "/", itFile.Key());
      logToFile(sPath);
    }
  }
}
