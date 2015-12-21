#include <PCH.h>
#include <EditorFramework/Settings/SettingsTab.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>

static ezSettingsTab* g_pInstance = nullptr;

ezString ezSettingsTab::GetWindowIcon() const
{
  return ":/GuiFoundation/Icons/ezEditor16.png";
}

void ezQtEditorApp::ShowSettingsDocument()
{
  ezSettingsTab* pSettingsTab = ezSettingsTab::GetInstance();

  if (pSettingsTab == nullptr)
  {
    pSettingsTab = new ezSettingsTab();
  }

  pSettingsTab->EnsureVisible();
}

void ezQtEditorApp::CloseSettingsDocument()
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

ezSettingsTab::ezSettingsTab() : ezQtDocumentWindow("")
{
  setCentralWidget(new QWidget());

  EZ_ASSERT_DEV(g_pInstance == nullptr, "");
  EZ_ASSERT_DEV(centralWidget() != nullptr, "");

  g_pInstance = this;
  setupUi(centralWidget());
  QMetaObject::connectSlotsByName(this);

  //EZ_VERIFY(connect(AssetBrowserWidget, SIGNAL(ItemChosen(QString, QString, QString)), this, SLOT(SlotAssetChosen(QString, QString, QString))) != nullptr, "signal/slot connection failed");

  ezMenuBarActionMapView* pMenuBar = static_cast<ezMenuBarActionMapView*>(menuBar());
  ezActionContext context;
  context.m_sMapping = "SettingsTabMenuBar";
  context.m_pDocument = nullptr;
  pMenuBar->SetActionContext(context);

  //AssetBrowserWidget->RestoreState("AssetBrowserPanel");

  FinishWindowCreation();
}

ezSettingsTab::~ezSettingsTab()
{
  //AssetBrowserWidget->SaveState("AssetBrowserPanel");

  g_pInstance = nullptr;
}

bool ezSettingsTab::InternalCanCloseWindow()
{
  // if this is the last window, prevent closing it
  return ezQtDocumentWindow::GetAllDocumentWindows().GetCount() > 1;
}

void ezSettingsTab::InternalCloseDocumentWindow()
{
  // make sure this instance isn't used anymore
  g_pInstance = nullptr;
}

void ezSettingsTab::SlotAssetChosen(QString sAssetGuid, QString sAssetPathRelative, QString sAssetPathAbsolute)
{
  ezQtEditorApp::GetInstance()->OpenDocument(sAssetPathAbsolute.toUtf8().data());
}

