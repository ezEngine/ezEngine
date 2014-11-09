#include <PCH.h>
#include <EditorFramework/DocumentWindow/DocumentWindow.moc.h>
#include <EditorFramework/EditorFramework.h>
#include <ToolsFoundation/Document/Document.h>
#include <QSettings>
#include <QMessageBox>
#include <QTimer>

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

  pWindow->ScheduleRestoreWindowLayout();
}

void ezDocumentWindow::Constructor()
{
  m_pContainerWindow = nullptr;
  m_pDocument = nullptr;

  setDockNestingEnabled(true);
}

ezDocumentWindow::ezDocumentWindow(ezDocumentBase* pDocument)
{
  Constructor();

  m_pDocument = pDocument;
  m_sUniqueName = m_pDocument->GetDocumentPath();

  setObjectName(GetUniqueName());

  pDocument->GetDocumentManager()->s_Events.AddEventHandler(ezDelegate<void (const ezDocumentManagerBase::Event&)>(&ezDocumentWindow::DocumentManagerEventHandler, this));
}

ezDocumentWindow::ezDocumentWindow(const char* szUniqueName)
{
  Constructor();

  m_sUniqueName = szUniqueName;

  setObjectName(GetUniqueName());
}


ezDocumentWindow::~ezDocumentWindow()
{
  if (m_pDocument)
    m_pDocument->GetDocumentManager()->s_Events.RemoveEventHandler(ezDelegate<void (const ezDocumentManagerBase::Event&)>(&ezDocumentWindow::DocumentManagerEventHandler, this));
}

void ezDocumentWindow::DocumentManagerEventHandler(const ezDocumentManagerBase::Event& e)
{
  switch (e.m_Type)
  {
  case ezDocumentManagerBase::Event::Type::DocumentClosing:
    {
      if (e.m_pDocument == m_pDocument)
      {
        ShutdownDocumentWindow();
        return;
      }
    }
    break;
  }
}

ezString ezDocumentWindow::GetDisplayNameShort() const
{
  ezStringBuilder s = GetDisplayName();
  return s.GetFileNameAndExtension();
}

void ezDocumentWindow::ScheduleRestoreWindowLayout()
{
  QTimer::singleShot(0, this, SLOT(SlotRestoreLayout()));
}

void ezDocumentWindow::SlotRestoreLayout()
{
  RestoreWindowLayout();
}

void ezDocumentWindow::SaveWindowLayout()
{
  const bool bMaximized = isMaximized();

  if (bMaximized)
    showNormal();

  ezStringBuilder sGroup;
  sGroup.Format("DocumentWnd_%s", GetGroupName());

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
  sGroup.Format("DocumentWnd_%s", GetGroupName());

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

ezStatus ezDocumentWindow::SaveDocument()
{
  if (m_pDocument)
  {
    ezStatus res = m_pDocument->SaveDocument();

    if (res.m_Result.Failed())
    {
      ezStringBuilder s;
      s.Format("Failed to save document:\n'%s'", m_pDocument->GetDocumentPath());

      if (!res.m_sError.IsEmpty())
        s.Append("\n\nDetails: ", res.m_sError.GetData());

      QMessageBox::warning(this, QLatin1String("ezEditor"), QString::fromUtf8(s.GetData()), QMessageBox::StandardButton::Ok);
      return res;
    }
  }

  return ezStatus(EZ_SUCCESS);

}

bool ezDocumentWindow::CanCloseWindow()
{
  return InternalCanCloseWindow();
}

bool ezDocumentWindow::InternalCanCloseWindow()
{
  if (m_pDocument)
  {
    // todo check if modified

    QMessageBox::StandardButton res = QMessageBox::question(this, QLatin1String("Close?"), QLatin1String("Save before closing?"), QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No | QMessageBox::StandardButton::Cancel, QMessageBox::StandardButton::Cancel);

    if (res == QMessageBox::StandardButton::Cancel)
      return false;

    if (res == QMessageBox::StandardButton::Yes)
    {
      if (SaveDocument().m_Result.Failed())
        return false;
    }
  }

  return true;
}

void ezDocumentWindow::CloseDocumentWindow()
{
  if (m_pDocument)
  {
    m_pDocument->GetDocumentManager()->CloseDocument(m_pDocument);
    return;
  }
  else
  {
    ShutdownDocumentWindow();
  }
}

void ezDocumentWindow::ShutdownDocumentWindow()
{
  SaveWindowLayout();

  InternalCloseDocumentWindow();

  Event e;
  e.m_pDocument = this;
  e.m_Type = Event::Type::DocumentWindowClosed;
  s_Events.Broadcast(e);

  InternalDeleteThis();
}

void ezDocumentWindow::InternalCloseDocumentWindow()
{
}

void ezDocumentWindow::EnsureVisible()
{
  m_pContainerWindow->EnsureVisible(this);
}


