#include <GuiFoundation/PCH.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QSettings>


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


