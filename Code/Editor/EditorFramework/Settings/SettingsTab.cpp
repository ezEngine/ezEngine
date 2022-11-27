#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetBrowserDlg.moc.h>
#include <EditorFramework/Dialogs/DashboardDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Settings/SettingsTab.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <QDesktopServices>

EZ_IMPLEMENT_SINGLETON(ezQtSettingsTab);

ezString ezQtSettingsTab::GetWindowIcon() const
{
  return ""; //:/GuiFoundation/EZ-logo.svg";
}

ezString ezQtSettingsTab::GetDisplayNameShort() const
{
  return "";
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

  ezToolsProject::s_Events.AddEventHandler(ezMakeDelegate(&ezQtSettingsTab::ToolsProjectEventHandler, this));
}

ezQtSettingsTab::~ezQtSettingsTab()
{
  ezToolsProject::s_Events.RemoveEventHandler(ezMakeDelegate(&ezQtSettingsTab::ToolsProjectEventHandler, this));
}

void ezQtSettingsTab::on_OpenScene_clicked()
{
  ezQtAssetBrowserDlg dlg(this, ezUuid(), "Scene");
  if (dlg.exec() == 0)
    return;

  ezQtEditorApp::GetSingleton()->OpenDocument(dlg.GetSelectedAssetPathAbsolute(), ezDocumentFlags::RequestWindow | ezDocumentFlags::AddToRecentFilesList);
}

void ezQtSettingsTab::on_OpenProject_clicked()
{
  ezQtDashboardDlg dlg(nullptr, ezQtDashboardDlg::DashboardTab::Samples);
  dlg.exec();
}

void ezQtSettingsTab::on_GettingStarted_clicked()
{
  QDesktopServices::openUrl(QUrl("https://ezengine.net/pages/getting-started/editor-overview.html"));
}

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

void ezQtSettingsTab::ToolsProjectEventHandler(const ezToolsProjectEvent& e)
{
  if (e.m_Type == ezToolsProjectEvent::Type::ProjectClosed || e.m_Type == ezToolsProjectEvent::Type::ProjectCreated || e.m_Type == ezToolsProjectEvent::Type::ProjectOpened)
  {
    ezStringBuilder txt = "<html><head/><body><p align=\"center\"><span style=\" font-size:18pt;\">Open Project:</span></p><p align=\"center\"><span style=\" font-size:18pt;\">None</span></p></body></html>";

    if (ezToolsProject::GetSingleton()->IsProjectOpen())
    {
      txt.ReplaceAll("None", ezToolsProject::GetSingleton()->GetProjectName(false));
      OpenScene->setVisible(true);
    }
    else
    {
      txt = "<html><head/><body><p align=\"center\"><span style=\" font-size:18pt;\">No Project Open</span></p></body></html>";
      OpenScene->setVisible(false);
    }

    ProjectLabel->setText(txt.GetData());
  }
}
