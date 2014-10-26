#include <PCH.h>
#include <EditorFramework/ContainerWindow/ContainerWindow.moc.h>
#include <EditorFramework/DocumentWindow/DocumentWindow.moc.h>
#include <EditorFramework/EditorFramework.h>
#include <QSettings>

static ezMap<ezString, ezContainerWindow*> g_ContainerWindows;

ezContainerWindow* ezEditorFramework::GetContainerWindow(const char* szUniqueName, bool bAllowCreate)
{
  auto it = g_ContainerWindows.Find(szUniqueName);

  if (it.IsValid())
    return it.Value();

  if (!bAllowCreate)
    return nullptr;

  EditorRequest r;
  r.m_sContainerName = szUniqueName;
  r.m_Type = EditorRequest::Type::RequestContainerWindow;

  s_EditorRequests.Broadcast(r);

  ezContainerWindow* pContainer = (ezContainerWindow*) r.m_pResult;

  if (pContainer != nullptr)
  {
    g_ContainerWindows[szUniqueName] = pContainer;
  }
  else
  {
    // TODO: Probably don't do this ?
    //pContainer = new ezContainerWindow(szUniqueName);
  }

  return pContainer;
}

ezContainerWindow::ezContainerWindow(const char* szUniqueName)
{
  m_sUniqueName = szUniqueName;
  setObjectName(QLatin1String(szUniqueName));
  setWindowTitle(QString::fromUtf8(szUniqueName));
  setWindowIcon(QIcon(QLatin1String(":/Icons/Icons/ezEditor16.png")));

  ezDocumentWindow::s_Events.AddEventHandler(ezDelegate<void (const ezDocumentWindow::Event&)>(&ezContainerWindow::DocumentWindowEventHandler, this));
}

ezContainerWindow::~ezContainerWindow()
{
  ezDocumentWindow::s_Events.RemoveEventHandler(ezDelegate<void (const ezDocumentWindow::Event&)>(&ezContainerWindow::DocumentWindowEventHandler, this));
}

void ezContainerWindow::closeEvent(QCloseEvent* e)
{
  SaveWindowLayout();
}

void ezContainerWindow::SaveWindowLayout()
{
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

  EZ_VERIFY(connect(pTabs, SIGNAL(tabCloseRequested(int)), this, SLOT(OnDocumentTabCloseRequested(int))) != nullptr, "signal/slot connection failed");

  setCentralWidget(pTabs);
}

ezDocumentWindow* ezContainerWindow::CreateDocumentWindow(const char* szUniqueName)
{
  return new ezDocumentWindow(szUniqueName, this);
}

ezDocumentWindow* ezContainerWindow::AddDocumentWindow(const char* szUniqueName)
{
  EZ_ASSERT(!m_DocumentWindows.Find(szUniqueName).IsValid(), "The document name '%s' is not unique", szUniqueName);

  SetupDocumentTabArea();

  QTabWidget* pTabs = (QTabWidget*) centralWidget();
  EZ_ASSERT(pTabs != nullptr, "The central widget is NULL");

  ezDocumentWindow* pDocWindow = CreateDocumentWindow(szUniqueName);
  EZ_ASSERT(pDocWindow != nullptr, "CreateDocumentWindow(%s) returned NULL", szUniqueName);

  m_DocumentWindows[szUniqueName] = pDocWindow;

  pTabs->addTab(pDocWindow, QString::fromUtf8(pDocWindow->GetDisplayName()));

  return pDocWindow;
}

ezDocumentWindow* ezContainerWindow::GetDocumentWindow(const char* szUniqueName)
{
  auto it = m_DocumentWindows.Find(szUniqueName);

  if (it.IsValid())
    return it.Value();

  return nullptr;
}

void ezContainerWindow::OnDocumentTabCloseRequested(int index)
{
  QTabWidget* pTabs = (QTabWidget*) centralWidget();
  EZ_ASSERT(pTabs != nullptr, "The central widget is NULL");

  ezDocumentWindow* pDocWindow = (ezDocumentWindow*) pTabs->widget(index);

  if (!pDocWindow->CanClose())
    return;

  pDocWindow->CloseDocument();
}

void ezContainerWindow::DocumentWindowEventHandler(const ezDocumentWindow::Event& e)
{
  switch (e.m_Type)
  {
  case ezDocumentWindow::Event::Type::AfterDocumentClosed:
    {
      for (auto it = m_DocumentWindows.GetIterator(); it.IsValid(); ++it)
      {
        if (it.Value() == e.m_pDocument)
        {
          InternalCloseDocumentWindow(e.m_pDocument);
          m_DocumentWindows.Erase(it);
          break;
        }
      }
    }
    break;
  }
}

void ezContainerWindow::InternalCloseDocumentWindow(ezDocumentWindow* pDocumentWindow)
{
  QTabWidget* pTabs = (QTabWidget*) centralWidget();
  EZ_ASSERT(pTabs != nullptr, "The central widget is NULL");

  int iIndex = pTabs->indexOf(pDocumentWindow);
  EZ_ASSERT(iIndex >= 0, "Invalid document window to close");

  pTabs->removeTab(iIndex);
}


