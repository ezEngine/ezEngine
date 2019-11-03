#include <GuiFoundationPCH.h>

#include <Foundation/Types/ScopeExit.h>
#include <GuiFoundation/ContainerWindow/ContainerWindow.moc.h>
#include <GuiFoundation/DockPanels/ApplicationPanel.moc.h>
#include <QCloseEvent>
#include <QLabel>
#include <QSettings>
#include <QStatusBar>
#include <QTabBar>
#include <QTimer>
#include <ToolsFoundation/Application/ApplicationServices.h>
#include <ads/DockManager.h>
#include <ads/DockWidgetTab.h>
#include <ads/FloatingDockContainer.h>

ezQtContainerWindow* ezQtContainerWindow::s_pContainerWindow = nullptr;
bool ezQtContainerWindow::s_bForceClose = false;

namespace
{
  bool GetProjectLayoutPath(ezStringBuilder& out_sFile, bool bWrite)
  {
    if (!ezToolsProject::IsProjectOpen())
    {
      out_sFile.Clear();
      return false;
    }
    out_sFile = ezApplicationServices::GetSingleton()->GetProjectPreferencesFolder();
    out_sFile.AppendPath("layout.settings");
    if (!bWrite && !QFile::exists(out_sFile.GetData()))
    {
      out_sFile.Clear();
      return false;
    }
    return true;
  }

  bool GetApplicationLayoutPath(ezStringBuilder& out_sFile, bool bWrite)
  {
    out_sFile = ezApplicationServices::GetSingleton()->GetApplicationPreferencesFolder();
    out_sFile.AppendPath("layout.settings");
    if (!bWrite && !QFile::exists(out_sFile.GetData()))
    {
      out_sFile.Clear();
      return false;
    }
    return true;
  }
} // namespace

ezQtContainerWindow::ezQtContainerWindow()
{
  setMinimumSize(QSize(800, 600));
  m_bWindowLayoutRestored = false;
  m_pStatusBarLabel = nullptr;
  m_iWindowLayoutRestoreScheduled = 0;

  s_pContainerWindow = this;

  setObjectName("ezEditor");
  setWindowIcon(QIcon(QStringLiteral(":/GuiFoundation/Icons/ezEditor16.png")));

  ezQtDocumentWindow::s_Events.AddEventHandler(ezMakeDelegate(&ezQtContainerWindow::DocumentWindowEventHandler, this));
  ezToolsProject::s_Events.AddEventHandler(ezMakeDelegate(&ezQtContainerWindow::ProjectEventHandler, this));
  ezQtUiServices::s_Events.AddEventHandler(ezMakeDelegate(&ezQtContainerWindow::UIServicesEventHandler, this));

  UpdateWindowTitle();

  m_DockManager = new ads::CDockManager(this);
  m_DockManager->setConfigFlags(static_cast<ads::CDockManager::ConfigFlags>(
    ads::CDockManager::DockAreaHasCloseButton |
    ads::CDockManager::DockAreaCloseButtonClosesTab |
    ads::CDockManager::OpaqueSplitterResize |
    ads::CDockManager::AllTabsHaveCloseButton));

  connect(m_DockManager, &ads::CDockManager::floatingWidgetOpened, this, &ezQtContainerWindow::SlotFloatingWidgetOpened);
}

ezQtContainerWindow::~ezQtContainerWindow()
{
  s_pContainerWindow = nullptr;

  ezQtDocumentWindow::s_Events.RemoveEventHandler(ezMakeDelegate(&ezQtContainerWindow::DocumentWindowEventHandler, this));
  ezToolsProject::s_Events.RemoveEventHandler(ezMakeDelegate(&ezQtContainerWindow::ProjectEventHandler, this));
  ezQtUiServices::s_Events.RemoveEventHandler(ezMakeDelegate(&ezQtContainerWindow::UIServicesEventHandler, this));

  // The dock manager does not take ownership of dock widgets.
  auto dockWidgets = m_DockManager->dockWidgetsMap();
  for (auto it = dockWidgets.begin(); it != dockWidgets.end(); ++it)
  {
    m_DockManager->removeDockWidget(it.value());
    delete it.value();
  }
}

void ezQtContainerWindow::UpdateWindowTitle()
{
  ezStringBuilder sTitle;

  if (ezToolsProject::IsProjectOpen())
  {
    ezStringBuilder sTemp = ezToolsProject::GetSingleton()->GetProjectFile();
    sTemp.PathParentDirectory();
    sTemp.Trim("/");

    sTitle = sTemp.GetFileName();
    sTitle.Append(" - ");
  }

  sTitle.Append(ezApplicationServices::GetSingleton()->GetApplicationName());

  setWindowTitle(QString::fromUtf8(sTitle.GetData()));
}

void ezQtContainerWindow::ScheduleRestoreWindowLayout()
{
  m_iWindowLayoutRestoreScheduled++;
  QTimer::singleShot(0, this, SLOT(SlotRestoreLayout()));
}

void ezQtContainerWindow::SlotRestoreLayout()
{
  RestoreWindowLayout();
}

void ezQtContainerWindow::closeEvent(QCloseEvent* e)
{
  SaveWindowLayout();
  SaveDocumentLayouts();

  if (s_bForceClose)
    return;

  s_bForceClose = true;
  EZ_SCOPE_EXIT(s_bForceClose = false);

  e->setAccepted(true);

  if (!ezToolsProject::CanCloseProject())
  {
    e->setAccepted(false);
    return;
  }

  // do not close the documents in the main container window here,
  // as that would remove them from the recently-open documents list and not restore them when opening the editor again
  ezDynamicArray<ezQtDocumentWindow*> windows = m_DocumentWindows;
  for (ezQtDocumentWindow* pWindow : windows)
  {
    pWindow->DisableWindowLayoutSaving();
  }
}

void ezQtContainerWindow::SaveWindowLayout()
{
  ezStringBuilder sFile;
  GetApplicationLayoutPath(sFile, true);

  ezStringBuilder sProjectFile;
  GetProjectLayoutPath(sProjectFile, true);

  const bool bMaximized = isMaximized();

  QSettings Settings(ezToolsProject::IsProjectOpen() ? sProjectFile.GetData() : sFile.GetData(), QSettings::IniFormat);
  Settings.beginGroup(QString::fromUtf8("ContainerWnd_ezEditor"));
  {
    Settings.setValue("DockManagerState", m_DockManager->saveState(1));
    Settings.setValue("WindowGeometry", saveGeometry());
    Settings.setValue("WindowState", saveState());
  }
  Settings.endGroup();

  if (ezToolsProject::IsProjectOpen())
  {
    // The last open project always serves as the default layout in case
    // a new project is created or a project without layout data is opened.
    QFile::remove(sFile.GetData());
    QFile::copy(sProjectFile.GetData(), sFile.GetData());
  }
}

void ezQtContainerWindow::SaveDocumentLayouts()
{
  for (ezUInt32 i = 0; i < m_DocumentWindows.GetCount(); ++i)
    m_DocumentWindows[i]->SaveWindowLayout();
}

void ezQtContainerWindow::RestoreWindowLayout()
{
  --m_iWindowLayoutRestoreScheduled;
  if (m_iWindowLayoutRestoreScheduled > 0)
    return;

  show();

  ezStringBuilder sFile;
  if (!GetProjectLayoutPath(sFile, false))
  {
    if (!GetApplicationLayoutPath(sFile, false))
    {
      // No project or app settings file found, exiting.
      return;
    }
  }

  {
    QSettings Settings(sFile.GetData(), QSettings::IniFormat);
    Settings.beginGroup(QString::fromUtf8("ContainerWnd_ezEditor"));
    {
      restoreGeometry(Settings.value("WindowGeometry", saveGeometry()).toByteArray());
      restoreState(Settings.value("WindowState", saveState()).toByteArray());
      auto dockState = Settings.value("DockManagerState");
      if (dockState.isValid() && dockState.type() == QVariant::ByteArray)
      {
        m_DockManager->restoreState(dockState.toByteArray(), 1);
        // As document windows can't be in a closed state (as pressing x destroys them),
        // we need to fix any document window that was accidentally saved in its closed state.
        for (ads::CDockWidget* dock : m_DocumentDocks)
        {
          if (dock->isClosed())
          {
            dock->toggleView();
          }
        }
      }
    }
    Settings.endGroup();
  }

  for (ezUInt32 i = 0; i < m_DocumentWindows.GetCount(); ++i)
    m_DocumentWindows[i]->RestoreWindowLayout();

  m_bWindowLayoutRestored = true;
}

void ezQtContainerWindow::SlotUpdateWindowDecoration(void* pDocWindow)
{
  UpdateWindowDecoration(static_cast<ezQtDocumentWindow*>(pDocWindow));
}

void ezQtContainerWindow::SlotFloatingWidgetOpened(ads::CFloatingDockContainer* FloatingWidget)
{
  FloatingWidget->installEventFilter(this);
}

void ezQtContainerWindow::UpdateWindowDecoration(ezQtDocumentWindow* pDocWindow)
{
  const ezUInt32 uiListIndex = m_DocumentWindows.IndexOf(pDocWindow);
  if (uiListIndex == ezInvalidIndex)
    return;

  ads::CDockWidget* dock = m_DocumentDocks[uiListIndex];

  dock->setTabToolTip(QString::fromUtf8(pDocWindow->GetDisplayName().GetData()));
  dock->setIcon(ezQtUiServices::GetCachedIconResource(pDocWindow->GetWindowIcon().GetData()));
  dock->setWindowTitle(QString::fromUtf8(pDocWindow->GetDisplayNameShort().GetData()));
}

void ezQtContainerWindow::RemoveDocumentWindow(ezQtDocumentWindow* pDocWindow)
{
  const ezUInt32 uiListIndex = m_DocumentWindows.IndexOf(pDocWindow);
  if (uiListIndex == ezInvalidIndex)
    return;

  ads::CDockWidget* dock = m_DocumentDocks[uiListIndex];
  m_DockManager->removeDockWidget(dock);

  m_DocumentWindows.RemoveAtAndSwap(uiListIndex);
  m_DocumentDocks.RemoveAtAndSwap(uiListIndex);
  EZ_ASSERT_DEV(m_DockNames.contains(dock->objectName()), "Object name must not change during lifetime.");
  m_DockNames.remove(dock->objectName());
  dock->hide();
  dock->deleteLater();
  pDocWindow->m_pContainerWindow = nullptr;
}

void ezQtContainerWindow::RemoveApplicationPanel(ezQtApplicationPanel* pPanel)
{
  const auto uiListIndex = m_ApplicationPanels.IndexOf(pPanel);

  if (uiListIndex == ezInvalidIndex)
    return;

  m_DockManager->removeDockWidget(pPanel);
  m_ApplicationPanels.RemoveAtAndSwap(uiListIndex);

  pPanel->m_pContainerWindow = nullptr;
}

void ezQtContainerWindow::AddDocumentWindow(ezQtDocumentWindow* pDocWindow)
{
  EZ_ASSERT_DEV(!pDocWindow->objectName().isEmpty(), "Panel name must be unique and not empty.");

  if (m_DocumentWindows.IndexOf(pDocWindow) != ezInvalidIndex)
    return;

  EZ_ASSERT_DEV(pDocWindow->m_pContainerWindow == nullptr, "Implementation error");

  m_DocumentWindows.PushBack(pDocWindow);
  ads::CDockWidget* dock = new ads::CDockWidget(QString::fromUtf8(pDocWindow->GetDisplayNameShort()));
  EZ_ASSERT_DEV(!dock->objectName().isEmpty(), "Dock name must not be empty.");
  EZ_ASSERT_DEV(!m_DockNames.contains(dock->objectName()), "Dock name must be unique.");
  m_DockNames.insert(dock->objectName());
  dock->setWidget(pDocWindow);
  dock->tabWidget()->setContextMenuPolicy(Qt::CustomContextMenu);
  if (!m_DocumentDocks.IsEmpty())
  {
    ads::CDockAreaWidget* dockArea = m_DocumentDocks.PeekBack()->dockAreaWidget();
    m_DockManager->addDockWidgetTabToArea(dock, dockArea);
  }
  else
  {
    m_DockManager->addDockWidgetTab(ads::LeftDockWidgetArea, dock);
  }
  m_DocumentDocks.PushBack(dock);
  connect(dock, &ads::CDockWidget::closed, this, &ezQtContainerWindow::SlotDocumentTabCloseRequested);
  connect(dock->tabWidget(), &QWidget::customContextMenuRequested, this, &ezQtContainerWindow::SlotTabsContextMenuRequested);



  pDocWindow->m_pContainerWindow = this;

  // we cannot call virtual functions on pDocWindow here, because the object might still be under construction
  // so we delay it until later
  QMetaObject::invokeMethod(this, "SlotUpdateWindowDecoration", Qt::ConnectionType::QueuedConnection, Q_ARG(void*, pDocWindow));
}

void ezQtContainerWindow::AddApplicationPanel(ezQtApplicationPanel* pPanel)
{
  // panel already in container window ?
  if (m_ApplicationPanels.IndexOf(pPanel) != ezInvalidIndex)
    return;

  EZ_ASSERT_DEV(!pPanel->objectName().isEmpty(), "Dock name must not be empty.");
  EZ_ASSERT_DEV(!m_DockNames.contains(pPanel->objectName()), "Dock name must be unique.");
  m_DockNames.insert(pPanel->objectName());
  EZ_ASSERT_DEV(pPanel->m_pContainerWindow == nullptr, "Implementation error");

  m_ApplicationPanels.PushBack(pPanel);
  pPanel->m_pContainerWindow = this;
  m_DockManager->addDockWidgetTab(ads::RightDockWidgetArea, pPanel);
}

ezResult ezQtContainerWindow::EnsureVisible(ezQtDocumentWindow* pDocWindow)
{
  const auto uiListIndex = m_DocumentWindows.IndexOf(pDocWindow);

  if (uiListIndex == ezInvalidIndex)
    return EZ_FAILURE;

  ads::CDockWidget* dock = m_DocumentDocks[uiListIndex];

  if (dock->isClosed())
  {
    dock->toggleView();
  }
  dock->raise();
  return EZ_SUCCESS;
}

ezResult ezQtContainerWindow::EnsureVisible(ezDocument* pDocument)
{
  for (auto doc : m_DocumentWindows)
  {
    if (doc->GetDocument() == pDocument)
      return EnsureVisible(doc);
  }

  return EZ_FAILURE;
}

ezResult ezQtContainerWindow::EnsureVisible(ezQtApplicationPanel* pPanel)
{
  if (m_ApplicationPanels.IndexOf(pPanel) == ezInvalidIndex)
    return EZ_FAILURE;

  if (pPanel->isClosed())
  {
    pPanel->toggleView();
  }
  pPanel->raise();
  return EZ_SUCCESS;
}

ezResult ezQtContainerWindow::EnsureVisibleAnyContainer(ezDocument* pDocument)
{
  // make sure there is a window to make visible in the first place
  pDocument->GetDocumentManager()->EnsureWindowRequested(pDocument);

  if (s_pContainerWindow->EnsureVisible(pDocument).Succeeded())
    return EZ_SUCCESS;

  return EZ_FAILURE;
}

void ezQtContainerWindow::GetDocumentWindows(ezHybridArray<ezQtDocumentWindow*, 16>& windows)
{
  windows = m_DocumentWindows;
}

bool ezQtContainerWindow::eventFilter(QObject* obj, QEvent* e)
{
  if (e->type() == QEvent::Type::Close)
  {
    if (auto* pFloatingWidget = qobject_cast<ads::CFloatingDockContainer*>(obj))
    {
      ezHybridArray<ezDocument*, 32> docs;
      docs.Reserve(m_DocumentWindows.GetCount());
      ezHybridArray<ezQtDocumentWindow*, 32> windows;
      windows.Reserve(m_DocumentWindows.GetCount());

      QList<ads::CDockWidget*> floatingDocks = pFloatingWidget->dockWidgets();
      for (ezUInt32 i = 0; i < m_DocumentWindows.GetCount(); ++i)
      {
        if (floatingDocks.contains(m_DocumentDocks[i]))
        {
          docs.PushBack(m_DocumentWindows[i]->GetDocument());
          windows.PushBack(m_DocumentWindows[i]);
        }
      }

      if (!ezToolsProject::CanCloseDocuments(docs))
      {
        e->setAccepted(false);
        return true;
      }

      // closing a non-main window should close all documents as well
      // this will remove them from the recently-open documents list and not restore them next time
      for (ezQtDocumentWindow* pWindow : windows)
      {
        pWindow->CloseDocumentWindow();
      }
    }
  }
  return false;
}

void ezQtContainerWindow::SlotDocumentTabCloseRequested()
{
  auto dock = qobject_cast<ads::CDockWidget*>(sender());
  const auto uiListIndex = m_DocumentDocks.IndexOf(dock);
  EZ_ASSERT_DEV(uiListIndex != ezInvalidIndex, "Can't close non-existing document.");

  ezQtDocumentWindow* pDocWindow = m_DocumentWindows[uiListIndex];

  if (!pDocWindow->CanCloseWindow())
  {
    //TODO: There is no CloseRequested event so we just reopen on a timer.
    QTimer::singleShot(1, [dock]() { dock->toggleView(); });
    return;
  }
  pDocWindow->CloseDocumentWindow();
}

void ezQtContainerWindow::DocumentWindowEventHandler(const ezQtDocumentWindowEvent& e)
{
  switch (e.m_Type)
  {
    case ezQtDocumentWindowEvent::Type::WindowClosing:
      RemoveDocumentWindow(e.m_pWindow);
      break;
    case ezQtDocumentWindowEvent::Type::WindowDecorationChanged:
      UpdateWindowDecoration(e.m_pWindow);
      break;

    default:
      break;
  }
}

void ezQtContainerWindow::ProjectEventHandler(const ezToolsProjectEvent& e)
{
  switch (e.m_Type)
  {
    case ezToolsProjectEvent::Type::ProjectOpened:
    case ezToolsProjectEvent::Type::ProjectClosed:
      UpdateWindowTitle();
      break;

    default:
      break;
  }
}

void ezQtContainerWindow::UIServicesEventHandler(const ezQtUiServices::Event& e)
{
  switch (e.m_Type)
  {
    case ezQtUiServices::Event::Type::ShowGlobalStatusBarText:
    {
      if (statusBar() == nullptr)
        setStatusBar(new QStatusBar());

      if (m_pStatusBarLabel == nullptr)
      {
        m_pStatusBarLabel = new QLabel();
        statusBar()->addWidget(m_pStatusBarLabel);

        QPalette pal = m_pStatusBarLabel->palette();
        pal.setColor(QPalette::WindowText, QColor(Qt::red));
        m_pStatusBarLabel->setPalette(pal);
      }

      statusBar()->setHidden(e.m_sText.IsEmpty());
      m_pStatusBarLabel->setText(QString::fromUtf8(e.m_sText.GetData()));
    }
    break;

    default:
      break;
  }
}

void ezQtContainerWindow::SlotTabsContextMenuRequested(const QPoint& pos)
{
  auto tab = qobject_cast<ads::CDockWidgetTab*>(sender());
  ads::CDockWidget* dock = tab->dockWidget();
  const auto uiListIndex = m_DocumentDocks.IndexOf(dock);
  EZ_ASSERT_DEV(uiListIndex != ezInvalidIndex, "Can't close non-existing document.");

  ezQtDocumentWindow* pDoc = m_DocumentWindows[uiListIndex];
  pDoc->RequestWindowTabContextMenu(tab->mapToGlobal(pos));
}
