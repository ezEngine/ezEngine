#include <EditorFrameworkPCH.h>

#include <EditorFramework/Dialogs/SnapSettingsDlg.moc.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>

ezQtSnapSettingsDlg::ezQtSnapSettingsDlg(QWidget* parent)
  : QDialog(parent)
{
  setupUi(this);

  m_Translation.PushBack(KeyValue{"Gizmo.Translate.Snap.0", 0.0f});
  m_Translation.PushBack(KeyValue{"Gizmo.Translate.Snap.0_1", 0.1f});
  m_Translation.PushBack(KeyValue{"Gizmo.Translate.Snap.0_125", 0.125f});
  m_Translation.PushBack(KeyValue{"Gizmo.Translate.Snap.0_2", 0.2f});
  m_Translation.PushBack(KeyValue{"Gizmo.Translate.Snap.0_25", 0.25f});
  m_Translation.PushBack(KeyValue{"Gizmo.Translate.Snap.0_5", 0.5f});
  m_Translation.PushBack(KeyValue{"Gizmo.Translate.Snap.1", 1.0f});
  m_Translation.PushBack(KeyValue{"Gizmo.Translate.Snap.5", 5.0f});
  m_Translation.PushBack(KeyValue{"Gizmo.Translate.Snap.10", 10.0f});

  m_Rotation.PushBack(KeyValue{"Gizmo.Rotation.Snap.0_Degree", 0.0f});
  m_Rotation.PushBack(KeyValue{"Gizmo.Rotation.Snap.1_Degree", 1.0f});
  m_Rotation.PushBack(KeyValue{"Gizmo.Rotation.Snap.5_Degree", 5.0f});
  m_Rotation.PushBack(KeyValue{"Gizmo.Rotation.Snap.10_Degree", 10.0f});
  m_Rotation.PushBack(KeyValue{"Gizmo.Rotation.Snap.15_Degree", 15.0f});
  m_Rotation.PushBack(KeyValue{"Gizmo.Rotation.Snap.30_Degree", 30.0f});
  m_Rotation.PushBack(KeyValue{"Gizmo.Rotation.Snap.2_8125_Degree", 2.8125f});
  m_Rotation.PushBack(KeyValue{"Gizmo.Rotation.Snap.5_625_Degree", 5.625f});
  m_Rotation.PushBack(KeyValue{"Gizmo.Rotation.Snap.11_25_Degree", 11.25f});
  m_Rotation.PushBack(KeyValue{"Gizmo.Rotation.Snap.22_5_Degree", 22.5f});
  m_Rotation.PushBack(KeyValue{"Gizmo.Rotation.Snap.45_Degree", 45.0f});

  m_Scale.PushBack(KeyValue{"Gizmo.Scale.Snap.0", 0.0f});
  m_Scale.PushBack(KeyValue{"Gizmo.Scale.Snap.8", 8.0f});
  m_Scale.PushBack(KeyValue{"Gizmo.Scale.Snap.4", 4.0f});
  m_Scale.PushBack(KeyValue{"Gizmo.Scale.Snap.2", 2.0f});
  m_Scale.PushBack(KeyValue{"Gizmo.Scale.Snap.1", 1.0f});
  m_Scale.PushBack(KeyValue{"Gizmo.Scale.Snap.0_5", 0.5f});
  m_Scale.PushBack(KeyValue{"Gizmo.Scale.Snap.0_25", 0.25f});
  m_Scale.PushBack(KeyValue{"Gizmo.Scale.Snap.0_125", 0.125f});

  ezUInt32 uiSelectedT = 0;
  ezUInt32 uiSelectedR = 0;
  ezUInt32 uiSelectedS = 0;

  for (ezUInt32 i = 0; i < m_Translation.GetCount(); ++i)
  {
    TranslationSnap->addItem(ezTranslate(m_Translation[i].m_szKey));

    if (ezSnapProvider::GetTranslationSnapValue() == m_Translation[i].m_fValue)
      uiSelectedT = i;
  }

  for (ezUInt32 i = 0; i < m_Rotation.GetCount(); ++i)
  {
    RotationSnap->addItem(ezTranslate(m_Rotation[i].m_szKey));

    if (ezSnapProvider::GetRotationSnapValue() == ezAngle::Degree(m_Rotation[i].m_fValue))
      uiSelectedR = i;
  }

  for (ezUInt32 i = 0; i < m_Scale.GetCount(); ++i)
  {
    ScaleSnap->addItem(ezTranslate(m_Scale[i].m_szKey));

    if (ezSnapProvider::GetScaleSnapValue() == m_Scale[i].m_fValue)
      uiSelectedS = i;
  }

  TranslationSnap->setCurrentIndex(uiSelectedT);
  RotationSnap->setCurrentIndex(uiSelectedR);
  ScaleSnap->setCurrentIndex(uiSelectedS);
}

void ezQtSnapSettingsDlg::QueryUI()
{
  ezSnapProvider::SetTranslationSnapValue(m_Translation[TranslationSnap->currentIndex()].m_fValue);
  ezSnapProvider::SetRotationSnapValue(ezAngle::Degree(m_Rotation[RotationSnap->currentIndex()].m_fValue));
  ezSnapProvider::SetScaleSnapValue(m_Scale[ScaleSnap->currentIndex()].m_fValue);
}

void ezQtSnapSettingsDlg::on_ButtonBox_clicked(QAbstractButton* button)
{
  if (button == ButtonBox->button(QDialogButtonBox::StandardButton::Ok))
  {
    QueryUI();
    accept();
    return;
  }

  if (button == ButtonBox->button(QDialogButtonBox::StandardButton::Cancel))
  {
    reject();
    return;
  }
}
