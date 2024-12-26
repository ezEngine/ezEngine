

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/ui_CreateProjectDlg.h>
#include <Foundation/Strings/String.h>
#include <QDialog>

class EZ_EDITORFRAMEWORK_DLL ezQtCreateProjectDlg : public QDialog, public Ui_ezQtCreateProjectDlg
{
public:
  Q_OBJECT

public:
  ezQtCreateProjectDlg(QWidget* pParent);

private Q_SLOTS:

private:
};
