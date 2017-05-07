#include <PCH.h>
#include <EditorPluginFmod/Dialogs/FmodProjectSettingsDlg.moc.h>
#include <GuiFoundation/Basics.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <ToolsFoundation/Project/ToolsProject.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <QInputDialog>
#include <QFileDialog>

ezQtFmodProjectSettingsDlg::ezQtFmodProjectSettingsDlg(QWidget* parent) : QDialog(parent)
{
  setupUi(this);
  
  Load();

  for (auto it = m_Configs.m_PlatformConfigs.GetIterator(); it.IsValid(); ++it)
  {
    ListPlatforms->addItem(it.Key().GetData());
  }

  if (!m_Configs.m_PlatformConfigs.IsEmpty())
  {
    if (m_Configs.m_PlatformConfigs.Contains("Desktop"))
      SetCurrentPlatform("Desktop");
    else
      SetCurrentPlatform(m_Configs.m_PlatformConfigs.GetIterator().Key());
  }
  else
  {
    SetCurrentPlatform("");
  }
}
ezResult ezQtFmodProjectSettingsDlg::Save()
{
  const char* szFile = ":project/FmodConfig.ddl";
  if (m_Configs.Save(szFile).Failed())
  {
    ezStringBuilder sError;
    sError.Format("Failed to save the Fmod configuration file\n'{0}'", szFile);

    ezQtUiServices::GetSingleton()->MessageBoxWarning(sError);

    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

void ezQtFmodProjectSettingsDlg::Load()
{
  m_Configs.Load(":project/FmodConfig.ddl");

  m_ConfigsOld = m_Configs;
}

void ezQtFmodProjectSettingsDlg::SetCurrentPlatform(const char* szPlatform)
{
  StoreCurrentPlatform();

  m_sCurrentPlatform = szPlatform;

  {
    const bool enable = !m_sCurrentPlatform.IsEmpty();
    EditMasterBank->setEnabled(enable);
    ButtonMB->setEnabled(enable);
    ComboMode->setEnabled(enable);
    SpinChannels->setEnabled(enable);
    SpinSamplerRate->setEnabled(enable);
    ButtonRemove->setEnabled(enable);
  }

  ezQtScopedBlockSignals bs(ListPlatforms);
  QList<QListWidgetItem*> items = ListPlatforms->findItems(szPlatform, Qt::MatchFlag::MatchExactly);

  ListPlatforms->clearSelection();

  if (items.size() > 0)
  {
    items[0]->setSelected(true);
  }

  m_sCurrentPlatform = szPlatform;

  if (m_sCurrentPlatform.IsEmpty())
    return;

  const auto& cfg = m_Configs.m_PlatformConfigs[m_sCurrentPlatform];

  EditMasterBank->setText(cfg.m_sMasterSoundBank.GetData());
  SpinChannels->setValue(cfg.m_uiVirtualChannels);
  SpinSamplerRate->setValue(cfg.m_uiSamplerRate);

  switch (cfg.m_SpeakerMode)
  {
  case ezFmodSpeakerMode::ModeStereo:
    ComboMode->setCurrentIndex(0);
    break;
  case ezFmodSpeakerMode::Mode7Point1:
    ComboMode->setCurrentIndex(2);
  case ezFmodSpeakerMode::Mode5Point1:
  default:
    ComboMode->setCurrentIndex(1);
    break;
  }
}

void ezQtFmodProjectSettingsDlg::StoreCurrentPlatform()
{
  if (!m_Configs.m_PlatformConfigs.Contains(m_sCurrentPlatform))
    return;

  auto& cfg = m_Configs.m_PlatformConfigs[m_sCurrentPlatform];

  cfg.m_sMasterSoundBank = EditMasterBank->text().toUtf8().data();
  cfg.m_uiVirtualChannels = SpinChannels->value();
  cfg.m_uiSamplerRate = SpinSamplerRate->value();

  if (cfg.m_uiSamplerRate < 8000)
    cfg.m_uiSamplerRate = 0;

  switch (ComboMode->currentIndex())
  {
  case 0:
    cfg.m_SpeakerMode = ezFmodSpeakerMode::ModeStereo;
    break;
  case 1:
    cfg.m_SpeakerMode = ezFmodSpeakerMode::Mode5Point1;
    break;
  case 2:
    cfg.m_SpeakerMode = ezFmodSpeakerMode::Mode7Point1;
    break;
  }
}

void ezQtFmodProjectSettingsDlg::on_ButtonBox_clicked(QAbstractButton* pButton)
{
  if (pButton == ButtonBox->button(QDialogButtonBox::Ok))
  {
    StoreCurrentPlatform();

    if (m_ConfigsOld.m_PlatformConfigs != m_Configs.m_PlatformConfigs)
    {
      if (ezQtUiServices::GetSingleton()->MessageBoxQuestion("Save the changes to the Fmod configuration?\nYou need to reload the project for the changes to take effect.", QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes)
      {
        ezQtEditorApp::GetSingleton()->AddReloadProjectRequiredReason("Fmod configuration was modified.");

        if (Save().Failed())
          return;
      }
    }

    accept();
    return;
  }

  if (pButton == ButtonBox->button(QDialogButtonBox::Cancel))
  {
    reject();
    return;
  }
}

void ezQtFmodProjectSettingsDlg::on_ListPlatforms_itemSelectionChanged()
{
  if (ListPlatforms->selectedItems().isEmpty())
  {
    SetCurrentPlatform("");
    return;
  }

  ButtonRemove->setEnabled(true);
  int row = ListPlatforms->selectionModel()->selectedIndexes()[0].row();
  SetCurrentPlatform(ListPlatforms->item(row)->text().toUtf8().data());
}

void ezQtFmodProjectSettingsDlg::on_ButtonAdd_clicked()
{
  QString name = QInputDialog::getText(this, "Add Platform", "Platform Name:");

  if (name.isEmpty())
    return;

  const ezString sName = name.toUtf8().data();

  if (!m_Configs.m_PlatformConfigs.Contains(sName))
  {
    // add a new item with default values
    m_Configs.m_PlatformConfigs[sName];

    ListPlatforms->addItem(sName.GetData());
  }
  
  SetCurrentPlatform(sName);
}

void ezQtFmodProjectSettingsDlg::on_ButtonRemove_clicked()
{
  if (ListPlatforms->selectedItems().isEmpty())
    return;

  int row = ListPlatforms->selectionModel()->selectedIndexes()[0].row();
  const ezString sPlatform = ListPlatforms->item(row)->text().toUtf8().data();

  m_Configs.m_PlatformConfigs.Remove(sPlatform);
  delete ListPlatforms->item(row);
}

void ezQtFmodProjectSettingsDlg::on_ButtonMB_clicked()
{
  static QString sLastPath = ezToolsProject::GetSingleton()->GetProjectDirectory().GetData();
  const QString sFile = QFileDialog::getOpenFileName(this, QLatin1String("Select Master Sound Bank"), sLastPath, "Sound Banks (*.bank)", nullptr, QFileDialog::Option::DontResolveSymlinks);

  if (sFile.isEmpty())
    return;

  ezStringBuilder sRelative = sFile.toUtf8().data();

  if (!ezQtEditorApp::GetSingleton()->MakePathDataDirectoryRelative(sRelative))
    return;

  sLastPath = sFile;

  if (sRelative.EndsWith_NoCase(".strings.bank"))
  {
    ezQtUiServices::GetSingleton()->MessageBoxWarning("The strings sound bank cannot be selected as the master sound bank.");
    return;
  }

  EditMasterBank->setText(sRelative.GetData());
}

