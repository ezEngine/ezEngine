#include <Inspector/MainWindow.moc.h>
#include <Inspector/GeneralWidget.moc.h>
#include <Inspector/MemoryWidget.moc.h>
#include <Inspector/InputWidget.moc.h>
#include <Inspector/LogWidget.moc.h>
#include <Foundation/Communication/Telemetry.h>
#include <qlistwidget.h>
#include <qinputdialog.h>
#include <qfile.h>

ezMainWindow* ezMainWindow::s_pWidget = NULL;

ezMainWindow::ezMainWindow() : QMainWindow()
{
  s_pWidget = this;

  setupUi(this);

}

void ezMainWindow::SaveLayout (const char* szFile) const
{
  QFile f(szFile);
  if (!f.open(QIODevice::WriteOnly))
    return;

  QDataStream out(&f);

  ezUInt32 uiVersion = 1;
  out << uiVersion;

  QByteArray store1 = saveState ();
  QByteArray store2 = saveGeometry ();

  ezUInt32 uiBytes1 = store1.size ();
  out << uiBytes1;

  for (int i = 0; i < store1.size (); ++i)
  {
    unsigned char uc = store1[i];
    out << uc;
  }

  ezUInt32 uiBytes2 = store2.size ();
  out << uiBytes2;

  for (int i = 0; i < store2.size (); ++i)
  {
    unsigned char uc = store2[i];
    out << uc;
  }

  f.close();
}

void ezMainWindow::LoadLayout (const char* szFile)
{
  QFile f(szFile);
  if (!f.open(QIODevice::ReadOnly))
    return;

  QDataStream in(&f);

  ezUInt32 uiVersion = 0;
  in >> uiVersion;

  if (uiVersion != 1)
    return;

  ezUInt32 uiBytes1;
  ezUInt32 uiBytes2;

  in >> uiBytes1;

  QByteArray store1;

  for (ezUInt32 ui = 0; ui < uiBytes1; ++ui)
  {
    unsigned char uc;
    in >> uc;
    store1.append (uc);
  }

  restoreState (store1);

  in >> uiBytes2;

  QByteArray store2;

  for (ezUInt32 ui = 0; ui < uiBytes2; ++ui)
  {
    unsigned char uc;
    in >> uc;
    store2.append (uc);
  }

  restoreGeometry (store2);

}


void ezMainWindow::paintEvent(QPaintEvent* event)
{
  bool bResetStats = false;

  {
    static ezUInt32 uiServerID = 0;
    static bool bConnected = false;

    if (ezTelemetry::IsConnectedToServer())
    {
      if ((uiServerID == 0) && (ezTelemetry::GetServerID() != 0))
      {
        uiServerID = ezTelemetry::GetServerID();
        bResetStats = true;

        ezStringBuilder s;
        s.Format("Connected to Server with ID %i", uiServerID);

        ezMainWindow::s_pWidget->Log(s.GetData());

        ezTelemetry::SendToServer('APP', 'RQDT');
      }

      if (uiServerID != ezTelemetry::GetServerID())
      {
        uiServerID = ezTelemetry::GetServerID();
        bResetStats = true;

        ezStringBuilder s;
        s.Format("Connected to Server with ID %i", uiServerID);

        ezMainWindow::s_pWidget->Log(s.GetData());
      }
      else
      if (!bConnected)
      {
        ezMainWindow::s_pWidget->Log("Reconnected to Server.");
      }

      bConnected = true;
    }
    else
    {
      if (bConnected)
      {
        ezMainWindow::s_pWidget->Log("Lost Connection to Server.");
      }

      bConnected = false;
    }
  }

  if (bResetStats)
  {
    if (ezLogWidget::s_pWidget)
      ezLogWidget::s_pWidget->ResetStats();

    if (ezGeneralWidget::s_pWidget)
      ezGeneralWidget::s_pWidget->ResetStats();

    if (ezMemoryWidget::s_pWidget)
      ezMemoryWidget::s_pWidget->ResetStats();
  }


  if (ezLogWidget::s_pWidget)
    ezLogWidget::s_pWidget->UpdateStats();

  if (ezGeneralWidget::s_pWidget)
    ezGeneralWidget::s_pWidget->UpdateStats();

  if (ezMemoryWidget::s_pWidget)
    ezMemoryWidget::s_pWidget->UpdateStats();

  ezTelemetry::CallProcessMessagesCallbacks();

  update();
}

void ezMainWindow::Log(const char* szMsg)
{
  LogMessage lm;
  lm.m_sMsg = szMsg;

  m_LogList.PushBack(lm);

  ListAppLog->addItem(szMsg);
}

void ezMainWindow::DockWidgetVisibilityChanged(bool bVisible)
{
  ActionShowWindowLog->setChecked(ezLogWidget::s_pWidget->isVisible());
  ActionShowWindowMemory->setChecked(ezMemoryWidget::s_pWidget->isVisible());
  ActionShowWindowConfig->setChecked(ezGeneralWidget::s_pWidget->isVisible());
  ActionShowWindowInput->setChecked(ezInputWidget::s_pWidget->isVisible());
}

void ezMainWindow::on_ActionShowWindowLog_triggered()
{
  ezLogWidget::s_pWidget->setVisible(ActionShowWindowLog->isChecked());
}

void ezMainWindow::on_ActionShowWindowConfig_triggered()
{
  ezGeneralWidget::s_pWidget->setVisible(ActionShowWindowConfig->isChecked());
}

void ezMainWindow::on_ActionShowWindowMemory_triggered()
{
  ezMemoryWidget::s_pWidget->setVisible(ActionShowWindowMemory->isChecked());
}

void ezMainWindow::on_ActionShowWindowInput_triggered()
{
  ezInputWidget::s_pWidget->setVisible(ActionShowWindowInput->isChecked());
}
