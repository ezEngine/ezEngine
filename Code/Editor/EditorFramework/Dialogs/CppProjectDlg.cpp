#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Dialogs/CppProjectDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OSFile.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

// TODO: make this work with binary release versions
// TODO: pass in output bin/lib dir
// TODO: allow relocating Output dir (?)

ezQtCppProjectDlg::ezQtCppProjectDlg(QWidget* parent)
  : QDialog(parent)
{
  setupUi(this);

  Generator->addItem("Visual Studio 2019");
  Generator->addItem("Visual Studio 2022");
  Generator->setCurrentIndex(1);

  UpdateUI();
}

ezResult ezQtCppProjectDlg::GenerateSolution()
{
  if (ezSystemInformation::IsDebuggerAttached())
  {
    ezQtUiServices::GetSingleton()->MessageBoxWarning("When a debugger is attached, CMake can fail with the error that no C/C++ compiler can be found.");
  }

  ezProgressRange progress("Generating Solution", 4, false);
  progress.SetStepWeighting(0, 0.05f);
  progress.SetStepWeighting(1, 0.1f);
  progress.SetStepWeighting(2, 0.1f);
  progress.SetStepWeighting(3, 1.0f);

  QApplication::setOverrideCursor(Qt::WaitCursor);

  OutputLog->clear();
  ezStringBuilder output;
  EZ_SCOPE_EXIT(OutputLog->setText(output.GetData()));
  EZ_SCOPE_EXIT(QApplication::restoreOverrideCursor());
  EZ_SCOPE_EXIT(UpdateUI());

  const ezString sProjectName = ezToolsProject::GetSingleton()->GetProjectName();
  ezStringBuilder sProjectNameUpper = sProjectName;
  sProjectNameUpper.ToUpper();

  const ezStringBuilder sTargetDir = ezToolsProject::GetSingleton()->GetProjectDirectory();

  ezStringBuilder sSourceDir = ezApplicationServices::GetSingleton()->GetApplicationDataFolder();
  sSourceDir.AppendPath("CppProject");

  ezDynamicArray<ezFileStats> items;
  ezOSFile::GatherAllItemsInFolder(items, sSourceDir, ezFileSystemIteratorFlags::ReportFilesRecursive);

  struct FileToCopy
  {
    ezString m_sSource;
    ezString m_sDestination;
  };

  ezHybridArray<FileToCopy, 32> filesCopied;

  // gather files
  {
    progress.BeginNextStep("Gathering source files");

    for (const auto& item : items)
    {
      ezStringBuilder srcPath, dstPath;
      item.GetFullPath(srcPath);

      dstPath = srcPath;
      dstPath.MakeRelativeTo(sSourceDir).IgnoreResult();

      dstPath.ReplaceAll("CppProject", sProjectName);
      dstPath.Prepend(sTargetDir, "/");
      dstPath.MakeCleanPath();

      // don't copy files over that already exist (and may have edits)
      if (ezOSFile::ExistsFile(dstPath))
      {
        // if any file already exists, don't copy non-existing (user might have deleted unwanted sample files)
        filesCopied.Clear();
        break;
      }

      auto& ftc = filesCopied.ExpandAndGetRef();
      ftc.m_sSource = srcPath;
      ftc.m_sDestination = dstPath;
    }
  }

  // Copy files
  {
    progress.BeginNextStep("Copying sources");

    for (const auto& ftc : filesCopied)
    {
      if (ezOSFile::CopyFile(ftc.m_sSource, ftc.m_sDestination).Failed())
      {
        output.AppendFormat("Failed to copy a file.\nSource: '{}'\nDestination: '{}'\n", ftc.m_sSource, ftc.m_sDestination);
        return EZ_FAILURE;
      }
    }
  }

  // Modify sources
  {
    progress.BeginNextStep("Modifying sources");

    for (const auto& filePath : filesCopied)
    {
      ezStringBuilder content;

      {
        ezFileReader file;
        if (file.Open(filePath.m_sDestination).Failed())
        {
          output.AppendFormat("Failed to open C++ project file for reading.\nSource: '{}'\n", filePath.m_sDestination);
          return EZ_FAILURE;
        }

        content.ReadAll(file);
      }

      content.ReplaceAll("CppProject", sProjectName);
      content.ReplaceAll("CPPPROJECT", sProjectNameUpper);

      {
        ezFileWriter file;
        if (file.Open(filePath.m_sDestination).Failed())
        {
          output.AppendFormat("Failed to open C++ project file for writing.\nSource: '{}'\n", filePath.m_sDestination);
          return EZ_FAILURE;
        }

        file.WriteBytes(content.GetData(), content.GetElementCount()).IgnoreResult();
      }
    }
  }
  // run CMake
  {
    progress.BeginNextStep("Running CMake");

    const ezString sSdkDir = ezFileSystem::GetSdkRootDirectory();
    const ezString sBuildDir = GetBuildDir();
    const ezString sSolutionFile = GetSolutionFile();

    if (ezOSFile::ExistsDirectory(sBuildDir) && ezOSFile::DeleteFolder(sBuildDir).Failed())
    {
      output.AppendFormat("Couldn't delete build output directory:\n{}\n\nProject is probably already open in Visual Studio.\n", sBuildDir);
    }

    ezStringBuilder tmp;

    QStringList args;
    args << "-S";
    args << GetTargetDir().GetData();

    tmp.Format("-DEZ_SDK_DIR:PATH={}", sSdkDir);
    args << tmp.GetData();

    args << "-G";
    args << GetGeneratorCMake().GetData();

    args << "-B";
    args << sBuildDir.GetData();

    args << "-A";
    args << "x64";

    ezLogSystemToBuffer log;

    ezStatus res = ezQtEditorApp::GetSingleton()->ExecuteTool("cmake/bin/cmake.exe", args, 120, &log, ezLogMsgType::InfoMsg);

    if (res.Failed())
    {
      output.AppendFormat("Solution generation failed:\n\n");
      output.AppendFormat("{}\n", log.m_sBuffer);
      return EZ_FAILURE;
    }

    output.AppendFormat("Generated solution successfully.\n\n");
    output.AppendFormat("{}\n", log.m_sBuffer);
  }

  return EZ_SUCCESS;
}

void ezQtCppProjectDlg::on_Result_rejected()
{
  reject();
}

void ezQtCppProjectDlg::on_OpenPluginLocation_clicked()
{
  ezQtUiServices::OpenInExplorer(PluginLocation->text().toUtf8().data(), false);
}

void ezQtCppProjectDlg::on_OpenBuildFolder_clicked()
{
  ezQtUiServices::OpenInExplorer(BuildFolder->text().toUtf8().data(), false);
}

void ezQtCppProjectDlg::on_Generator_currentIndexChanged(int)
{
  UpdateUI();
}

void ezQtCppProjectDlg::on_OpenSolution_clicked()
{
  if (!ezQtUiServices::OpenFileInDefaultProgram(GetSolutionFile()))
  {
    ezQtUiServices::GetSingleton()->MessageBoxWarning("Opening the solution failed.");
  }
}

void ezQtCppProjectDlg::on_GenerateSolution_clicked()
{
  if (ezOSFile::ExistsFile(GetSolutionFile()))
  {
    if (ezQtUiServices::MessageBoxQuestion("The solution already exists, do you want to recreate it?", QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::No) != QMessageBox::StandardButton::Yes)
      return;
  }

  if (GenerateSolution().Failed())
  {
    ezQtUiServices::GetSingleton()->MessageBoxWarning("Generating the solution failed. Check the log output for details.");
  }
  else
  {
    ezStringBuilder txt;
    txt.Format("The solution was generated successfully.\n\nMake sure to compile it with the same build type with which you use the editor.\nYou are currently running a '{}' build.\n\nDo you want to open the solution now?", BUILDSYSTEM_BUILDTYPE);

    if (ezQtUiServices::GetSingleton()->MessageBoxQuestion(txt, QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes)
    {
      on_OpenSolution_clicked();
    }

    ezStringBuilder sProjectName = ezToolsProject::GetSingleton()->GetProjectName();
    ezStringBuilder sPluginName(sProjectName, "Plugin");

    ezPluginBundleSet& bundles = ezQtEditorApp::GetSingleton()->GetPluginBundles();

    bundles.m_Plugins.Remove(sPluginName);
    ezPluginBundle& plugin = bundles.m_Plugins[sPluginName];
    plugin.m_bLoadCopy = true;
    plugin.m_bSelected = true;
    plugin.m_LastModificationTime = ezTimestamp::CurrentTimestamp();
    plugin.m_ExclusiveFeatures.PushBack("GamePlugin");
    plugin.m_sDisplayName = sProjectName;
    txt.Set("C++ code for the ", sProjectName, " project.");
    plugin.m_sDescription = txt;
    plugin.m_RuntimePlugins.PushBack(sPluginName);

    ezQtEditorApp::GetSingleton()->WritePluginSelectionStateDDL();
  }
}

void ezQtCppProjectDlg::UpdateUI()
{
  PluginLocation->setText(GetTargetDir().GetData());
  BuildFolder->setText(GetBuildDir().GetData());

  OpenPluginLocation->setEnabled(ezOSFile::ExistsDirectory(PluginLocation->text().toUtf8().data()));
  OpenBuildFolder->setEnabled(ezOSFile::ExistsDirectory(BuildFolder->text().toUtf8().data()));
  OpenSolution->setEnabled(ezOSFile::ExistsFile(GetSolutionFile()));
}

ezString ezQtCppProjectDlg::GetTargetDir() const
{
  ezStringBuilder sTargetDir = ezToolsProject::GetSingleton()->GetProjectDirectory();
  sTargetDir.AppendPath("CppSource");

  return sTargetDir;
}

ezString ezQtCppProjectDlg::GetBuildDir() const
{
  ezStringBuilder sBuildDir;
  sBuildDir.Format("{}/Build/{}", GetTargetDir(), GetGeneratorFolder());

  return sBuildDir;
}

ezString ezQtCppProjectDlg::GetSolutionFile() const
{
  ezStringBuilder sSolutionFile;
  sSolutionFile = GetBuildDir();
  sSolutionFile.AppendPath(ezToolsProject::GetSingleton()->GetProjectName());
  sSolutionFile.Append(".sln");

  return sSolutionFile;
}

ezString ezQtCppProjectDlg::GetGeneratorCMake() const
{
  switch (Generator->currentIndex())
  {
    case 0:
      return "Visual Studio 16 2019";
    case 1:
      return "Visual Studio 17 2022";

      EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return "";
}

ezString ezQtCppProjectDlg::GetGeneratorFolder() const
{
  switch (Generator->currentIndex())
  {
    case 0:
      return "Vs2019x64";
    case 1:
      return "Vs2022x64";

      EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return "";
}
