#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/ui_CppProjectDlg.h>
#include <QDialog>

class EZ_EDITORFRAMEWORK_DLL ezQtCppProjectDlg : public QDialog, public Ui_ezQtCppProjectDlg
{
public:
  Q_OBJECT

public:
  ezQtCppProjectDlg(QWidget* parent);

private Q_SLOTS:
  void on_Result_accepted();
  void on_Result_rejected();

private:
};

