

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/EditorApp/Configuration/Plugins.h>
#include <EditorFramework/ui_CreateProjectDlg.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/Status.h>
#include <GuiFoundation/Dialogs/Dialog.moc.h>

class EZ_EDITORFRAMEWORK_DLL ezQtCreateProjectDlg : public ezQtDialog, public Ui_ezQtCreateProjectDlg
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
  void FillProjectTemplatesList();
  ezStatus CreateProject();

  enum class State
  {
    Basics,
    Templates,
    Plugins,
    Summary,
    Create,
  };

  State m_State = State::Basics;
  ezString m_sProjectTemplate;
  ezPluginBundleSet m_LocalPluginSet;
};
