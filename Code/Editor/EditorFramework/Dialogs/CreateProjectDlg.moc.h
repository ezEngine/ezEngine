

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
  void on_Prev_clicked();
  void on_Next_clicked();

private:
  void UpdateUI();

  enum class State
  {
    Basics,
    Plugins,
    Create,
  };

  State m_state = State::Basics;
  ezPluginBundleSet m_LocalPluginSet;
};
