#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Application/Application.h>
#include <Foundation/Types/ScopeExit.h>
#include <GuiFoundation/ContainerWindow/ContainerWindow.moc.h>
#include <GuiFoundation/DockPanels/ApplicationPanel.moc.h>
#include <QCloseEvent>
#include <QLabel>
#include <QStatusBar>
#include <QTabBar>
#include <ads/DockAreaWidget.h>
#include <ads/DockManager.h>
#include <ads/DockWidgetTab.h>
#include <ads/FloatingDockContainer.h>

ezQtContainerWindow* ezQtContainerWindow::s_pContainerWindow = nullptr;
bool ezQtContainerWindow::s_bForceClose = false;

ezQtContainerWindow::ezQtContainerWindow()
{
  setMinimumSize(QSize(800, 600));
  m_pStatusBarLabel = nullptr;

  s_pContainerWindow = this;

  setObjectName("ezEditor");
  setWindowIcon(QIcon(QStringLiteral(":/GuiFoundation/EZ-logo.svg")));

  ezQtDocumentWindow::s_Events.AddEventHandler(ezMakeDelegate(&ezQtContainerWindow::DocumentWindowEventHandler, this));
  ezToolsProject::s_Events.AddEventHandler(ezMakeDelegate(&ezQtContainerWindow::ProjectEventHandler, this));
  ezQtUiServices::s_Events.AddEventHandler(ezMakeDelegate(&ezQtContainerWindow::UIServicesEventHandler, this));

  UpdateWindowTitle();

  ads::CDockManager::ConfigFlags flags =
    ads::CDockManager::DefaultDockAreaButtons |
    ads::CDockManager::ActiveTabHasCloseButton |
    ads::CDockManager::XmlCompressionEnabled |
    ads::CDockManager::FloatingContainerHasWidgetTitle |
    ads::CDockManager::FloatingContainerHasWidgetIcon |
    ads::CDockManager::HideSingleCentralWidgetTitleBar |
    ads::CDockManager::DragPreviewShowsContentPixmap |
    // ads::CDockManager::FocusHighlighting |
    // ads::CDockManager::AlwaysShowTabs |
    // ads::CDockManager::DockAreaHasCloseButton |
    ads::CDockManager::DockAreaCloseButtonClosesTab |
    ads::CDockManager::MiddleMouseButtonClosesTab |
    ads::CDockManager::DockAreaHasTabsMenuButton |
    ads::CDockManager::DockAreaDynamicTabsMenuButtonVisibility |
    // ads::CDockManager::AllTabsHaveCloseButton |
    ads::CDockManager::RetainTabSizeWhenCloseButtonHidden |
    ads::CDockManager::DockAreaHideDisabledButtons |
    ads::CDockManager::DockAreaHasUndockButton |
    // ads::CDockManager::DoubleClickUndocksWidget | // don't want this
    ads::CDockManager::OpaqueSplitterResize;
  ads::CDockManager::setConfigFlags(flags);

  ads::CDockManager::setAutoHideConfigFlags(ads::CDockManager::DefaultAutoHideConfig);
  ads::CDockManager::setAutoHideConfigFlag(ads::CDockManager::AutoHideShowOnMouseOver, false);
  ads::CDockManager::setAutoHideConfigFlag(ads::CDockManager::AutoHideCloseOnOutsideMouseClick, false);

  m_pDockManager = new ads::CDockManager(this);

  connect(m_pDockManager, &ads::CDockManager::floatingWidgetCreated, this, &ezQtContainerWindow::SlotFloatingWidgetOpened);
}

ezQtContainerWindow::~ezQtContainerWindow()
{
  s_pContainerWindow = nullptr;

  ezQtDocumentWindow::s_Events.RemoveEventHandler(ezMakeDelegate(&ezQtContainerWindow::DocumentWindowEventHandler, this));
  ezToolsProject::s_Events.RemoveEventHandler(ezMakeDelegate(&ezQtContainerWindow::ProjectEventHandler, this));
  ezQtUiServices::s_Events.RemoveEventHandler(ezMakeDelegate(&ezQtContainerWindow::UIServicesEventHandler, this));
}

void ezQtContainerWindow::UpdateWindowTitle()
{
  ezStringBuilder sTitle;

  if (ezToolsProject::IsProjectOpen())
  {
    sTitle = ezToolsProject::GetSingleton()->GetProjectName(false);
    sTitle.Append(" - ");
  }

  sTitle.Append(ezApplication::GetApplicationInstance()->GetApplicationName().GetView());

  setWindowTitle(QString::fromUtf8(sTitle.GetData()));
}

void ezQtContainerWindow::closeEvent(QCloseEvent* e)
{
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

  ezToolsProject::SaveProjectState();

  // do not close the documents in the main container window here,
  // as that would remove them from the recently-open documents list and not restore them when opening the editor again
  ezDynamicArray<ezQtDocumentWindow*> windows = m_DocumentWindows;
  for (ezQtDocumentWindow* pWindow : windows)
  {
    pWindow->ShutdownDocumentWindow();
  }

  // We need to destroy the dock manager here, doing it in the constructor leads to an access violation.
  m_pDockManager->deleteLater();
  m_pDockManager = nullptr;
  QMainWindow::closeEvent(e);
}

void ezQtContainerWindow::SlotUpdateWindowDecoration(void* pDocWindow)
{
  UpdateWindowDecoration(static_cast<ezQtDocumentWindow*>(pDocWindow));
}

void ezQtContainerWindow::SlotFloatingWidgetOpened(ads::CFloatingDockContainer* FloatingWidget)
{
  FloatingWidget->installEventFilter(this);
}

void ezQtContainerWindow::SlotDockWidgetFloatingChanged(bool bFloating)
{
  if (!bFloating)
    return;

  for (auto pDoc : m_DocumentWindows)
  {
    UpdateWindowDecoration(pDoc);
  }
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

  if (dock->isFloating())
  {
    dock->dockContainer()->floatingWidget()->setWindowTitle(dock->windowTitle());
    dock->dockContainer()->floatingWidget()->setWindowIcon(dock->icon());
  }
}

void ezQtContainerWindow::RemoveDocumentWindow(ezQtDocumentWindow* pDocWindow)
{
  const ezUInt32 uiListIndex = m_DocumentWindows.IndexOf(pDocWindow);
  if (uiListIndex == ezInvalidIndex)
    return;

  ads::CDockWidget* dock = m_DocumentDocks[uiListIndex];

  int iCurIdx = -1;

  const bool bIsTabbed = dock->isTabbed();
  ads::CDockAreaWidget* pDockArea = dock->dockAreaWidget();

  iCurIdx = pDockArea->currentIndex();

  m_pDockManager->removeDockWidget(dock);

  m_DocumentWindows.RemoveAtAndSwap(uiListIndex);
  m_DocumentDocks.RemoveAtAndSwap(uiListIndex);
  EZ_ASSERT_DEV(m_DockNames.contains(dock->objectName()), "Object name must not change during lifetime.");
  m_DockNames.remove(dock->objectName());
  dock->hide();
  dock->deleteLater();
  pDocWindow->m_pContainerWindow = nullptr;

  if (bIsTabbed)
  {
    iCurIdx = ezMath::Min(iCurIdx, pDockArea->openDockWidgetsCount() - 1);
    pDockArea->setCurrentIndex(iCurIdx);
    pDockArea->currentDockWidget()->update();
  }

  if (pDockArea && pDockArea->openDockWidgetsCount() == 1)
  {
    for (auto pDocWindow2 : m_DocumentWindows)
    {
      UpdateWindowDecoration(pDocWindow2);
    }
  }
}

void ezQtContainerWindow::RemoveApplicationPanel(ezQtApplicationPanel* pPanel)
{
  const auto uiListIndex = m_ApplicationPanels.IndexOf(pPanel);

  if (uiListIndex == ezInvalidIndex)
    return;

  m_pDockManager->removeDockWidget(pPanel);
  m_ApplicationPanels.RemoveAtAndSwap(uiListIndex);

  pPanel->m_pContainerWindow = nullptr;
}

void ezQtContainerWindow::AddDocumentWindow(ezQtDocumentWindow* pDocWindow)
{
  EZ_PROFILE_SCOPE("AddDocumentWindow");
  EZ_ASSERT_DEV(!pDocWindow->objectName().isEmpty(), "Panel name must be unique and not empty.");

  if (m_DocumentWindows.IndexOf(pDocWindow) != ezInvalidIndex)
    return;

  EZ_ASSERT_DEV(pDocWindow->m_pContainerWindow == nullptr, "Implementation error");

  // NOTE: This function is called by the ezQtDocumentWindow constructor
  // that means any derived classes are not yet constructed!
  // therefore calling virtual functions here, like GetDisplayNameShort() will still call
  // the base class implementation, NOT the derived one !
  // therefore, we do some stuff in ezQtContainerWindow::UpdateWindowDecoration() instead

  pDocWindow->m_pContainerWindow = this;

  m_DocumentWindows.PushBack(pDocWindow);
  ezString displayName = pDocWindow->GetDisplayNameShort();
  ads::CDockWidget* dock = new ads::CDockWidget(m_pDockManager, QString::fromUtf8(displayName.GetData(), displayName.GetElementCount()));
  dock->installEventFilter(pDocWindow);

  dock->setFeature(ads::CDockWidget::CustomCloseHandling, true);
  dock->setFeature(ads::CDockWidget::DockWidgetPinnable, false);

  // this is a hacky way to detect the ezQtSettingsTab
  if (displayName == "Settings")
  {
    dock->setFeature(ads::CDockWidget::DockWidgetClosable, false);
    dock->setFeature(ads::CDockWidget::DockWidgetMovable, false);
    dock->setFeature(ads::CDockWidget::DockWidgetFloatable, false);
    dock->setFeature(ads::CDockWidget::NoTab, true);
  }

  dock->setObjectName(pDocWindow->GetUniqueName());
  EZ_ASSERT_DEV(!dock->objectName().isEmpty(), "Dock name must not be empty.");
  EZ_ASSERT_DEV(!m_DockNames.contains(dock->objectName()), "Dock name must be unique.");
  m_DockNames.insert(dock->objectName());
  dock->setWidget(pDocWindow);
  dock->tabWidget()->setContextMenuPolicy(Qt::CustomContextMenu);
  if (!m_DocumentDocks.IsEmpty())
  {
    ads::CDockAreaWidget* dockArea = m_DocumentDocks.PeekBack()->dockAreaWidget();
    m_pDockManager->addDockWidgetTabToArea(dock, dockArea);
  }
  else
  {
    EZ_PROFILE_SCOPE("AddDocumentWindow - addDockWidgetTab");
    m_pDockManager->addDockWidgetTab(ads::CenterDockWidgetArea, dock);
  }
  m_DocumentDocks.PushBack(dock);
  connect(dock, &ads::CDockWidget::closeRequested, this, &ezQtContainerWindow::SlotDocumentTabCloseRequested);
  connect(dock->tabWidget(), &QWidget::customContextMenuRequested, this, &ezQtContainerWindow::SlotTabsContextMenuRequested);
  connect(dock, &ads::CDockWidget::topLevelChanged, this, &ezQtContainerWindow::SlotDockWidgetFloatingChanged);


  pDocWindow->m_pContainerWindow = this;

  // we cannot call virtual functions on pDocWindow here, because the object might still be under construction
  // so we delay it until later
  QMetaObject::invokeMethod(this, "SlotUpdateWindowDecoration", Qt::ConnectionType::QueuedConnection, Q_ARG(void*, pDocWindow));
}

void ezQtContainerWindow::DocumentWindowRenamed(ezQtDocumentWindow* pDocWindow)
{
  const ezUInt32 uiListIndex = m_DocumentWindows.IndexOf(pDocWindow);
  if (uiListIndex == ezInvalidIndex)
    return;

  ads::CDockWidget* dock = m_DocumentDocks[uiListIndex];
  EZ_ASSERT_DEV(m_DockNames.contains(dock->objectName()), "Object name must not change during lifetime.");
  m_DockNames.remove(dock->objectName());

  dock->setObjectName(pDocWindow->GetUniqueName());
  EZ_ASSERT_DEV(!dock->objectName().isEmpty(), "Dock name must not be empty.");
  EZ_ASSERT_DEV(!m_DockNames.contains(dock->objectName()), "Dock name must be unique.");
  m_DockNames.insert(dock->objectName());
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
  m_pDockManager->addDockWidgetTab(ads::RightDockWidgetArea, pPanel);
}

ezResult ezQtContainerWindow::EnsureVisible(ezQtDocumentWindow* pDocWindow)
{
  const auto uiListIndex = m_DocumentWindows.IndexOf(pDocWindow);

  if (uiListIndex == ezInvalidIndex)
    return EZ_FAILURE;

  ads::CDockWidget* dock = m_DocumentDocks[uiListIndex];

  dock->toggleView(true);
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

void ezQtContainerWindow::SaveDocumentWindowStates(ezMap<ads::CDockWidget*, DocumentWindowState>& out_states)
{
  out_states.Clear();
  for (ads::CDockWidget* pDock : m_DocumentDocks)
  {
    DocumentWindowState state;
    state.m_bFloating = pDock->isFloating();
    out_states[pDock] = state;
  }
}

void ezQtContainerWindow::RestoreDocumentWindowStates(const ezMap<ads::CDockWidget*, DocumentWindowState>& states)
{
  if (m_DocumentDocks.IsEmpty())
    return;

  // Find a dock area where documents are docked (not floating, not closed)
  ads::CDockAreaWidget* pDocumentArea = nullptr;
  for (ads::CDockWidget* pDock : m_DocumentDocks)
  {
    if (!pDock->isClosed() && !pDock->isFloating())
    {
      pDocumentArea = pDock->dockAreaWidget();
      break;
    }
  }

  // Restore each document window to its previous state
  for (ads::CDockWidget* pDock : m_DocumentDocks)
  {
    auto it = states.Find(pDock);
    const bool bWasFloating = it.IsValid() ? it.Value().m_bFloating : false;

    if (pDock->isClosed())
    {
      if (bWasFloating)
      {
        // Was floating, make it floating again
        m_pDockManager->addDockWidgetFloating(pDock);
      }
      else
      {
        // Was docked, re-dock it
        if (pDocumentArea != nullptr)
        {
          m_pDockManager->addDockWidgetTabToArea(pDock, pDocumentArea);
        }
        else
        {
          // No area yet, add to center and use that as the document area
          pDocumentArea = m_pDockManager->addDockWidgetTab(ads::CenterDockWidgetArea, pDock);
        }
      }
    }
    // If not closed, leave it as-is (it's already visible, either floating or docked)
  }

  for (auto pDocWindow : m_DocumentWindows)
  {
    UpdateWindowDecoration(pDocWindow);
  }
}

ezResult ezQtContainerWindow::EnsureVisibleAnyContainer(ezDocument* pDocument)
{
  // make sure there is a window to make visible in the first place
  pDocument->GetDocumentManager()->EnsureWindowRequested(pDocument);

  if (s_pContainerWindow->EnsureVisible(pDocument).Succeeded())
    return EZ_SUCCESS;

  return EZ_FAILURE;
}

void ezQtContainerWindow::GetDocumentWindows(ezHybridArray<ezQtDocumentWindow*, 16>& ref_windows)
{
  ref_windows = m_DocumentWindows;
}

bool ezQtContainerWindow::eventFilter(QObject* obj, QEvent* e)
{
  if (e->type() == QEvent::Type::Close)
  {
    if (auto* pFloatingWidget = qobject_cast<ads::CFloatingDockContainer*>(obj))
    {
      ezTempHybridArray<ezDocument*, 32> docs;
      docs.Reserve(m_DocumentWindows.GetCount());
      ezTempHybridArray<ezQtDocumentWindow*, 32> windows;
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
      // This is necessary to clean up some 'delete later' Qt objects before the document is closed as they need to remove their references to the doc.
      qApp->processEvents();
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
