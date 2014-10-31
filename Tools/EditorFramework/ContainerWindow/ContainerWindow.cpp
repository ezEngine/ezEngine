#include <PCH.h>
#include <EditorFramework/ContainerWindow/ContainerWindow.moc.h>
#include <EditorFramework/DocumentWindow/DocumentWindow.moc.h>
#include <EditorFramework/EditorFramework.h>
#include <EditorFramework/Settings/SettingsTab.moc.h>
#include <QSettings>
#include <QMenu>
#include <QMenuBar>
#include <QTimer>
#include <QPushButton>
#include <QToolButton>

ezContainerWindow::ezContainerWindow()
{
  setObjectName(QLatin1String(GetUniqueName())); // todo
  setWindowTitle(QString::fromUtf8(GetUniqueName()));
  setWindowIcon(QIcon(QLatin1String(":/Icons/Icons/ezEditor16.png")));

  QMenu* pMenuSettings = menuBar()->addMenu("Settings");
  QAction* pAction = pMenuSettings->addAction("Plugins");

  EZ_VERIFY(connect(pAction, SIGNAL(triggered()), this, SLOT(SlotMenuSettingsPlugins())) != nullptr, "signal/slot connection failed");

  ezDocumentWindow::s_Events.AddEventHandler(ezDelegate<void (const ezDocumentWindow::Event&)>(&ezContainerWindow::DocumentWindowEventHandler, this));

  QTimer::singleShot(0, this, SLOT(SlotRestoreLayout()));
}

ezContainerWindow::~ezContainerWindow()
{
  ezDocumentWindow::s_Events.RemoveEventHandler(ezDelegate<void (const ezDocumentWindow::Event&)>(&ezContainerWindow::DocumentWindowEventHandler, this));
}

void ezContainerWindow::SlotRestoreLayout()
{
  RestoreWindowLayout();
}

void ezContainerWindow::SlotMenuSettingsPlugins()
{
  ezEditorFramework::ShowPluginConfigDialog();
}

void ezContainerWindow::closeEvent(QCloseEvent* e)
{
  SaveWindowLayout();
}

void ezContainerWindow::SaveWindowLayout()
{
  for (ezUInt32 i = 0; i < m_DocumentWindows.GetCount(); ++i)
    m_DocumentWindows[i]->SaveWindowLayout();

  QTabWidget* pTabs = (QTabWidget*) centralWidget();
  EZ_ASSERT(pTabs != nullptr, "The central widget is NULL");

  if (pTabs->currentWidget())
  {
    ezDocumentWindow* pDoc = (ezDocumentWindow*) pTabs->currentWidget();
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

  EZ_VERIFY(connect(pTabs, SIGNAL(tabCloseRequested(int)), this, SLOT(SlotDocumentTabCloseRequested(int))) != nullptr, "signal/slot connection failed");

  m_pActionSettings = new QAction("Settings", this);
  EZ_VERIFY(connect(m_pActionSettings, SIGNAL(triggered()), this, SLOT(SlotSettings())) != nullptr, "signal/slot connection failed");

  QToolButton* pButton = new QToolButton();
  pButton->setText("+");
  pButton->setIcon(QIcon(QLatin1String(":/Icons/Icons/ezEditor16.png")));
  pButton->setAutoRaise(true);
  pButton->setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonTextOnly);
  pButton->setPopupMode(QToolButton::ToolButtonPopupMode::InstantPopup);
  pButton->setArrowType(Qt::ArrowType::NoArrow);
  pButton->setStyleSheet("QToolButton::menu-indicator { image: none; }");
  pButton->setMenu(new QMenu());
  pButton->menu()->addAction("Create Document");
  pButton->menu()->addAction("Open Document");
  pButton->menu()->addSeparator();
  pButton->menu()->addAction("Create Project");
  pButton->menu()->addAction("Open Project");
  pButton->menu()->addSeparator();
  pButton->menu()->addAction(m_pActionSettings);

  pTabs->setCornerWidget(pButton, Qt::Corner::TopLeftCorner);

  setCentralWidget(pTabs);
}

void ezContainerWindow::RemoveDocumentWindowFromContainer(ezDocumentWindow* pDocWindow)
{
  const ezInt32 iListIndex = m_DocumentWindows.IndexOf(pDocWindow);

  if (iListIndex == ezInvalidIndex)
    return;

  QTabWidget* pTabs = (QTabWidget*) centralWidget();
  EZ_ASSERT(pTabs != nullptr, "The central widget is NULL");

  int iTabIndex = pTabs->indexOf(pDocWindow);
  EZ_ASSERT(iTabIndex >= 0, "Invalid document window to close");

  pTabs->removeTab(iTabIndex);

  m_DocumentWindows.RemoveAtSwap(iListIndex);

  pDocWindow->m_pContainerWindow = nullptr;
}

void ezContainerWindow::MoveDocumentWindowToContainer(ezDocumentWindow* pDocWindow)
{
  if (m_DocumentWindows.IndexOf(pDocWindow) != ezInvalidIndex)
    return;

  if (pDocWindow->m_pContainerWindow != nullptr)
    pDocWindow->m_pContainerWindow->RemoveDocumentWindowFromContainer(pDocWindow);

  EZ_ASSERT(pDocWindow->m_pContainerWindow == nullptr, "Implementation error");

  SetupDocumentTabArea();

  m_DocumentWindows.PushBack(pDocWindow);
  pDocWindow->m_pContainerWindow = this;

  QTabWidget* pTabs = (QTabWidget*) centralWidget();
  EZ_ASSERT(pTabs != nullptr, "The central widget is NULL");

  pTabs->addTab(pDocWindow, QString::fromUtf8(pDocWindow->GetDisplayNameShort()));
}

void ezContainerWindow::EnsureVisible(ezDocumentWindow* pDocWindow)
{
  if (m_DocumentWindows.IndexOf(pDocWindow) == ezInvalidIndex)
    return;

  QTabWidget* pTabs = (QTabWidget*) centralWidget();
  EZ_ASSERT(pTabs != nullptr, "The central widget is NULL");

  pTabs->setCurrentWidget(pDocWindow);
}

void ezContainerWindow::SlotDocumentTabCloseRequested(int index)
{
  QTabWidget* pTabs = (QTabWidget*) centralWidget();
  EZ_ASSERT(pTabs != nullptr, "The central widget is NULL");

  ezDocumentWindow* pDocWindow = (ezDocumentWindow*) pTabs->widget(index);

  if (!pDocWindow->CanClose())
    return;

  pDocWindow->CloseDocument();
  delete pDocWindow;
}

void ezContainerWindow::DocumentWindowEventHandler(const ezDocumentWindow::Event& e)
{
  switch (e.m_Type)
  {
  case ezDocumentWindow::Event::Type::AfterDocumentClosed:
    RemoveDocumentWindowFromContainer(e.m_pDocument);
    break;
  }
}

void ezContainerWindow::SlotSettings()
{
  ezSettingsTab* pSettingsTab = ezSettingsTab::GetInstance();

  if (pSettingsTab == nullptr)
  {
    pSettingsTab = new ezSettingsTab();

    ezEditorFramework::AddDocumentWindow(pSettingsTab);
  }

  pSettingsTab->EnsureVisible();
}

