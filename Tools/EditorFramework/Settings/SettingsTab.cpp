#include <PCH.h>
#include <EditorFramework/Settings/SettingsTab.moc.h>
#include <EditorFramework/EditorApp.moc.h>

static ezSettingsTab* g_pInstance = nullptr;

ezSettingsTab* ezSettingsTab::GetInstance()
{
  return g_pInstance;
}

ezSettingsTab::ezSettingsTab() : ezDocumentWindow("Settings")
{
  setCentralWidget(new QWidget());

  m_sSelectedSettingDomain = "<Application>";

  EZ_ASSERT(g_pInstance == nullptr, "");
  EZ_ASSERT(centralWidget() != nullptr, "");

  g_pInstance = this;
  setupUi(centralWidget());

  ezStringBuilder sTitle;
  sTitle.Format("<html><head/><body><p><span style=\" font-size:18pt; font-weight:600;\">%s</span></p></body></html>", ezEditorApp::GetInstance()->GetApplicationName().GetData());
  LabelAppName->setText(QString::fromUtf8(sTitle.GetData()));

  m_pSettingsGrid = new ezSimplePropertyGridWidget(this);
  GroupSettings->layout()->addWidget(m_pSettingsGrid);

  EZ_VERIFY(connect(m_pSettingsGrid, SIGNAL(value_changed()), this, SLOT(SlotSettingsChanged())) != nullptr, "signal/slot connection failed");
  EZ_VERIFY(connect(ButtonPluginConfig, SIGNAL(clicked()), this, SLOT(SlotButtonPluginConfig())) != nullptr, "signal/slot connection failed");
  EZ_VERIFY(connect(ComboSettingsDomain, SIGNAL(currentIndexChanged(int)), this, SLOT(SlotComboSettingsDomainIndexChanged(int))) != nullptr, "signal/slot connection failed");

  ezPlugin::s_PluginEvents.AddEventHandler(ezDelegate<void (const ezPlugin::PluginEvent& e)>(&ezSettingsTab::PluginEventHandler, this));
  ezEditorProject::s_Events.AddEventHandler(ezDelegate<void (const ezEditorProject::Event&)>(&ezSettingsTab::ProjectEventHandler, this));
  ezDocumentManagerBase::s_Events.AddEventHandler(ezDelegate<void (const ezDocumentManagerBase::Event&)>(&ezSettingsTab::DocumentManagerEventHandler, this));

  UpdateSettings();
}

ezSettingsTab::~ezSettingsTab()
{
  ezEditorProject::s_Events.RemoveEventHandler(ezDelegate<void (const ezEditorProject::Event&)>(&ezSettingsTab::ProjectEventHandler, this));
  ezDocumentManagerBase::s_Events.RemoveEventHandler(ezDelegate<void (const ezDocumentManagerBase::Event&)>(&ezSettingsTab::DocumentManagerEventHandler, this));
  ezPlugin::s_PluginEvents.RemoveEventHandler(ezDelegate<void (const ezPlugin::PluginEvent& e)>(&ezSettingsTab::PluginEventHandler, this));

  g_pInstance = nullptr;
}

void ezSettingsTab::PluginEventHandler(const ezPlugin::PluginEvent& e)
{
  switch (e.m_EventType)
  {
  case ezPlugin::PluginEvent::Type::AfterPluginChanges:
    {
      // this is necessary to detect new settings when a plugin has been loaded
      UpdateSettings();
    }
    break;
  }
}

void ezSettingsTab::ProjectEventHandler(const ezEditorProject::Event& e)
{
  switch (e.m_Type)
  {
  case ezEditorProject::Event::Type::ProjectClosed:
  case ezEditorProject::Event::Type::ProjectOpened:
    UpdateSettings();
    break;
  }
}

void ezSettingsTab::DocumentManagerEventHandler(const ezDocumentManagerBase::Event& e)
{
  switch (e.m_Type)
  {
  case ezDocumentManagerBase::Event::Type::DocumentClosed:
  case ezDocumentManagerBase::Event::Type::DocumentOpened:
    UpdateSettings();
    break;
  }
}

void ezSettingsTab::UpdateSettings()
{
  ezStringBuilder sTemp;

  ComboSettingsDomain->blockSignals(true);
  ComboSettingsDomain->clear();

  int iSelected = 0;
  ComboSettingsDomain->addItem("<Application>");

  if (ezEditorProject::IsProjectOpen())
  {
    ComboSettingsDomain->addItem("<Project>");

    if ("<Project>" == m_sSelectedSettingDomain)
      iSelected = 1;

    ezInt32 iIndex = 2;
    for (auto dm : ezDocumentManagerBase::GetAllDocumentManagers())
    {
      for (auto doc : dm->GetAllDocuments())
      {
        ezString sRel;
        if (!ezEditorProject::GetInstance()->IsDocumentInProject(doc->GetDocumentPath(), &sRel))
          continue;

        ComboSettingsDomain->addItem(sRel.GetData());

        if (sRel == m_sSelectedSettingDomain)
          iSelected = iIndex;

        ++iIndex;
      }
    }
  }

  ComboSettingsDomain->setCurrentIndex(iSelected);
  m_sSelectedSettingDomain = ComboSettingsDomain->currentText().toUtf8().data();
  ComboSettingsDomain->blockSignals(false);

  m_pSettingsGrid->BeginProperties();

  for (const ezString& sName : ezEditorApp::GetInstance()->GetRegisteredPluginNamesForSettings())
  {
    ezSettings* s;

    if (m_sSelectedSettingDomain == "<Application>")
      s = &ezEditorApp::GetInstance()->GetEditorSettings(sName);
    else
    if (m_sSelectedSettingDomain == "<Project>")
      s = &ezEditorApp::GetInstance()->GetProjectSettings(sName);
    else
      s = &ezEditorApp::GetInstance()->GetDocumentSettings(m_sSelectedSettingDomain, sName);

    bool bAddedGroupName = false;

    for (auto it = s->GetAllSettings().GetIterator(); it.IsValid(); ++it)
    {
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

bool ezSettingsTab::InternalCanClose()
{
  return true;
}

void ezSettingsTab::InternalCloseDocument()
{
  // make sure this instance isn't used anymore
  g_pInstance = nullptr;
}

void ezSettingsTab::SlotSettingsChanged()
{
}

void ezSettingsTab::SlotButtonPluginConfig()
{
  ezEditorApp::GetInstance()->ShowPluginConfigDialog();
}

void ezSettingsTab::SlotComboSettingsDomainIndexChanged(int iIndex)
{
  m_sSelectedSettingDomain = ComboSettingsDomain->itemText(iIndex).toUtf8().data();

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

