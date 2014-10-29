#include <PCH.h>
#include <EditorFramework/EditorGUI.moc.h>
#include <QSettings>

ezEditorGUI* ezEditorGUI::GetInstance()
{
  static ezEditorGUI instance;
  return &instance;
}

ezEditorGUI::ezEditorGUI()
{
  m_pColorDlg = nullptr;
}

void ezEditorGUI::SaveState()
{
  QSettings Settings;
  Settings.beginGroup("EditorGUI");
  {
    Settings.setValue("ColorDlgPos", m_ColorDlgPos);
  }
  Settings.endGroup();
}

void ezEditorGUI::LoadState()
{
  QSettings Settings;
  Settings.beginGroup("EditorGUI");
  {
    m_ColorDlgPos = Settings.value("ColorDlgPos", QPoint(100, 100)).toPoint();
  }
  Settings.endGroup();
}
