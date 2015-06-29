#include <GuiFoundation/PCH.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QSettings>
#include <QProcess>
#include <QDir>

ezEvent<const ezUIServices::Event&> ezUIServices::s_Events;

ezUIServices* ezUIServices::GetInstance()
{
  static ezUIServices instance;
  return &instance;
}

ezUIServices::ezUIServices()
{
  m_pColorDlg = nullptr;
}

void ezUIServices::SaveState()
{
  QSettings Settings;
  Settings.beginGroup("EditorGUI");
  {
    Settings.setValue("ColorDlgPos", m_ColorDlgPos);
  }
  Settings.endGroup();
}

void ezUIServices::LoadState()
{
  QSettings Settings;
  Settings.beginGroup("EditorGUI");
  {
    m_ColorDlgPos = Settings.value("ColorDlgPos", QPoint(100, 100)).toPoint();
  }
  Settings.endGroup();
}

void ezUIServices::ShowAllDocumentsStatusBarMessage(const char* szMsg, ezTime timeOut)
{
  Event e;
  e.m_Type = Event::ShowDocumentStatusBarText;
  e.m_sText = szMsg;
  e.m_Time = timeOut;

  s_Events.Broadcast(e);
}

void ezUIServices::ShowGlobalStatusBarMessage(const char* szMsg)
{
  Event e;
  e.m_Type = Event::ShowGlobalStatusBarText;
  e.m_sText = szMsg;
  e.m_Time = ezTime::Seconds(0);

  s_Events.Broadcast(e);
}

void ezUIServices::OpenInExplorer(const char* szPath)
{
  QStringList args;
  args << "/select," << QDir::toNativeSeparators(szPath);
  QProcess::startDetached("explorer", args);
}

