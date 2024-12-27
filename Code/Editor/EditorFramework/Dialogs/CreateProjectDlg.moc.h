

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/EditorApp/Configuration/Plugins.h>
#include <EditorFramework/ui_CreateProjectDlg.h>
#include <Foundation/Strings/String.h>
#include <QDialog>

class EZ_EDITORFRAMEWORK_DLL ezQtCreateProjectDlg : public QDialog, public Ui_ezQtCreateProjectDlg
{
public:
  Q_OBJECT

public:
  ezQtCreateProjectDlg(QWidget* pParent);

  ezString GetFullTargetPath() const;

  ezString m_sTargetFolder;
  ezString m_sTargetName;

private Q_SLOTS:
  void on_BrowseFolder_clicked();
  void on_ProjectName_textChanged(QString text);
  void on_CreateProject_clicked();

private:
  void UpdateeUI();

  ezPluginBundleSet m_LocalPluginSet;
};
