#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/CodeGen/CppProject.h>
#include <EditorFramework/CodeGen/CppSettings.h>
#include <EditorFramework/Dialogs/CppProjectDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OSFile.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

#include <Shlobj.h>

ezQtCppProjectDlg::ezQtCppProjectDlg(QWidget* pParent)
  : QDialog(pParent)
{
  setupUi(this);

  m_CppSettings.Load().IgnoreResult();

  {
    ezQtScopedBlockSignals _1(PluginName);
    PluginName->setPlaceholderText(ezToolsProject::GetSingleton()->GetProjectName(true).GetData());
    PluginName->setText(m_CppSettings.m_sPluginName.GetData());
  }

  if (ezCppProject::ExistsProjectCMakeListsTxt())
  {
    PluginName->setToolTip("The name cannot be changed once the CppSource folder has been created.");
    PluginName->setEnabled(false);
  }

  {
    ezQtScopedBlockSignals _1(Generator);
    Generator->addItem("None");
    Generator->addItem("Visual Studio 2019");
    Generator->addItem("Visual Studio 2022");

    if (m_CppSettings.m_Compiler == ezCppSettings::Compiler::Vs2019)
    {
      Generator->setCurrentIndex(1);
    }
    else if (m_CppSettings.m_Compiler == ezCppSettings::Compiler::Vs2022)
    {
      Generator->setCurrentIndex(2);
    }
    else
    {
      Generator->setCurrentIndex(0);
    }
  }

  UpdateUI();
}

ezResult ezQtCppProjectDlg::RunCMake()
{
  if (m_CppSettings.m_sPluginName.IsEmpty())
  {
    m_CppSettings.m_sPluginName = PluginName->placeholderText().toUtf8().data();
  }

  m_CppSettings.Save().IgnoreResult();

  if (ezSystemInformation::IsDebuggerAttached())
  {
    ezQtUiServices::GetSingleton()->MessageBoxWarning("When a debugger is attached, CMake can fail with the error that no C/C++ compiler can be found.");
  }

  ezProgressRange progress("Generating Solution", 2, false);
  progress.SetStepWeighting(0, 0.2f);
  progress.SetStepWeighting(1, 0.1f);
  progress.SetStepWeighting(2, 0.8f);

  QApplication::setOverrideCursor(Qt::WaitCursor);

  OutputLog->clear();
  ezStringBuilder output;
  EZ_SCOPE_EXIT(OutputLog->setText(output.GetData()));
  EZ_SCOPE_EXIT(QApplication::restoreOverrideCursor());
  EZ_SCOPE_EXIT(UpdateUI());

  {
    progress.BeginNextStep("Populate with Default Sources");
    EZ_SUCCEED_OR_RETURN(ezCppProject::PopulateWithDefaultSources(m_CppSettings, output));
  }

  {
    progress.BeginNextStep("Clean Build Directory");

    if (ezCppProject::CleanBuildDir(m_CppSettings).Failed())
    {
      output.AppendFormat("Couldn't delete build output directory:\n{}\n\nProject is probably already open in Visual Studio.\n", ezCppProject::GetBuildDir(m_CppSettings));
    }
  }

  // run CMake
  {
    progress.BeginNextStep("Running CMake");

    EZ_SUCCEED_OR_RETURN(ezCppProject::RunCMake(m_CppSettings, output));
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
  switch (Generator->currentIndex())
  {
    case 0:
      m_CppSettings.m_Compiler = ezCppSettings::Compiler::None;
      break;
    case 1:
      m_CppSettings.m_Compiler = ezCppSettings::Compiler::Vs2019;
      break;
    case 2:
      m_CppSettings.m_Compiler = ezCppSettings::Compiler::Vs2022;
      break;
  }

  UpdateUI();
}

void ezQtCppProjectDlg::on_OpenSolution_clicked()
{
  if (!ezQtUiServices::OpenFileInDefaultProgram(ezCppProject::GetSolutionPath(m_CppSettings)))
  {
    ezQtUiServices::GetSingleton()->MessageBoxWarning("Opening the solution failed.");
  }
}

void ezQtCppProjectDlg::on_GenerateSolution_clicked()
{
  if (ezCppProject::ExistsSolution(m_CppSettings))
  {
    if (ezQtUiServices::MessageBoxQuestion("The solution already exists, do you want to recreate it?", QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::No) != QMessageBox::StandardButton::Yes)
      return;
  }

  if (RunCMake().Failed())
  {
    ezQtUiServices::GetSingleton()->MessageBoxWarning("Generating the solution failed. Check the log output for details.");
  }
  else
  {
    ezStringBuilder txt;
    txt.Format("The solution was generated successfully.\n\nDo you want to open the solution now?");

    if (ezQtUiServices::GetSingleton()->MessageBoxQuestion(txt, QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes)
    {
      on_OpenSolution_clicked();
    }

    ezStringBuilder sProjectName = m_CppSettings.m_sPluginName;
    ezStringBuilder sPluginName(sProjectName, "Plugin");

    ezPluginBundleSet& bundles = ezQtEditorApp::GetSingleton()->GetPluginBundles();

    bundles.m_Plugins.Remove(sPluginName);
    ezPluginBundle& plugin = bundles.m_Plugins[sPluginName];
    plugin.m_bLoadCopy = true;
    plugin.m_bSelected = true;
    plugin.m_bMissing = true;
    plugin.m_LastModificationTime = ezTimestamp::CurrentTimestamp();
    plugin.m_ExclusiveFeatures.PushBack("ProjectPlugin");
    txt.Set("'", sProjectName, "' project plugin");
    plugin.m_sDisplayName = txt;
    txt.Set("C++ code for the '", sProjectName, "' project.");
    plugin.m_sDescription = txt;
    plugin.m_RuntimePlugins.PushBack(sPluginName);

    ezQtEditorApp::GetSingleton()->WritePluginSelectionStateDDL();
  }
}

void ezQtCppProjectDlg::on_CompileSolution_clicked()
{
  ezStringBuilder sVsWhere;

  wchar_t* pPath = nullptr;
  if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_ProgramFilesX86, KF_FLAG_DEFAULT, nullptr, &pPath)))
  {
    sVsWhere = ezStringWChar(pPath);
    sVsWhere.AppendPath("Microsoft Visual Studio/Installer/vswhere.exe");

    CoTaskMemFree(pPath);
  }
  else
  {
    return;
  }

  ezStringBuilder sMsBuildPath;

  {
    ezStringBuilder sStdOut;
    ezProcessOptions po;
    po.m_sProcess = sVsWhere;
    po.AddCommandLine("-latest -requires Microsoft.Component.MSBuild -find MSBuild\\**\\Bin\\MSBuild.exe");
    po.m_bHideConsoleWindow = true;
    po.m_onStdOut = [&](ezStringView res)
    { sStdOut.Append(res); };

    if (ezProcess::Execute(po).Failed())
      return;

    sMsBuildPath = sStdOut;
    sMsBuildPath.Trim("\n\r");
    sMsBuildPath.MakeCleanPath();
  }

  {

    ezStringBuilder sMsBuildCmd;
    sMsBuildCmd.AppendFormat("\"{}\"", GetSolutionFile());
    sMsBuildCmd.AppendFormat(" /m /nr:false"); // multi-threaded compilation
    sMsBuildCmd.AppendFormat(" /t:Build");
    sMsBuildCmd.AppendFormat(" /p:Configuration={}", BUILDSYSTEM_BUILDTYPE);
    sMsBuildCmd.AppendFormat(" /p:Platform=x64");

    ezStringBuilder sStdOut;
    ezProcessOptions po;
    po.m_sProcess = sMsBuildPath;
    po.AddCommandLine(sMsBuildCmd);
    po.m_bHideConsoleWindow = true;
    po.m_onStdOut = [&](ezStringView res)
    { sStdOut.Append(res); };

    if (ezProcess::Execute(po).Failed())
      return;
  }
  // Run(msBuildPath, $"\"{solutionProjectPath}\" {(buildInfo.Multicore ? "/m /nr:false" : "")} /t:{(buildInfo.RebuildAppx ? "Rebuild" : "Build")} /p:Configuration={buildInfo.Configuration} /p:Platform={buildInfo.BuildPlatform} {(string.IsNullOrEmpty(buildInfo.PlatformToolset) ? string.Empty : $"/p:PlatformToolset={buildInfo.PlatformToolset}")} {GetMSBuildLoggingCommand(buildInfo.LogDirectory, "buildAppx.log")}",

  // ezQtUiServices::GetSingleton()->MessageBoxInformation(sResult);

}

void ezQtCppProjectDlg::on_PluginName_textEdited(const QString& text)
{
  m_CppSettings.m_sPluginName = PluginName->text().toUtf8().data();

  UpdateUI();
}

void ezQtCppProjectDlg::UpdateUI()
{
  PluginLocation->setText(ezCppProject::GetTargetSourceDir().GetData());
  BuildFolder->setText(ezCppProject::GetBuildDir(m_CppSettings).GetData());

  GenerateSolution->setEnabled(m_CppSettings.m_Compiler != ezCppSettings::Compiler::None);
  OpenPluginLocation->setEnabled(ezOSFile::ExistsDirectory(PluginLocation->text().toUtf8().data()));
  OpenBuildFolder->setEnabled(ezOSFile::ExistsDirectory(BuildFolder->text().toUtf8().data()));
  OpenSolution->setEnabled(ezCppProject::ExistsSolution(m_CppSettings));
}
