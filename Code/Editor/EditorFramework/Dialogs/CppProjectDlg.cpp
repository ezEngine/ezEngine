#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/CodeGen/CppProject.h>
#include <EditorFramework/CodeGen/CppSettings.h>
#include <EditorFramework/Dialogs/CppProjectDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/System/Process.h>
#include <ToolsFoundation/Application/ApplicationServices.h>


ezQtCppProjectDlg::ezQtCppProjectDlg(QWidget* pParent)
  : QDialog(pParent)
{
  setupUi(this);

  m_OldCppSettings.Load().IgnoreResult();
  m_CppSettings.Load().IgnoreResult();

  {
    ezQtScopedBlockSignals _1(PluginName);
    PluginName->setPlaceholderText(ezToolsProject::GetSingleton()->GetProjectName(true).GetData());
    PluginName->setText(m_CppSettings.m_sPluginName.GetData());
  }

  if (ezStatus compilerTestResult = ezCppProject::TestCompiler(); compilerTestResult.Failed())
  {
    // TODO: how do I color the ErrorText label in Red (or whatever error color is configured?)
    ezStringBuilder fmt;
    fmt.SetFormat("<html><b>Error:</b> {}<br>Please go to preferences and configure the C & C++ compiler.", compilerTestResult.GetMessageString());

    ErrorText->setText(ezMakeQString(fmt));
    GenerateSolution->setDisabled(true);
  }

  UpdateUI();
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

void ezQtCppProjectDlg::on_OpenSolution_clicked()
{
  if (auto result = ezCppProject::OpenSolution(m_CppSettings); result.Failed())
  {
    ezQtUiServices::GetSingleton()->MessageBoxWarning(result.GetMessageString().GetView());
  }
}

void ezQtCppProjectDlg::on_PluginName_textEdited(const QString& text)
{
  ezStringBuilder name = PluginName->text().toUtf8().data();

  if (name.EndsWith_NoCase("Plugin"))
  {
    name.Shrink(0, 6);
  }

  m_CppSettings.m_sPluginName = name;

  UpdateUI();
}

void ezQtCppProjectDlg::UpdateUI()
{
  PluginLocation->setText(ezCppProject::GetTargetSourceDir().GetData());
  BuildFolder->setText(ezCppProject::GetBuildDir(m_CppSettings).GetData());

  OpenPluginLocation->setEnabled(ezOSFile::ExistsDirectory(PluginLocation->text().toUtf8().data()));
  OpenBuildFolder->setEnabled(ezOSFile::ExistsDirectory(BuildFolder->text().toUtf8().data()));
  OpenSolution->setEnabled(ezCppProject::ExistsSolution(m_CppSettings));
}

class ezForwardToQTextEdit : public ezLogInterface
{
public:
  QTextEdit* m_pTextEdit = nullptr;

  void HandleLogMessage(const ezLoggingEventData& le) override
  {
    switch (le.m_EventType)
    {
      case ezLogMsgType::GlobalDefault:
      case ezLogMsgType::Flush:
      case ezLogMsgType::BeginGroup:
      case ezLogMsgType::EndGroup:
      case ezLogMsgType::None:
      case ezLogMsgType::All:
      case ezLogMsgType::ENUM_COUNT:
        return;

      case ezLogMsgType::ErrorMsg:
      case ezLogMsgType::SeriousWarningMsg:
      case ezLogMsgType::WarningMsg:
      case ezLogMsgType::SuccessMsg:
      case ezLogMsgType::InfoMsg:
      case ezLogMsgType::DevMsg:
      case ezLogMsgType::DebugMsg:
      {
        ezStringBuilder tmp(le.m_sText, "\n");

        QString s = m_pTextEdit->toPlainText();
        s.append(tmp);
        m_pTextEdit->setText(s);
        return;
      }

        EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
    }
  }
};

void ezQtCppProjectDlg::on_GenerateSolution_clicked()
{
  if (ezCppProject::ExistsSolution(m_CppSettings))
  {
    if (ezQtUiServices::MessageBoxQuestion("The solution already exists, do you want to recreate it?", QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::No) != QMessageBox::StandardButton::Yes)
    {
      return;
    }
  }

  if (m_CppSettings.m_sPluginName.IsEmpty())
  {
    m_CppSettings.m_sPluginName = PluginName->placeholderText().toUtf8().data();
  }

  if (!m_OldCppSettings.m_sPluginName.IsEmpty() && m_OldCppSettings.m_sPluginName != m_CppSettings.m_sPluginName)
  {
    if (ezQtUiServices::MessageBoxQuestion("You are attempting to change the name of the existing C++ plugin.\n\nTHIS IS A BAD IDEA.\n\nThe C++ sources and CMake files were already created with the old name in it. To not accidentally delete your work, EZ won't touch any of those files. Therefore this change won't have any effect, unless you have already deleted those files yourself and EZ can just create new ones. Only select YES if you have done the necessary steps and/or know what you are doing.", QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::No) != QMessageBox::StandardButton::Yes)
    {
      return;
    }
  }

  if (m_CppSettings.Save().Failed())
  {
    ezQtUiServices::GetSingleton()->MessageBoxWarning("Saving new C++ project settings failed.");
    return;
  }

  m_OldCppSettings.Load().IgnoreResult();

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  if (ezSystemInformation::IsDebuggerAttached())
  {
    ezQtUiServices::GetSingleton()->MessageBoxWarning("When a debugger is attached, CMake usually fails with the error that no C/C++ compiler can be found.\n\nDetach the debugger now, then press OK to continue.");
  }
#endif

  OutputLog->clear();

  {
    ezForwardToQTextEdit log;
    log.m_pTextEdit = OutputLog;
    ezLogSystemScope _logScope(&log);

    ezProgressRange progress("Generating Solution", 3, false);
    progress.SetStepWeighting(0, 0.1f);
    progress.SetStepWeighting(1, 0.1f);
    progress.SetStepWeighting(2, 0.8f);

    EZ_SCOPE_EXIT(UpdateUI());

    {
      progress.BeginNextStep("Clean Build Directory");

      if (ezCppProject::CleanBuildDir(m_CppSettings).Failed())
      {
        ezLog::Warning("Couldn't delete build output directory:\n{}\n\nProject is probably already open in Visual Studio.\n", ezCppProject::GetBuildDir(m_CppSettings));
      }
    }

    {
      progress.BeginNextStep("Populate with Default Sources");
      if (ezCppProject::PopulateWithDefaultSources(m_CppSettings).Failed())
      {
        ezQtUiServices::GetSingleton()->MessageBoxWarning("Failed to populate the CppSource directory with the default files.\n\nCheck the log for details.");
        return;
      }
    }

    // run CMake
    {
      progress.BeginNextStep("Running CMake");

      if (ezCppProject::RunCMake(m_CppSettings).Failed())
      {

        ezQtUiServices::GetSingleton()->MessageBoxWarning("Generating the solution failed.\n\nCheck the log for details.");
        return;
      }
    }

    if (ezCppProject::BuildCodeIfNecessary(m_CppSettings).Failed())
    {
      ezLog::Error("Failed to compile the newly generated C++ solution.");
    }
  }

  ezCppProject::UpdatePluginConfig(m_CppSettings);

  ezQtEditorApp::GetSingleton()->RestartEngineProcessIfPluginsChanged(true);

  if (ezQtUiServices::GetSingleton()->MessageBoxQuestion("The solution was generated successfully.\n\nDo you want to open it now?", QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes)
  {
    on_OpenSolution_clicked();
  }
}
