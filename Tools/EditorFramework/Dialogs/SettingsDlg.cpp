#include <PCH.h>
#include <EditorFramework/Dialogs/SettingsDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>

SettingsDlg::SettingsDlg(QWidget* parent) : QDialog(parent)
{
  setupUi(this);

  m_sSelectedSettingDomain = "<Application>";

  m_pSettingsGrid = new ezQtSimplePropertyGridWidget(this);
  GroupSettings->layout()->addWidget(m_pSettingsGrid);

  EZ_VERIFY(connect(m_pSettingsGrid, SIGNAL(value_changed()), this, SLOT(SlotSettingsChanged())) != nullptr, "signal/slot connection failed");
  EZ_VERIFY(connect(ComboSettingsDomain, SIGNAL(currentIndexChanged(int)), this, SLOT(SlotComboSettingsDomainIndexChanged(int))) != nullptr, "signal/slot connection failed");

  UpdateSettings();
}

void SettingsDlg::SlotSettingsChanged()
{
  int i = 0;
  /// \todo This is not implemented: Cannot modify stored settings
}

void SettingsDlg::SlotComboSettingsDomainIndexChanged(int iIndex)
{
  m_sSelectedSettingDomain = ComboSettingsDomain->itemData(iIndex, Qt::UserRole).toString().toUtf8().data();

  UpdateSettings();

  if (iIndex == 0) // Application
  {
  }
  else if (iIndex == 1) // Project
  {
  }
  else
  {
    ComboSettingsDomain->itemText(iIndex);
  }
}


void SettingsDlg::UpdateSettings()
{
  ezStringBuilder sTemp;

  ComboSettingsDomain->blockSignals(true);
  ComboSettingsDomain->clear();

  int iSelected = 0;
  ComboSettingsDomain->addItem("<Application>", QString("<Application>"));

  if (ezToolsProject::IsProjectOpen())
  {
    ComboSettingsDomain->addItem("<Project>", QString("<Project>"));

    if ("<Project>" == m_sSelectedSettingDomain)
      iSelected = 1;

    ezInt32 iIndex = 2;
    for (auto dm : ezDocumentManager::GetAllDocumentManagers())
    {
      for (auto doc : dm->GetAllDocuments())
      {
        ezString sRel;
        if (!ezToolsProject::GetInstance()->IsDocumentInAllowedRoot(doc->GetDocumentPath(), &sRel))
          continue;

        ComboSettingsDomain->addItem(sRel.GetData(), QString(doc->GetDocumentPath()));

        if (doc->GetDocumentPath() == m_sSelectedSettingDomain)
          iSelected = iIndex;

        ++iIndex;
      }
    }
  }

  ComboSettingsDomain->setCurrentIndex(iSelected);
  m_sSelectedSettingDomain = ComboSettingsDomain->itemData(iSelected, Qt::UserRole).toString().toUtf8().data();
  ComboSettingsDomain->blockSignals(false);

  m_pSettingsGrid->BeginProperties();

  for (const ezString& sName : ezQtEditorApp::GetInstance()->GetRegisteredPluginNamesForSettings())
  {
    ezSettings* s;

    if (m_sSelectedSettingDomain == "<Application>")
      s = &ezQtEditorApp::GetInstance()->GetEditorSettings(sName);
    else
      if (m_sSelectedSettingDomain == "<Project>")
        s = &ezQtEditorApp::GetInstance()->GetProjectSettings(sName);
      else
        s = &ezQtEditorApp::GetInstance()->GetDocumentSettings(m_sSelectedSettingDomain, sName);

    bool bAddedGroupName = false;

    for (auto it = s->GetAllSettings().GetIterator(); it.IsValid(); ++it)
    {
      if (!it.Value().m_Flags.IsAnySet(ezSettingsFlags::Registered))
        continue;
      if (it.Value().m_Flags.IsAnySet(ezSettingsFlags::Hidden))
        continue;

      if (!bAddedGroupName)
      {
        m_pSettingsGrid->AddProperty("Data Group:", sName, nullptr, true);
        bAddedGroupName = true;
      }

      m_pSettingsGrid->AddProperty(it.Key(), it.Value().m_Value, &it.Value().m_Value, it.Value().m_Flags.IsAnySet(ezSettingsFlags::ReadOnly));
    }
  }

  m_pSettingsGrid->EndProperties();
}