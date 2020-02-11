#include <EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Settings/SettingsTab.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>

EZ_IMPLEMENT_SINGLETON(ezQtSettingsTab);

ezString ezQtSettingsTab::GetWindowIcon() const
{
  return ":/GuiFoundation/Icons/ezEditor16.png";
}

void ezQtEditorApp::ShowSettingsDocument()
{
  ezQtSettingsTab* pSettingsTab = ezQtSettingsTab::GetSingleton();

  if (pSettingsTab == nullptr)
  {
    pSettingsTab = new ezQtSettingsTab();
  }

  pSettingsTab->EnsureVisible();
}

void ezQtEditorApp::CloseSettingsDocument()
{
  ezQtSettingsTab* pSettingsTab = ezQtSettingsTab::GetSingleton();

  if (pSettingsTab != nullptr)
  {
    pSettingsTab->CloseDocumentWindow();
  }
}

ezQtSettingsTab::ezQtSettingsTab()
    : ezQtDocumentWindow("Settings")
    , m_SingletonRegistrar(this)
{
  setCentralWidget(new QWidget());
  EZ_ASSERT_DEV(centralWidget() != nullptr, "");

  setupUi(centralWidget());
  QMetaObject::connectSlotsByName(this);

  ezQtMenuBarActionMapView* pMenuBar = static_cast<ezQtMenuBarActionMapView*>(menuBar());
  ezActionContext context;
  context.m_sMapping = "SettingsTabMenuBar";
  context.m_pDocument = nullptr;
  pMenuBar->SetActionContext(context);

  FinishWindowCreation();

  WhatsNewText->setText(ezQtEditorApp::GetSingleton()->GetWhatsNew().GetText().GetData());
}

ezQtSettingsTab::~ezQtSettingsTab() = default;

bool ezQtSettingsTab::InternalCanCloseWindow()
{
  // if this is the last window, prevent closing it
  return ezQtDocumentWindow::GetAllDocumentWindows().GetCount() > 1;
}

void ezQtSettingsTab::InternalCloseDocumentWindow()
{
  // make sure this instance isn't used anymore
  UnregisterSingleton();
}

