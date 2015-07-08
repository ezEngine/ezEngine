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

ezDynamicArray<ezContainerWindow*> ezContainerWindow::s_AllContainerWindows;

ezContainerWindow::ezContainerWindow()
{
  m_bWindowLayoutRestored = false;
  m_pStatusBarLabel = nullptr;
  m_iWindowLayoutRestoreScheduled = 0;

  setObjectName(QLatin1String(GetUniqueName())); // todo
  setWindowIcon(QIcon(QLatin1String(":/GuiFoundation/Icons/ezEditor16.png"))); /// \todo Make icon configurable

  s_AllContainerWindows.PushBack(this);

  ezDocumentWindow::s_Events.AddEventHandler(ezMakeDelegate(&ezContainerWindow::DocumentWindowEventHandler, this));
  ezToolsProject::s_Events.AddEventHandler(ezMakeDelegate(&ezContainerWindow::ProjectEventHandler, this));
  ezUIServices::s_Events.AddEventHandler(ezMakeDelegate(&ezContainerWindow::UIServicesEventHandler, this));

  UpdateWindowTitle();

  ScheduleRestoreWindowLayout();
}

ezContainerWindow::~ezContainerWindow()
{
  s_AllContainerWindows.RemoveSwap(this);

  ezDocumentWindow::s_Events.RemoveEventHandler(ezMakeDelegate(&ezContainerWindow::DocumentWindowEventHandler, this));
  ezToolsProject::s_Events.RemoveEventHandler(ezMakeDelegate(&ezContainerWindow::ProjectEventHandler, this));
  ezUIServices::s_Events.RemoveEventHandler(ezMakeDelegate(&ezContainerWindow::UIServicesEventHandler, this));
}

QTabWidget* ezContainerWindow::GetTabWidget() const
{
  QTabWidget* pTabs = (QTabWidget*)centralWidget();
  EZ_ASSERT_DEV(pTabs != nullptr, "The central widget is NULL");

  return pTabs;
}

void ezContainerWindow::UpdateWindowTitle()
{
  ezStringBuilder sTitle;

  if (ezToolsProject::IsProjectOpen())
  {
    sTitle = ezPathUtils::GetFileName(ezToolsProject::GetInstance()->GetProjectPath());
    sTitle.Append(" - ");
  }

  sTitle.Append(ezUIServices::GetApplicationName());

  setWindowTitle(QString::fromUtf8(sTitle.GetData()));
}

void ezContainerWindow::ScheduleRestoreWindowLayout()
{
  m_iWindowLayoutRestoreScheduled++;
  QTimer::singleShot(0, this, SLOT(SlotRestoreLayout()));
}

void ezContainerWindow::SlotRestoreLayout()
{
  RestoreWindowLayout();
}

void ezContainerWindow::closeEvent(QCloseEvent* e)
{
  if (!ezToolsProject::CanCloseProject())
  {
    e->setAccepted(false);
    return;
  }

  SaveWindowLayout();
}

void ezContainerWindow::SaveWindowLayout()
{
  for (ezUInt32 i = 0; i < m_DocumentWindows.GetCount(); ++i)
    m_DocumentWindows[i]->SaveWindowLayout();

  QTabWidget* pTabs = GetTabWidget();

  if (pTabs->currentWidget())
  {
    ezDocumentWindow* pDoc = (ezDocumentWindow*)pTabs->currentWidget();
    pDoc->SaveWindowLayout();
  }

  const bool bMaximized = isMaximized();

  if (bMaximized)
    showNormal();

  ezStringBuilder sGroup;
  sGroup.Format("ContainerWnd_%s", GetUniqueName());

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

void ezContainerWindow::RestoreWindowLayout()
{
  EZ_ASSERT_DEBUG(m_iWindowLayoutRestoreScheduled > 0, "Incorrect use of ScheduleRestoreWindowLayout");

  --m_iWindowLayoutRestoreScheduled;

  if (m_iWindowLayoutRestoreScheduled > 0)
    return;

  ezStringBuilder sGroup;
  sGroup.Format("ContainerWnd_%s", GetUniqueName());

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

void ezContainerWindow::SetupDocumentTabArea()
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

void ezContainerWindow::SlotUpdateWindowDecoration(void* pDocWindow)
{
  UpdateWindowDecoration(static_cast<ezDocumentWindow*>(pDocWindow));
}

void ezContainerWindow::UpdateWindowDecoration(ezDocumentWindow* pDocWindow)
{
  const ezInt32 iListIndex = m_DocumentWindows.IndexOf(pDocWindow);

  if (iListIndex == ezInvalidIndex)
    return;

  QTabWidget* pTabs = GetTabWidget();

  int iTabIndex = pTabs->indexOf(pDocWindow);
  EZ_ASSERT_DEV(iTabIndex >= 0, "Invalid document window to close");

  pTabs->setTabToolTip(iTabIndex, QString::fromUtf8(pDocWindow->GetDisplayName().GetData()));
  pTabs->setTabText(iTabIndex, QString::fromUtf8(pDocWindow->GetDisplayNameShort().GetData()));
  pTabs->setTabIcon(iTabIndex, QIcon(QString::fromUtf8(pDocWindow->GetWindowIcon().GetData())));
}

void ezContainerWindow::RemoveDocumentWindowFromContainer(ezDocumentWindow* pDocWindow)
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
}

void ezContainerWindow::RemoveApplicationPanelFromContainer(ezApplicationPanel* pPanel)
{
  const ezInt32 iListIndex = m_ApplicationPanels.IndexOf(pPanel);

  if (iListIndex == ezInvalidIndex)
    return;

  m_ApplicationPanels.RemoveAtSwap(iListIndex);

  pPanel->m_pContainerWindow = nullptr;
}

void ezContainerWindow::MoveDocumentWindowToContainer(ezDocumentWindow* pDocWindow)
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
  int iTab = pTabs->addTab(pDocWindow, QString::fromUtf8(pDocWindow->GetDisplayNameShort()));

  // we cannot call virutal functions on pDocWindow here, because the object might still be under construction
  // so we delay it until later
  QMetaObject::invokeMethod(this, "SlotUpdateWindowDecoration", Qt::ConnectionType::QueuedConnection, Q_ARG(void*, pDocWindow));
}

void ezContainerWindow::MoveApplicationPanelToContainer(ezApplicationPanel* pPanel)
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

ezResult ezContainerWindow::EnsureVisible(ezDocumentWindow* pDocWindow)
{
  if (m_DocumentWindows.IndexOf(pDocWindow) == ezInvalidIndex)
    return EZ_FAILURE;

  QTabWidget* pTabs = GetTabWidget();

  pTabs->setCurrentWidget(pDocWindow);
  return EZ_SUCCESS;
}

ezResult ezContainerWindow::EnsureVisible(ezDocumentBase* pDocument)
{
  for (auto doc : m_DocumentWindows)
  {
    if (doc->GetDocument() == pDocument)
      return EnsureVisible(doc);
  }

  return EZ_FAILURE;
}

ezResult ezContainerWindow::EnsureVisible(ezApplicationPanel* pPanel)
{
  if (m_ApplicationPanels.IndexOf(pPanel) == ezInvalidIndex)
    return EZ_FAILURE;

  pPanel->show();
  pPanel->raise();
  return EZ_SUCCESS;
}

ezResult ezContainerWindow::EnsureVisibleAnyContainer(ezDocumentBase* pDocument)
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

void ezContainerWindow::SlotDocumentTabCloseRequested(int index)
{
  QTabWidget* pTabs = GetTabWidget();

  ezDocumentWindow* pDocWindow = (ezDocumentWindow*)pTabs->widget(index);

  if (!pDocWindow->CanCloseWindow())
    return;

  pDocWindow->CloseDocumentWindow();
}

void ezContainerWindow::SlotDocumentTabCurrentChanged(int index)
{
  QTabWidget* pTabs = GetTabWidget();

  ezDocumentWindow* pNowCurrentWindow = nullptr;

  if (index >= 0)
    pNowCurrentWindow = (ezDocumentWindow*)pTabs->widget(index);

  for (ezDocumentWindow* pWnd : m_DocumentWindows)
  {
    if (pNowCurrentWindow == pWnd)
      continue;

    pWnd->SetVisibleInContainer(false);
  }

  if (pNowCurrentWindow)
    pNowCurrentWindow->SetVisibleInContainer(true);
}

void ezContainerWindow::DocumentWindowEventHandler(const ezDocumentWindow::Event& e)
{
  switch (e.m_Type)
  {
  case ezDocumentWindow::Event::Type::WindowClosing:
    RemoveDocumentWindowFromContainer(e.m_pWindow);
    break;
  case ezDocumentWindow::Event::Type::WindowDecorationChanged:
    UpdateWindowDecoration(e.m_pWindow);
    break;
  }
}

void ezContainerWindow::ProjectEventHandler(const ezToolsProject::Event& e)
{
  switch (e.m_Type)
  {
  case ezToolsProject::Event::Type::ProjectOpened:
  case ezToolsProject::Event::Type::ProjectClosed:
    UpdateWindowTitle();
    break;
  }
}

void ezContainerWindow::UIServicesEventHandler(const ezUIServices::Event& e)
{
  switch (e.m_Type)
  {
  case ezUIServices::Event::Type::ShowGlobalStatusBarText:
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
  }
}

void ezContainerWindow::SlotTabsContextMenuRequested(const QPoint& pos)
{
  QTabWidget* pTabs = GetTabWidget();

  int iTab = pTabs->tabBar()->tabAt(pos);

  if (iTab < 0)
    return;

  pTabs->setCurrentIndex(iTab);

  if (!pTabs->currentWidget())
    return;

  ezDocumentWindow* pDoc = (ezDocumentWindow*)pTabs->currentWidget();

  pDoc->RequestWindowTabContextMenu(pTabs->tabBar()->mapToGlobal(pos));
}
