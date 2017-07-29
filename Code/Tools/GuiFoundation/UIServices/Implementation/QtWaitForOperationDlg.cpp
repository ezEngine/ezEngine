#include <PCH.h>
#include <GuiFoundation/UIServices/QtWaitForOperationDlg.moc.h>
#include <QTimer>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
#include <QtWinExtras/QWinTaskbarButton>
#include <QtWinExtras/QWinTaskbarProgress>
#endif

ezQtWaitForOperationDlg::ezQtWaitForOperationDlg(QWidget* parent)
  : QDialog(parent)
{
  setupUi(this);

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  m_pWinTaskBarButton = new QWinTaskbarButton(QApplication::activeWindow());
  m_pWinTaskBarButton->setWindow(QApplication::activeWindow()->windowHandle());

  m_pWinTaskBarProgress = m_pWinTaskBarButton->progress();
  m_pWinTaskBarProgress->setMinimum(0);
  m_pWinTaskBarProgress->setMaximum(0);
  m_pWinTaskBarProgress->setValue(0);
  m_pWinTaskBarProgress->reset();
  m_pWinTaskBarProgress->show();
  m_pWinTaskBarProgress->setVisible(true);
#endif

  QTimer::singleShot(10, this, &ezQtWaitForOperationDlg::onIdle);
}

ezQtWaitForOperationDlg::~ezQtWaitForOperationDlg()
{
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  if (m_pWinTaskBarProgress)
  {
    m_pWinTaskBarProgress->hide();
    m_pWinTaskBarProgress = nullptr;
  }

  if (m_pWinTaskBarButton)
  {
    delete m_pWinTaskBarButton;
    m_pWinTaskBarButton = nullptr;
  }
#endif
}

void ezQtWaitForOperationDlg::on_ButtonCancel_clicked()
{
  reject();
}

void ezQtWaitForOperationDlg::onIdle()
{
  if (m_OnIdle())
  {
    QTimer::singleShot(10, this, &ezQtWaitForOperationDlg::onIdle);
  }
  else
  {
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
    m_pWinTaskBarProgress->setMinimum(0);
    m_pWinTaskBarProgress->setMaximum(100);
    m_pWinTaskBarProgress->setValue(99);
    m_pWinTaskBarProgress->setPaused(true);
#endif

    accept();
  }
}
