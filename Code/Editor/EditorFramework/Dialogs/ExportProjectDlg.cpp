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
  // TODO:
  // filter out unused runtime/game plugins
  // user selected output path
  // transform assets
  // progress bar
  // start.bat
  // better logic to filter out input assets
  // include / exclude config file ??
  // output log to UI
  // asset profile
  // copy inputs into resource: RML files
  // code cleanup

  ezProgressRange mainProgress("Export Project", 6, true);
  mainProgress.SetStepWeighting(0, 0.05f); // Preparing output folder
  mainProgress.SetStepWeighting(1, 0.10f); // Scanning data directories
  mainProgress.SetStepWeighting(2, 0.10f); // Filtering files
  mainProgress.SetStepWeighting(3, 0.05f); // Gathering binaries
  mainProgress.SetStepWeighting(4, 0.01f); // Writing data directory config
  //mainProgress.SetStepWeighting(5, 0.0f); // Copying files

  const char* szDstFolder = "C:/GitHub/ExportTest";

  {
    mainProgress.BeginNextStep("Preparing output folder");

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

  struct DataDirInfo
  {
    ezString m_sTargetDirPath;
    ezString m_sTargetDirRootName;
    ezSet<ezString> m_Files;
  };

  ezMap<ezString, DataDirInfo> fileList;
  ezSet<ezString> assetInputs;

  auto logToFile = [&](const char* msg, const char* msg2)
  {
    logFile.Write(msg, ezStringUtils::GetStringElementCount(msg)).AssertSuccess();
    logFile.Write(msg2, ezStringUtils::GetStringElementCount(msg2)).AssertSuccess();
    logFile.Write("\n", 1).AssertSuccess();
  };

  ezUInt32 uiTotalFiles = 0;

  {
    mainProgress.BeginNextStep("Scanning data directories");

    ezProgressRange progress("Scanning data directories", dataDirs.m_DataDirs.GetCount(), true);

    ezUInt32 uiDataDirNumber = 1;

    for (const auto& dataDir : dataDirs.m_DataDirs)
    {
      progress.BeginNextStep(dataDir.m_sDataDirSpecialPath);

      if (ezFileSystem::ResolveSpecialDirectory(dataDir.m_sDataDirSpecialPath, sPath).Failed())
      {
        ezQtUiServices::GetSingleton()->MessageBoxWarning(ezFmt("Failed to get special directory '{0}'", dataDir.m_sDataDirSpecialPath));
        return;
      }

      sPath.Trim("/\\");
      const ezUInt32 uiStrip = sPath.GetElementCount() + 1;

      DataDirInfo& ddInfo = fileList[sPath];
      ezSet<ezString>& ddFileList = ddInfo.m_Files;

      if (!dataDir.m_sRootName.IsEmpty())
      {
        sTemp.Set("Data/", dataDir.m_sRootName);

        ddInfo.m_sTargetDirRootName = dataDir.m_sRootName;
        ddInfo.m_sTargetDirPath = sTemp;
      }
      else
      {
        sTemp.Format("Data/Extra{}", uiDataDirNumber);

        ddInfo.m_sTargetDirPath = sTemp;
      }

      ezFileSystemIterator it;
      for (it.StartSearch(sPath, ezFileSystemIteratorFlags::ReportFilesAndFoldersRecursive); it.IsValid();)
      {
        if (ezProgress::GetGlobalProgressbar()->WasCanceled())
          return;

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
            it.GetStats().m_sName == "DataDirectories.ddl")
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
        ++uiTotalFiles;
        it.Next();
      }
    }
  }


  // filter out asset inputs
  // this is problematic
  {
    mainProgress.BeginNextStep("Filtering files");

    ezProgressRange range("Filtering files", uiTotalFiles, true);

    for (auto itDir = fileList.GetIterator(); itDir.IsValid(); ++itDir)
    {
      if (itDir.Value().m_sTargetDirRootName.IsEqual_NoCase("base"))
      {
        range.BeginNextStep("Skipping Base Directory", itDir.Value().m_Files.GetCount());
        continue;
      }

      for (auto itFile = itDir.Value().m_Files.GetIterator(); itFile.IsValid();)
      {
        range.BeginNextStep(itFile.Key());

        if (ezProgress::GetGlobalProgressbar()->WasCanceled())
          return;

        if (itFile.Key().EndsWith_NoCase(".ezShader") ||
            itFile.Key().EndsWith_NoCase(".bank") ||
            itFile.Key().EndsWith_NoCase(".rml"))
        {
          // TODO: special case
          ++itFile;
          continue;
        }

        if (assetInputs.Contains(*itFile))
        {
          itFile = itDir.Value().m_Files.Remove(itFile);
          --uiTotalFiles;
        }
        else
        {
          ++itFile;
        }
      }
    }
  }

  {
    mainProgress.BeginNextStep("Gathering binaries");

    //ezProgressRange range("Gathering binaries", true);

    sPath = ezOSFile::GetApplicationDirectory();
    sPath.MakeCleanPath();
    sPath.Trim("/\\");
    const ezUInt32 uiStrip = sPath.GetElementCount() + 1;

    DataDirInfo& ddInfo = fileList[sPath];
    ezSet<ezString>& ddFileList = ddInfo.m_Files;

    ddInfo.m_sTargetDirPath = "Bin";
    ddInfo.m_sTargetDirRootName = "-"; // don't add to data dir config

    ddFileList.Insert("Player.exe");
    ++uiTotalFiles;

    ezFileSystemIterator it;
    for (it.StartSearch(sPath, ezFileSystemIteratorFlags::ReportFilesAndFoldersRecursive); it.IsValid();)
    {
      if (ezProgress::GetGlobalProgressbar()->WasCanceled())
        return;

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
      ++uiTotalFiles;
      it.Next();
    }
  }

  // write data dir config file
  {
    mainProgress.BeginNextStep("Writing data directory config");

    ezApplicationFileSystemConfig cfg;

    for (auto itDir = fileList.GetIterator(); itDir.IsValid(); ++itDir)
    {
      const auto& info = itDir.Value();

      if (info.m_sTargetDirRootName == "-")
        continue;

      sPath.Set(">sdk/", info.m_sTargetDirPath);

      auto& ddc = cfg.m_DataDirs.ExpandAndGetRef();
      ddc.m_sDataDirSpecialPath = sPath;
      ddc.m_sRootName = info.m_sTargetDirRootName;
    }

    sPath.Set(szDstFolder, "/Data/project/DataDirectories.ddl");
    if (cfg.Save(sPath).Failed())
    {
      if (cfg.Save(sPath).Failed())
        ezQtUiServices::GetSingleton()->MessageBoxWarning(ezFmt("Failed to write data directory config file '{0}'", sPath));
      return;
    }
  }

  {
    mainProgress.BeginNextStep("Copying files");

    ezProgressRange range("Copying files", uiTotalFiles, true);

    logToFile("Output folder: ", szDstFolder);

    for (auto itDir = fileList.GetIterator(); itDir.IsValid(); ++itDir)
    {
      logToFile("Source sub-folder: ", itDir.Key());
      logToFile("Destination sub-folder: ", itDir.Value().m_sTargetDirPath);

      for (auto itFile = itDir.Value().m_Files.GetIterator(); itFile.IsValid(); ++itFile)
      {
        if (ezProgress::GetGlobalProgressbar()->WasCanceled())
          return;

        range.BeginNextStep(itFile.Key());

        sPath.Set(itDir.Key(), "/", itFile.Key());
        sTemp.Set(szDstFolder, "/", itDir.Value().m_sTargetDirPath, "/", itFile.Key());

        if (ezOSFile::CopyFile(sPath, sTemp).Succeeded())
        {
          logToFile(" -> ", itFile.Key());
        }
        else
        {
          logToFile(" Copy failed:", itFile.Key());
        }
      }
    }
  }

  accept();
}
