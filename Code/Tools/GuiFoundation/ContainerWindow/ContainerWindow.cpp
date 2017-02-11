#include <GuiFoundation/PCH.h>
#include <GuiFoundation/ContainerWindow/ContainerWindow.moc.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <GuiFoundation/DockPanels/ApplicationPanel.moc.h>
#include <ToolsFoundation/Project/ToolsProject.h>
#include <ToolsFoundation/Document/DocumentManager.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <Foundation/IO/OSFile.h>
#include <QSettings>
#include <QTimer>
#include <QFileDialog>
#include <QCloseEvent>
#include <QTabBar>
#include <QStatusBar>
#include <QLabel>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <ToolsFoundation/Application/ApplicationServices.h>
#include <Foundation/Types/ScopeExit.h>

ezDynamicArray<ezQtContainerWindow*> ezQtContainerWindow::s_AllContainerWindows;
bool ezQtContainerWindow::s_bForceClose = false;

ezQtContainerWindow::ezQtContainerWindow(ezInt32 iUniqueIdentifier)
{
  m_iUniqueIdentifier = iUniqueIdentifier;
  m_bWindowLayoutRestored = false;
  m_pStatusBarLabel = nullptr;
  m_iWindowLayoutRestoreScheduled = 0;

  s_AllContainerWindows.PushBack(this);

  setObjectName(GetUniqueName().GetData());
  setWindowIcon(QIcon(QStringLiteral(":/GuiFoundation/Icons/ezEditor16.png"))); /// \todo Make icon configurable


  ezQtDocumentWindow::s_Events.AddEventHandler(ezMakeDelegate(&ezQtContainerWindow::DocumentWindowEventHandler, this));
  ezToolsProject::s_Events.AddEventHandler(ezMakeDelegate(&ezQtContainerWindow::ProjectEventHandler, this));
  ezQtUiServices::s_Events.AddEventHandler(ezMakeDelegate(&ezQtContainerWindow::UIServicesEventHandler, this));

  UpdateWindowTitle();

  ScheduleRestoreWindowLayout();
}

ezQtContainerWindow::~ezQtContainerWindow()
{
  s_AllContainerWindows.RemoveSwap(this);

  ezQtDocumentWindow::s_Events.RemoveEventHandler(ezMakeDelegate(&ezQtContainerWindow::DocumentWindowEventHandler, this));
  ezToolsProject::s_Events.RemoveEventHandler(ezMakeDelegate(&ezQtContainerWindow::ProjectEventHandler, this));
  ezQtUiServices::s_Events.RemoveEventHandler(ezMakeDelegate(&ezQtContainerWindow::UIServicesEventHandler, this));
}

ezQtContainerWindow* ezQtContainerWindow::CreateNewContainerWindow()
{
  ezInt32 iUniqueIdentifier = 0;
  auto isIdInUse = [](ezInt32 iUniqueIdentifier)->bool
  {
    for (auto pContainer : s_AllContainerWindows)
    {
      if (pContainer->m_iUniqueIdentifier == iUniqueIdentifier)
        return true;
    }
    return false;
  };
  while (isIdInUse(iUniqueIdentifier))
  {
    ++iUniqueIdentifier;
  }

  ezQtContainerWindow* pNewContainer = new ezQtContainerWindow(iUniqueIdentifier);
  return pNewContainer;
}

ezQtContainerWindow* ezQtContainerWindow::GetOrCreateContainerWindow(ezInt32 iUniqueIdentifier)
{
  for (auto pContainer : s_AllContainerWindows)
  {
    if (pContainer->m_iUniqueIdentifier == iUniqueIdentifier)
      return pContainer;
  }

  return new ezQtContainerWindow(iUniqueIdentifier);
}

QTabWidget* ezQtContainerWindow::GetTabWidget() const
{
  QTabWidget* pTabs = qobject_cast<QTabWidget*>(centralWidget());
  EZ_ASSERT_DEV(pTabs != nullptr, "The central widget is nullptr");

  return pTabs;
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

  if (s_bForceClose)
    return;

  s_bForceClose = true;
  EZ_SCOPE_EXIT(s_bForceClose = false);

  e->setAccepted(true);

  if (IsMainContainer())
  {
    if (!ezToolsProject::CanCloseProject())
    {
      e->setAccepted(false);
      return;
    }

    // Closing the main container should also close all other container windows.
    ezDynamicArray<ezQtContainerWindow*> cntainerWindows = s_AllContainerWindows;
    for (ezQtContainerWindow* pContainer : cntainerWindows)
    {
      if (pContainer != this)
        pContainer->close();
    }
  }
  else
  {
    ezHybridArray<ezDocument*, 32> docs;
    docs.Reserve(m_DocumentWindows.GetCount());
    for (ezQtDocumentWindow* pWindow : m_DocumentWindows)
    {
      docs.PushBack(pWindow->GetDocument());
    }
    if (!ezToolsProject::CanCloseDocuments(docs))
    {
      e->setAccepted(false);
      return;
    }

    ezDynamicArray<ezQtDocumentWindow*> windows = m_DocumentWindows;
    for (ezQtDocumentWindow* pWindow : windows)
    {
      pWindow->CloseDocumentWindow();
    }
  }
}


void ezQtContainerWindow::ReassignWindowIndex()
{
  QTabWidget* pTabs = GetTabWidget();

  for (ezInt32 i = 0; i < pTabs->count(); ++i)
  {
    ezQtDocumentWindow* pDocWnd = (ezQtDocumentWindow*)pTabs->widget(i);

    pDocWnd->SetWindowIndex(i);
  }
}

void ezQtContainerWindow::SaveWindowLayout()
{
  for (ezUInt32 i = 0; i < m_DocumentWindows.GetCount(); ++i)
    m_DocumentWindows[i]->SaveWindowLayout();

  QTabWidget* pTabs = GetTabWidget();

  if (pTabs->currentWidget())
  {
    ezQtDocumentWindow* pDoc = (ezQtDocumentWindow*)pTabs->currentWidget();
    pDoc->SaveWindowLayout();
  }

  const bool bMaximized = isMaximized();

  if (bMaximized)
    showNormal();

  ezStringBuilder sGroup;
  sGroup.Format("ContainerWnd_{0}", GetUniqueName().GetData());

  QSettings Settings;
  Settings.beginGroup(QString::fromUtf8(sGroup));
  {
    Settings.setValue("WindowGeometry", saveGeometry());
    Settings.setValue("WindowState", saveState());
    Settings.setValue("IsMaximized", bMaximized);
    Settings.setValue("WindowPosition", pos());

    if (!bMaximized)
      Settings.setValue("WindowSize", size());
  }
  Settings.endGroup();
}

void ezQtContainerWindow::RestoreWindowLayout()
{
  EZ_ASSERT_DEBUG(m_iWindowLayoutRestoreScheduled > 0, "Incorrect use of ScheduleRestoreWindowLayout");

  --m_iWindowLayoutRestoreScheduled;

  if (m_iWindowLayoutRestoreScheduled > 0)
    return;

  show();
  ezStringBuilder sGroup;
  sGroup.Format("ContainerWnd_{0}", GetUniqueName().GetData());

  QSettings Settings;
  Settings.beginGroup(QString::fromUtf8(sGroup));
  {
    restoreGeometry(Settings.value("WindowGeometry", saveGeometry()).toByteArray());

    move(Settings.value("WindowPosition", pos()).toPoint());
    resize(Settings.value("WindowSize", size()).toSize());

    if (Settings.value("IsMaximized", isMaximized()).toBool())
      showMaximized();

    restoreState(Settings.value("WindowState", saveState()).toByteArray());
  }
  Settings.endGroup();

  m_bWindowLayoutRestored = true;
}

void ezQtContainerWindow::SetupDocumentTabArea()
{
  if (centralWidget() != nullptr)
    return;

  QTabWidget* pTabs = new QTabWidget(this);
  pTabs->setObjectName("DocumentTabs");
  pTabs->setTabsClosable(true);
  pTabs->setMovable(true);
  pTabs->setTabShape(QTabWidget::TabShape::Rounded);
  pTabs->setDocumentMode(true);
  pTabs->tabBar()->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);

  EZ_VERIFY(connect(pTabs->tabBar(), SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(SlotTabsContextMenuRequested(const QPoint&))) != nullptr, "signal/slot connection failed");

  EZ_VERIFY(connect(pTabs, SIGNAL(tabCloseRequested(int)), this, SLOT(SlotDocumentTabCloseRequested(int))) != nullptr, "signal/slot connection failed");
  EZ_VERIFY(connect(pTabs, SIGNAL(currentChanged(int)), this, SLOT(SlotDocumentTabCurrentChanged(int))) != nullptr, "signal/slot connection failed");

  setCentralWidget(pTabs);
}

ezString ezQtContainerWindow::GetUniqueName() const
{
  if (IsMainContainer())
  {
    return "ezEditor";
  }
  else
  {
    return ezConversionUtils::ToString(m_iUniqueIdentifier);
  }
}

void ezQtContainerWindow::SlotUpdateWindowDecoration(void* pDocWindow)
{
  UpdateWindowDecoration(static_cast<ezQtDocumentWindow*>(pDocWindow));
}

void ezQtContainerWindow::UpdateWindowDecoration(ezQtDocumentWindow* pDocWindow)
{
  const ezInt32 iListIndex = m_DocumentWindows.IndexOf(pDocWindow);

  if (iListIndex == ezInvalidIndex)
    return;

  QTabWidget* pTabs = GetTabWidget();

  int iTabIndex = pTabs->indexOf(pDocWindow);
  EZ_ASSERT_DEV(iTabIndex >= 0, "Invalid document window to close");

  pTabs->setTabToolTip(iTabIndex, QString::fromUtf8(pDocWindow->GetDisplayName().GetData()));
  pTabs->setTabText(iTabIndex, QString::fromUtf8(pDocWindow->GetDisplayNameShort().GetData()));
  pTabs->setTabIcon(iTabIndex, ezQtUiServices::GetCachedIconResource(pDocWindow->GetWindowIcon().GetData()));
}

void ezQtContainerWindow::RemoveDocumentWindowFromContainer(ezQtDocumentWindow* pDocWindow)
{
  const ezInt32 iListIndex = m_DocumentWindows.IndexOf(pDocWindow);

  if (iListIndex == ezInvalidIndex)
    return;

  QTabWidget* pTabs = GetTabWidget();

  int iTabIndex = pTabs->indexOf(pDocWindow);
  EZ_ASSERT_DEV(iTabIndex >= 0, "Invalid document window to close");

  pTabs->removeTab(iTabIndex);

  m_DocumentWindows.RemoveAtSwap(iListIndex);

  pDocWindow->m_pContainerWindow = nullptr;

  if (m_DocumentWindows.IsEmpty() && m_ApplicationPanels.IsEmpty() && !IsMainContainer())
  {
    close();
    deleteLater();
  }
}

void ezQtContainerWindow::RemoveApplicationPanelFromContainer(ezQtApplicationPanel* pPanel)
{
  const ezInt32 iListIndex = m_ApplicationPanels.IndexOf(pPanel);

  if (iListIndex == ezInvalidIndex)
    return;

  m_ApplicationPanels.RemoveAtSwap(iListIndex);

  pPanel->m_pContainerWindow = nullptr;
}

void ezQtContainerWindow::MoveDocumentWindowToContainer(ezQtDocumentWindow* pDocWindow)
{
  if (m_DocumentWindows.IndexOf(pDocWindow) != ezInvalidIndex)
    return;

  if (pDocWindow->m_pContainerWindow != nullptr)
    pDocWindow->m_pContainerWindow->RemoveDocumentWindowFromContainer(pDocWindow);

  EZ_ASSERT_DEV(pDocWindow->m_pContainerWindow == nullptr, "Implementation error");

  SetupDocumentTabArea();

  m_DocumentWindows.PushBack(pDocWindow);
  pDocWindow->m_pContainerWindow = this;

  QTabWidget* pTabs = GetTabWidget();
  pTabs->addTab(pDocWindow, QString::fromUtf8(pDocWindow->GetDisplayNameShort()));

  // we cannot call virutal functions on pDocWindow here, because the object might still be under construction
  // so we delay it until later
  QMetaObject::invokeMethod(this, "SlotUpdateWindowDecoration", Qt::ConnectionType::QueuedConnection, Q_ARG(void*, pDocWindow));
}

void ezQtContainerWindow::MoveApplicationPanelToContainer(ezQtApplicationPanel* pPanel)
{
  // panel already in container window ?
  if (m_ApplicationPanels.IndexOf(pPanel) != ezInvalidIndex)
    return;

  pPanel->setParent(this);

  if (pPanel->m_pContainerWindow != nullptr)
    pPanel->m_pContainerWindow->RemoveApplicationPanelFromContainer(pPanel);

  EZ_ASSERT_DEV(pPanel->m_pContainerWindow == nullptr, "Implementation error");

  SetupDocumentTabArea();

  m_ApplicationPanels.PushBack(pPanel);
  pPanel->m_pContainerWindow = this;

  // dock the panel
  addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPanel);

  ScheduleRestoreWindowLayout();
}


bool ezQtContainerWindow::IsMainContainer() const
{
  return m_iUniqueIdentifier == 0;
}


ezInt32 ezQtContainerWindow::GetUniqueIdentifier() const
{
  return m_iUniqueIdentifier;
}

ezResult ezQtContainerWindow::EnsureVisible(ezQtDocumentWindow* pDocWindow)
{
  if (m_DocumentWindows.IndexOf(pDocWindow) == ezInvalidIndex)
    return EZ_FAILURE;

  QTabWidget* pTabs = GetTabWidget();

  pTabs->setCurrentWidget(pDocWindow);
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

  pPanel->show();
  pPanel->raise();
  return EZ_SUCCESS;
}

ezResult ezQtContainerWindow::EnsureVisibleAnyContainer(ezDocument* pDocument)
{
  // make sure there is a window to make visible in the first place
  pDocument->GetDocumentManager()->EnsureWindowRequested(pDocument);

  for (auto cont : s_AllContainerWindows)
  {
    if (cont->EnsureVisible(pDocument).Succeeded())
      return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

void ezQtContainerWindow::GetDocumentWindows(ezHybridArray<ezQtDocumentWindow*, 16>& windows)
{
  struct WindowComparer
  {
    WindowComparer(QTabWidget* pTabs) : m_pTabs(pTabs) {}
    EZ_FORCE_INLINE bool Less(ezQtDocumentWindow* a, ezQtDocumentWindow* b) const
    {
      int iIndexA = m_pTabs->indexOf(a);
      int iIndexB = m_pTabs->indexOf(b);
      return iIndexA < iIndexB;
    }
    QTabWidget* m_pTabs;
  };

  if (QTabWidget* pTabs = qobject_cast<QTabWidget*>(centralWidget()))
  {
    windows = m_DocumentWindows;
    windows.Sort(WindowComparer(pTabs));
  }
}

void ezQtContainerWindow::SlotDocumentTabCloseRequested(int index)
{
  QTabWidget* pTabs = GetTabWidget();

  ezQtDocumentWindow* pDocWindow = (ezQtDocumentWindow*)pTabs->widget(index);

  if (!pDocWindow->CanCloseWindow())
    return;

  pDocWindow->CloseDocumentWindow();
}

void ezQtContainerWindow::SlotDocumentTabCurrentChanged(int index)
{
  QTabWidget* pTabs = GetTabWidget();

  ezQtDocumentWindow* pNowCurrentWindow = nullptr;

  if (index >= 0)
    pNowCurrentWindow = (ezQtDocumentWindow*)pTabs->widget(index);

  for (ezQtDocumentWindow* pWnd : m_DocumentWindows)
  {
    if (pNowCurrentWindow == pWnd)
      continue;

    pWnd->SetVisibleInContainer(false);
  }

  if (pNowCurrentWindow)
    pNowCurrentWindow->SetVisibleInContainer(true);

  ReassignWindowIndex();
}

void ezQtContainerWindow::DocumentWindowEventHandler(const ezQtDocumentWindowEvent& e)
{
  switch (e.m_Type)
  {
  case ezQtDocumentWindowEvent::Type::WindowClosing:
    RemoveDocumentWindowFromContainer(e.m_pWindow);
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
  QTabWidget* pTabs = GetTabWidget();

  int iTab = pTabs->tabBar()->tabAt(pos);

  if (iTab < 0)
    return;

  pTabs->setCurrentIndex(iTab);

  if (!pTabs->currentWidget())
    return;

  ezQtDocumentWindow* pDoc = (ezQtDocumentWindow*)pTabs->currentWidget();

  pDoc->RequestWindowTabContextMenu(pTabs->tabBar()->mapToGlobal(pos));
}
