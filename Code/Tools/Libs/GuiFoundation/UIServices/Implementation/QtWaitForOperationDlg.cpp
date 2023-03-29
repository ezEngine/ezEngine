#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/UIServices/QtWaitForOperationDlg.moc.h>
#include <QTimer>

ezQtWaitForOperationDlg::ezQtWaitForOperationDlg(QWidget* pParent)
  : QDialog(pParent)
{
  setupUi(this);

  QTimer::singleShot(10, this, &ezQtWaitForOperationDlg::onIdle);
}

ezQtWaitForOperationDlg::~ezQtWaitForOperationDlg()
{
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
    accept();
  }
}
