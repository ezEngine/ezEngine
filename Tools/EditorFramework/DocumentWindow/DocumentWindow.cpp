#include <PCH.h>
#include <EditorFramework/DocumentWindow/DocumentWindow.moc.h>
#include <EditorFramework/EditorFramework.h>
#include <ToolsFoundation/Document/Document.h>
#include <EditorFramework/EditorGUI.moc.h>
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
  pDocument->m_Events.AddEventHandler(ezDelegate<void (const ezDocumentBase::Event&)>(&ezDocumentWindow::DocumentEventHandler, this));
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
  {
    m_pDocument->m_Events.RemoveEventHandler(ezDelegate<void (const ezDocumentBase::Event&)>(&ezDocumentWindow::DocumentEventHandler, this));
    m_pDocument->GetDocumentManager()->s_Events.RemoveEventHandler(ezDelegate<void (const ezDocumentManagerBase::Event&)>(&ezDocumentWindow::DocumentManagerEventHandler, this));
  }
}

void ezDocumentWindow::DocumentEventHandler(const ezDocumentBase::Event& e)
{
  switch (e.m_Type)
  {
  case ezDocumentBase::Event::Type::ModifiedChanged:
    {
      Event dwe;
      dwe.m_pWindow = this;
      dwe.m_Type = Event::Type::WindowDecorationChanged;
      s_Events.Broadcast(dwe);
    }
    break;
  case ezDocumentBase::Event::Type::EnsureVisible:
    {
      EnsureVisible();
    }
    break;
  }
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
  s = s.GetFileNameAndExtension();

  if (m_pDocument && m_pDocument->IsModified())
    s.Append('*');

  return s;
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

    
    ezStringBuilder s, s2;
    s.Format("Failed to save document:\n'%s'", m_pDocument->GetDocumentPath());
    s2.Format("Successfully saved document:\n'%s'", m_pDocument->GetDocumentPath());

    ezEditorGUI::MessageBoxStatus(res, s, s2);

    if (res.m_Result.Failed())
      return res;
  }

  return ezStatus(EZ_SUCCESS);

}

bool ezDocumentWindow::CanCloseWindow()
{
  return InternalCanCloseWindow();
}

bool ezDocumentWindow::InternalCanCloseWindow()
{
  if (m_pDocument && m_pDocument->IsModified())
  {
    QMessageBox::StandardButton res = QMessageBox::question(this, QLatin1String("ezEditor"), QLatin1String("Save before closing?"), QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No | QMessageBox::StandardButton::Cancel, QMessageBox::StandardButton::Cancel);

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
  e.m_pWindow = this;
  e.m_Type = Event::Type::WindowClosed;
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


