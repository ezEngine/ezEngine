

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/CodeGen/CppSettings.h>
#include <EditorFramework/ui_CppProjectDlg.h>
#include <Foundation/Strings/String.h>
#include <QDialog>

class EZ_EDITORFRAMEWORK_DLL ezQtCppProjectDlg : public QDialog, public Ui_ezQtCppProjectDlg
{
public:
  Q_OBJECT

public:
  ezQtCppProjectDlg(QWidget* pParent);

private Q_SLOTS:
  void on_Result_rejected();
  void on_OpenPluginLocation_clicked();
  void on_OpenBuildFolder_clicked();
  void on_OpenSolution_clicked();
  void on_GenerateSolution_clicked();
  void on_PluginName_textEdited(const QString& text);

private:
  void UpdateUI();

  ezCppSettings m_OldCppSettings;
  ezCppSettings m_CppSettings;
};
