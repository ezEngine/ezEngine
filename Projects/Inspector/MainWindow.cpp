#include <Inspector/MainWindow.moc.h>
#include <Inspector/GeneralWidget.moc.h>
#include <Inspector/MemoryWidget.moc.h>
#include <Inspector/LogWidget.moc.h>
#include <Foundation/Communication/Telemetry.h>
#include <qlistwidget.h>
#include <qinputdialog.h>
#include <qfile.h>

ezMainWindow::ezMainWindow() : QMainWindow()
{
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
  if (ezLogWidget::s_pWidget)
    ezLogWidget::s_pWidget->UpdateStats();

  if (ezGeneralWidget::s_pWidget)
    ezGeneralWidget::s_pWidget->UpdateStats();

  if (ezMemoryWidget::s_pWidget)
    ezMemoryWidget::s_pWidget->UpdateStats();

  ezTelemetry::CallProcessMessagesCallbacks();

  update();
}

