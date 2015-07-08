#include <PCH.h>
#include <EditorFramework/Settings/SettingsTab.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Dialogs/DataDirsDlg.moc.h>
#include <EditorFramework/Dialogs/PluginDlg.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <EditorFramework/Assets/AssetBrowserDlg.moc.h>

static ezSettingsTab* g_pInstance = nullptr;

ezString ezSettingsTab::GetWindowIcon() const
{
  return ":/GuiFoundation/Icons/Settings16.png";
}

void ezEditorApp::ShowSettingsDocument()
{
  ezSettingsTab* pSettingsTab = ezSettingsTab::GetInstance();

  if (pSettingsTab == nullptr)
  {
    pSettingsTab = new ezSettingsTab();
  }

  pSettingsTab->EnsureVisible();
}

void ezEditorApp::CloseSettingsDocument()
{
  ezSettingsTab* pSettingsTab = ezSettingsTab::GetInstance();

  if (pSettingsTab != nullptr)
  {
    pSettingsTab->CloseDocumentWindow();
  }
}

ezSettingsTab* ezSettingsTab::GetInstance()
{
  return g_pInstance;
}

ezSettingsTab::ezSettingsTab() : ezDocumentWindow("Settings")
{
  setCentralWidget(new QWidget());

  m_sSelectedSettingDomain = "<Application>";

  EZ_ASSERT_DEV(g_pInstance == nullptr, "");
  EZ_ASSERT_DEV(centralWidget() != nullptr, "");

  g_pInstance = this;
  setupUi(centralWidget());
  QMetaObject::connectSlotsByName(this);

  m_pSettingsGrid = new ezSimplePropertyGridWidget(this);
  GroupSettings->layout()->addWidget(m_pSettingsGrid);

  EZ_VERIFY(connect(m_pSettingsGrid, SIGNAL(value_changed()), this, SLOT(SlotSettingsChanged())) != nullptr, "signal/slot connection failed");
  EZ_VERIFY(connect(ComboSettingsDomain, SIGNAL(currentIndexChanged(int)), this, SLOT(SlotComboSettingsDomainIndexChanged(int))) != nullptr, "signal/slot connection failed");
  EZ_VERIFY(connect(AssetBrowserWidget, SIGNAL(ItemChosen(QString, QString, QString)), this, SLOT(SlotAssetChosen(QString, QString, QString))) != nullptr, "signal/slot connection failed");

  ezPlugin::s_PluginEvents.AddEventHandler(ezMakeDelegate(&ezSettingsTab::PluginEventHandler, this));
  ezToolsProject::s_Events.AddEventHandler(ezMakeDelegate(&ezSettingsTab::ProjectEventHandler, this));
  ezDocumentManagerBase::s_Events.AddEventHandler(ezMakeDelegate(&ezSettingsTab::DocumentManagerEventHandler, this));

  UpdateSettings();

  ezMenuBarActionMapView* pMenuBar = static_cast<ezMenuBarActionMapView*>(menuBar());
  ezActionContext context;
  context.m_sMapping = "SettingsTabMenuBar";
  context.m_pDocument = nullptr;
  pMenuBar->SetActionContext(context);

  AssetBrowserWidget->RestoreState("AssetBrowserPanel");
}

ezSettingsTab::~ezSettingsTab()
{
  AssetBrowserWidget->SaveState("AssetBrowserPanel");

  ezToolsProject::s_Events.RemoveEventHandler(ezMakeDelegate(&ezSettingsTab::ProjectEventHandler, this));
  ezDocumentManagerBase::s_Events.RemoveEventHandler(ezMakeDelegate(&ezSettingsTab::DocumentManagerEventHandler, this));
  ezPlugin::s_PluginEvents.RemoveEventHandler(ezMakeDelegate(&ezSettingsTab::PluginEventHandler, this));

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

void ezSettingsTab::ProjectEventHandler(const ezToolsProject::Event& e)
{
  switch (e.m_Type)
  {
  case ezToolsProject::Event::Type::ProjectClosed:
  case ezToolsProject::Event::Type::ProjectOpened:
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

  GroupProject->setEnabled(ezToolsProject::IsProjectOpen());

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
    for (auto dm : ezDocumentManagerBase::GetAllDocumentManagers())
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

bool ezSettingsTab::InternalCanCloseWindow()
{
  // if this is the last window, prevent closing it
  return ezDocumentWindow::GetAllDocumentWindows().GetCount() > 1;
}

void ezSettingsTab::InternalCloseDocumentWindow()
{
  // make sure this instance isn't used anymore
  g_pInstance = nullptr;
}

void ezSettingsTab::SlotSettingsChanged()
{
}

void ezSettingsTab::on_ButtonPluginConfig_clicked()
{
  PluginDlg dlg(nullptr);
  dlg.exec();
}

void ezSettingsTab::SlotAssetChosen(QString sAssetGuid, QString sAssetPathRelative, QString sAssetPathAbsolute)
{
  ezEditorApp::GetInstance()->OpenDocument(sAssetPathAbsolute.toUtf8().data());
}

void ezSettingsTab::SlotComboSettingsDomainIndexChanged(int iIndex)
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

void ezSettingsTab::on_ButtonDataDirConfig_clicked()
{
  DataDirsDlg dlg(this);
  dlg.exec();
}


