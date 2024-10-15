#include <Fileserve/FileservePCH.h>

#include <Fileserve/Fileserve.h>

#ifdef EZ_USE_QT

#  include <EditorPluginFileserve/FileserveUI/FileserveWidget.moc.h>
#  include <Fileserve/Gui.moc.h>
#  include <Foundation/Application/Application.h>
#  include <QTimer>

void CreateFileserveMainWindow(ezApplication* pApp)
{
  ezQtFileserveMainWnd* pMainWnd = new ezQtFileserveMainWnd(pApp);
  pMainWnd->show();
}

ezQtFileserveMainWnd::ezQtFileserveMainWnd(ezApplication* pApp, QWidget* pParent)
  : QMainWindow(pParent)
  , m_pApp(pApp)
{
  OnServerStopped();

  m_pFileserveWidget = new ezQtFileserveWidget(this);
  QMainWindow::setCentralWidget(m_pFileserveWidget);
  resize(700, 650);

  connect(m_pFileserveWidget, &ezQtFileserveWidget::ServerStarted, this, &ezQtFileserveMainWnd::OnServerStarted);
  connect(m_pFileserveWidget, &ezQtFileserveWidget::ServerStopped, this, &ezQtFileserveMainWnd::OnServerStopped);

  show();

  QTimer::singleShot(0, this, &ezQtFileserveMainWnd::UpdateNetworkSlot);

  setWindowIcon(m_pFileserveWidget->windowIcon());
}


void ezQtFileserveMainWnd::UpdateNetworkSlot()
{
  m_pApp->Run();

  if (m_pApp->ShouldApplicationQuit())
  {
    close();
  }
  else
  {
    QTimer::singleShot(0, this, &ezQtFileserveMainWnd::UpdateNetworkSlot);
  }
}

void ezQtFileserveMainWnd::OnServerStarted(const QString& ip, ezUInt16 uiPort)
{
  QString title = QString("ezFileserve (Port %1)").arg(uiPort);

  setWindowTitle(title);
}

void ezQtFileserveMainWnd::OnServerStopped()
{
  setWindowTitle("ezFileserve (not running)");
}

#endif
