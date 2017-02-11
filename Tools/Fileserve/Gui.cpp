#include <Fileserve/Main.h>

#ifdef EZ_USE_QT

#include <EditorPluginFileserve/FileserveUI/FileserveWidget.moc.h>
#include <Core/Application/Application.h>
#include <Gui.moc.h>
#include <QTimer>

void CreateFileserveMainWindow(ezApplication* pApp)
{
  ezQtFileserveMainWnd* pMainWnd = new ezQtFileserveMainWnd(pApp);
  pMainWnd->show();
}

ezQtFileserveMainWnd::ezQtFileserveMainWnd(ezApplication* pApp, QWidget* parent)
  : QMainWindow(parent)
  , m_pApp(pApp)
{
  m_pFileserveWidget = new ezQtFileserveWidget(this);
  QMainWindow::setCentralWidget(m_pFileserveWidget);
  show();

  QTimer::singleShot(0, this, &ezQtFileserveMainWnd::UpdateNetworkSlot);
}


void ezQtFileserveMainWnd::UpdateNetworkSlot()
{
  if (m_pApp->Run() == ezApplication::Continue)
  {
    QTimer::singleShot(0, this, &ezQtFileserveMainWnd::UpdateNetworkSlot);
  }
  else
  {
    close();
  }
}

#endif

