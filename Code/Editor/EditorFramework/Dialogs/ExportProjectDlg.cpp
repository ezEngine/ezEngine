#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/CodeGen/CppProject.h>
#include <EditorFramework/Dialogs/ExportProjectDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Preferences/Preferences.h>
#include <EditorFramework/Preferences/ProjectPreferences.h>
#include <EditorFramework/Project/ProjectExport.h>
#include <Foundation/CodeUtils/Preprocessor.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Math/ColorScheme.h>
#include <Foundation/Strings/String.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QSyntaxHighlighter>
#include <QUrl>
#include <ToolsFoundation/Utilities/PathPatternFilter.h>

namespace
{
  /// Colors the lines of the export log, based on the prefixes that ezLogSystemToBuffer writes.
  class ExportLogHighlighter : public QSyntaxHighlighter
  {
  public:
    using QSyntaxHighlighter::QSyntaxHighlighter;

  protected:
    virtual void highlightBlock(const QString& sText) override
    {
      if (sText.startsWith(QLatin1String("Error: ")))
      {
        setFormat(0, sText.length(), ezToQtColor(ezColorScheme::LightUI(ezColorScheme::Red)));
      }
      else if (sText.startsWith(QLatin1String("Warning: ")))
      {
        setFormat(0, sText.length(), ezToQtColor(ezColorScheme::LightUI(ezColorScheme::Yellow)));
      }
    }
  };
} // namespace

bool ezQtExportProjectDlg::s_bTransformAll = true;
bool ezQtExportProjectDlg::s_bCompileCpp = true;
bool ezQtExportProjectDlg::s_bRunPostExportScript = true;

ezQtExportProjectDlg::ezQtExportProjectDlg(QWidget* pParent)
  : ezQtDialog(pParent)
{
  setupUi(this);

  new ExportLogHighlighter(ExportLog->document());

  ezProjectPreferencesUser* pPref = ezPreferences::QueryPreferences<ezProjectPreferencesUser>();

  Destination->setText(pPref->m_sExportFolder.GetData());

  // setText() doesn't send a change notification when the stored folder is empty
  UpdateOpenFolderButton();
}

void ezQtExportProjectDlg::showEvent(QShowEvent* e)
{
  QDialog::showEvent(e);

  TransformAll->setChecked(s_bTransformAll);

  if (!ezCppProject::ExistsProjectCMakeListsTxt())
  {
    CompileCpp->setEnabled(false);
    CompileCpp->setToolTip("This project doesn't have a C++ plugin.");
    CompileCpp->setChecked(false);
  }
  else
  {
    CompileCpp->setChecked(s_bCompileCpp);
  }

  ezString sScriptFile;
  const ezStatus scriptStatus = ezProjectExport::FindPostExportScript(sScriptFile);

  if (scriptStatus.Failed())
  {
    // stays enabled, because an export with this checked fails with this message, unchecking it is one way out
    RunPostExportScript->setChecked(s_bRunPostExportScript);
    RunPostExportScript->setToolTip(ezMakeQString(scriptStatus.GetMessageString()));
  }
  else if (sScriptFile.IsEmpty())
  {
    RunPostExportScript->setEnabled(false);
    RunPostExportScript->setChecked(false);
    RunPostExportScript->setToolTip("This project doesn't have a post-export script.\nAdd a script named 'PostExport' to the project's 'Editor' folder to run custom logic on the exported files. See the help for details.");
  }
  else
  {
    ezStringBuilder sText("Run Post-Export Script (", ezPathUtils::GetFileNameAndExtension(sScriptFile), ")");
    RunPostExportScript->setText(ezMakeQString(sText));
    RunPostExportScript->setChecked(s_bRunPostExportScript);
    RunPostExportScript->setToolTip("Runs this script from the project's 'Editor' folder on the exported files. If it fails, the export counts as failed.");
  }
}

void ezQtExportProjectDlg::on_BrowseDestination_clicked()
{
  QString sPath = QFileDialog::getExistingDirectory(this, QLatin1String("Select output directory"), Destination->text());

  if (!sPath.isEmpty())
  {
    Destination->setText(sPath);
  }
}

void ezQtExportProjectDlg::on_ExportProjectButton_clicked()
{
  // TODO:
  // select asset profile for export

  ezProjectExportOptions options;
  options.m_bCompileCppPlugin = CompileCpp->isChecked();
  options.m_bTransformAssets = TransformAll->isChecked();
  options.m_bRunPostExportScript = RunPostExportScript->isChecked();

  const ezString sDstFolder = Destination->text().toUtf8().data();

  if (ezPathUtils::IsAbsolutePath(sDstFolder) && ezOSFile::ExistsDirectory(sDstFolder))
  {
    ezFileSystemIterator it;
    it.StartSearch(sDstFolder, ezFileSystemIteratorFlags::ReportFilesAndFoldersRecursive);

    if (it.IsValid())
    {
      if (ezQtUiServices::GetSingleton()->MessageBoxQuestion(ezFmt("The folder '{}' is not empty.\n\nThe export deletes everything in it. Continue?", sDstFolder), QMessageBox::Yes | QMessageBox::No, QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
        return;
    }
  }

  m_Report = ezProjectExportReport();
  const ezStatus res = ezProjectExport::ExportProjectComplete(sDstFolder, options, &m_Report);

  const bool bHasProblems = m_Report.m_uiNumErrors > 0 || m_Report.m_uiNumWarnings > 0;

  // among thousands of lines about copied files the problems are otherwise easily missed
  {
    QSignalBlocker blocker(ProblemsOnly);
    ProblemsOnly->setChecked(bHasProblems);
  }

  UpdateLogView();

  // the log is also of interest for a failed export, but there is none when the destination was rejected right away
  SaveLogButton->setEnabled(!m_Report.m_sLog.IsEmpty());
  UpdateOpenFolderButton();

  if (res.Failed())
  {
    ezQtUiServices::GetSingleton()->MessageBoxStatus(res, "Project export failed. See log for details.");
  }
  else if (bHasProblems)
  {
    ezQtUiServices::GetSingleton()->MessageBoxWarning(ezFmt("Project exported with problems. See log for details.\n\nErrors: {}\nWarnings: {}", m_Report.m_uiNumErrors, m_Report.m_uiNumWarnings));
  }
  else
  {
    ezQtUiServices::GetSingleton()->MessageBoxInformation(ezFmt("Project exported: {} files, {}.", m_Report.m_uiNumFiles, ezArgFileSize(m_Report.m_uiTotalBytes)), "project-export-success");
  }
}

void ezQtExportProjectDlg::on_OpenFolderButton_clicked()
{
  const ezString sFolder = Destination->text().toUtf8().data();
  ezQtUiServices::GetSingleton()->OpenInExplorer(sFolder, false);
}

void ezQtExportProjectDlg::on_SaveLogButton_clicked()
{
  // Suggests a location next to the destination folder, because inside of it the file would become part
  // of the exported game and get deleted by the next export.
  const QString sSuggestion = QFileInfo(QDir::cleanPath(Destination->text())).dir().filePath(QLatin1String("ExportLog.txt"));

  const QString sFile = QFileDialog::getSaveFileName(this, QLatin1String("Save Export Log"), sSuggestion, QLatin1String("Text Files (*.txt)"), nullptr, QFileDialog::Option::DontResolveSymlinks);

  if (sFile.isEmpty())
    return;

  const ezString sFilePath = sFile.toUtf8().data();

  ezOSFile file;
  if (file.Open(sFilePath, ezFileOpenMode::Write).Failed() || file.Write(m_Report.m_sLog.GetData(), m_Report.m_sLog.GetElementCount()).Failed())
  {
    ezQtUiServices::GetSingleton()->MessageBoxWarning(ezFmt("Failed to write the export log to '{}'.", sFilePath));
  }
}

void ezQtExportProjectDlg::on_Destination_textChanged(const QString& sText)
{
  ezProjectPreferencesUser* pPref = ezPreferences::QueryPreferences<ezProjectPreferencesUser>();
  pPref->m_sExportFolder = sText.toUtf8().data();

  UpdateOpenFolderButton();
}

void ezQtExportProjectDlg::on_TransformAll_clicked(bool bChecked)
{
  s_bTransformAll = bChecked;
}

void ezQtExportProjectDlg::on_CompileCpp_clicked(bool bChecked)
{
  s_bCompileCpp = bChecked;
}

void ezQtExportProjectDlg::on_RunPostExportScript_clicked(bool bChecked)
{
  s_bRunPostExportScript = bChecked;
}

void ezQtExportProjectDlg::on_HelpButton_clicked()
{
  QDesktopServices::openUrl(QUrl("https://ezengine.net/pages/docs/shipping/project-export.html"));
}

void ezQtExportProjectDlg::on_ProblemsOnly_toggled(bool bChecked)
{
  UpdateLogView();
}

void ezQtExportProjectDlg::UpdateOpenFolderButton()
{
  const ezStringBuilder sFolder = Destination->text().toUtf8().data();

  // ExistsDirectory() asserts on relative paths, which is what a partially typed path may be
  OpenFolderButton->setEnabled(sFolder.IsAbsolutePath() && ezOSFile::ExistsDirectory(sFolder));
}

void ezQtExportProjectDlg::UpdateLogView()
{
  ExportLog->setPlainText(ProblemsOnly->isChecked() ? m_Report.m_sProblems.GetData() : m_Report.m_sLog.GetData());

  // the full log ends with the summary
  ExportLog->moveCursor(QTextCursor::End);
}
