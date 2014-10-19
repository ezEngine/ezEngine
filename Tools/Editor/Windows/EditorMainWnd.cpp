#include <PCH.h>
#include <Editor/Windows/EditorMainWnd.moc.h>
#include <Editor/Dialogs/PluginDlg.moc.h>
#include <EditorFramework/EditorFramework.h>
#include <QFileDialog>
#include <QMessageBox>

ezEditorMainWnd* ezEditorMainWnd::s_pWidget = nullptr;

ezEditorMainWnd::ezEditorMainWnd() : QMainWindow()
{
  s_pWidget = this;

  setupUi(this);
}

ezEditorMainWnd::~ezEditorMainWnd()
{
  s_pWidget = nullptr;
}

void ezEditorMainWnd::closeEvent(QCloseEvent* event) 
{
  ezEditorFramework::SaveWindowLayout();
}

void ezEditorMainWnd::on_ActionConfigurePlugins_triggered()
{
  PluginDlg dlg(this);
  dlg.exec();
}

void ezEditorMainWnd::on_ActionProjectCreate_triggered()
{
  static QString sDir = QString::fromUtf8(ezOSFile::GetApplicationDirectory());
  QString sResult = QFileDialog::getSaveFileName(this, QLatin1String("Create Project"), sDir, QLatin1String("Projects (*.project)"));

  if (sResult.isEmpty())
    return;

  if (ezEditorFramework::CreateProject(sResult.toUtf8().data()).Failed())
  {
    QMessageBox::critical(this, QString::fromUtf8(ezEditorFramework::GetApplicationName().GetData()), QLatin1String("Failed to create the project"), QMessageBox::StandardButton::Ok, QMessageBox::StandardButton::Ok);
    return;
  }
}

void ezEditorMainWnd::on_ActionProjectOpen_triggered()
{
  static QString sDir = QString::fromUtf8(ezOSFile::GetApplicationDirectory());
  QString sResult = "D:\\Daten\\Code\\ezEngine\\Trunk\\Output\\Bin\\Projects\\test.project"; //QFileDialog::getOpenFileName(this, QLatin1String("Open Project"), sDir, QLatin1String("Projects (*.project)"));

  if (sResult.isEmpty())
    return;

  if (ezEditorFramework::OpenProject(sResult.toUtf8().data()).Failed())
  {
    QMessageBox::critical(this, QString::fromUtf8(ezEditorFramework::GetApplicationName().GetData()), QLatin1String("Failed to open the project"), QMessageBox::StandardButton::Ok, QMessageBox::StandardButton::Ok);
    return;
  }
}

void ezEditorMainWnd::on_ActionProjectClose_triggered()
{
  ezEditorFramework::CloseProject();
}

void ezEditorMainWnd::on_ActionSceneCreate_triggered()
{
  static QString sDir = QString::fromUtf8(ezOSFile::GetApplicationDirectory());
  QString sResult = QFileDialog::getSaveFileName(this, QLatin1String("Create Scene"), sDir, QLatin1String("Scenes (*.scene)"));

  if (sResult.isEmpty())
    return;

  if (ezEditorFramework::CreateScene(sResult.toUtf8().data()).Failed())
  {
    QMessageBox::critical(this, QString::fromUtf8(ezEditorFramework::GetApplicationName().GetData()), QLatin1String("Failed to create the scene"), QMessageBox::StandardButton::Ok, QMessageBox::StandardButton::Ok);
    return;
  }
}

void ezEditorMainWnd::on_ActionSceneOpen_triggered()
{
  static QString sDir = QString::fromUtf8(ezOSFile::GetApplicationDirectory());
  QString sResult = "D:\\Daten\\Code\\ezEngine\\Trunk\\Output\\Bin\\Projects\\Scenes\\myscene.scene";//QFileDialog::getOpenFileName(this, QLatin1String("Open Scene"), sDir, QLatin1String("Scenes (*.scene)"));

  if (sResult.isEmpty())
    return;

  if (ezEditorFramework::OpenScene(sResult.toUtf8().data()).Failed())
  {
    QMessageBox::critical(this, QString::fromUtf8(ezEditorFramework::GetApplicationName().GetData()), QLatin1String("Failed to open the scene"), QMessageBox::StandardButton::Ok, QMessageBox::StandardButton::Ok);
    return;
  }
}

void ezEditorMainWnd::on_ActionSceneClose_triggered()
{
  ezEditorFramework::CloseScene();
}


