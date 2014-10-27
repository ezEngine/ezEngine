#include <PCH.h>
#include <EditorFramework/DocumentWindow/DocumentWindow.moc.h>
#include <EditorFramework/EditorFramework.h>
#include <QSettings>
#include <QMessageBox>

ezEvent<const ezDocumentWindow::Event&> ezDocumentWindow::s_Events;
ezMap<ezString, ezDocumentWindow*> ezEditorFramework::s_DocumentWindows;


ezDocumentWindow* ezEditorFramework::GetDocumentWindow(const char* szUniqueName)
{
  auto it = s_DocumentWindows.Find(szUniqueName);

  if (it.IsValid())
    return it.Value();

  return nullptr;
}

void ezEditorFramework::AddDocumentWindow(ezDocumentWindow* pWindow)
{
  s_DocumentWindows[pWindow->GetUniqueName()] = pWindow;
  s_ContainerWindows[0]->MoveDocumentWindowToContainer(pWindow);

  pWindow->RestoreWindowLayout();
}


ezDocumentWindow::ezDocumentWindow(const char* szUniqueName)
{
  m_pContainerWindow = nullptr;

  m_sUniqueName = szUniqueName;
  setObjectName(QLatin1String(szUniqueName));

  setDockNestingEnabled(true);

  //setAutoFillBackground(true);
  //setBackgroundRole(QPalette::ColorRole::Highlight);

  // todo restore state
}

ezDocumentWindow::~ezDocumentWindow()
{
}

void ezDocumentWindow::SaveWindowLayout()
{
  const bool bMaximized = isMaximized();

  if (bMaximized)
    showNormal();

  ezStringBuilder sGroup;
  sGroup.Format("DocumentWnd_%s", GetUniqueName());

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

void ezDocumentWindow::RestoreWindowLayout()
{
  ezStringBuilder sGroup;
  sGroup.Format("DocumentWnd_%s", GetUniqueName());

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

bool ezDocumentWindow::CanClose()
{
  return InternalCanClose();
}

bool ezDocumentWindow::InternalCanClose()
{
  // todo check if modified

  QMessageBox::StandardButton res = QMessageBox::question(this, QLatin1String("Close?"), QLatin1String("Save before closing?"), QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No | QMessageBox::StandardButton::Cancel, QMessageBox::StandardButton::Cancel);

  if (res == QMessageBox::StandardButton::Cancel)
    return false;

  // todo save ... 

  return true;
}

void ezDocumentWindow::CloseDocument()
{
  SaveWindowLayout();

  Event e;
  e.m_pDocument = this;

  e.m_Type = Event::Type::BeforeDocumentClosed;
  s_Events.Broadcast(e);

  InternalCloseDocument();

  e.m_Type = Event::Type::AfterDocumentClosed;
  s_Events.Broadcast(e);
}

void ezDocumentWindow::InternalCloseDocument()
{
}




