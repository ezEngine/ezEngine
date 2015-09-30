#include <GuiFoundation/PCH.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <GuiFoundation/ContainerWindow/ContainerWindow.moc.h>
#include <ToolsFoundation/Document/Document.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/MenuActionMapView.moc.h>
#include <Foundation/Logging/Log.h>
#include <QSettings>
#include <QMessageBox>
#include <QTimer>
#include <QStatusBar>

ezEvent<const ezDocumentWindow::Event&> ezDocumentWindow::s_Events;
ezDynamicArray<ezDocumentWindow*> ezDocumentWindow::s_AllDocumentWindows;

void ezDocumentWindow::Constructor()
{
  if (s_AllDocumentWindows.IsEmpty())
  {
    ezActionMapManager::RegisterActionMap("DocumentWindowTabMenu");
    ezDocumentActions::MapActions("DocumentWindowTabMenu", "", false);
  }

  s_AllDocumentWindows.PushBack(this);

  setStatusBar(new QStatusBar());

  m_pContainerWindow = nullptr;
  m_bIsVisibleInContainer = false;
  m_bRedrawIsTriggered = false;
  m_bIsDrawingATM = false;
  m_bTriggerRedrawQueued = false;
  m_iTargetFramerate = 0;

  setDockNestingEnabled(true);

  ezMenuBarActionMapView* pMenuBar = new ezMenuBarActionMapView(this);
  setMenuBar(pMenuBar);

  ezContainerWindow::GetAllContainerWindows()[0]->MoveDocumentWindowToContainer(this);

  ezUIServices::s_Events.AddEventHandler(ezMakeDelegate(&ezDocumentWindow::UIServicesEventHandler, this));

  ScheduleRestoreWindowLayout();
}

ezDocumentWindow::ezDocumentWindow(ezDocumentBase* pDocument)
{
  m_pDocument = pDocument;
  m_sUniqueName = m_pDocument->GetDocumentPath();
  setObjectName(GetUniqueName());

  Constructor();

  ezDocumentManagerBase::s_Events.AddEventHandler(ezMakeDelegate(&ezDocumentWindow::DocumentManagerEventHandler, this));
  pDocument->m_EventsOne.AddEventHandler(ezMakeDelegate(&ezDocumentWindow::DocumentEventHandler, this));
}

ezDocumentWindow::ezDocumentWindow(const char* szUniqueName)
{
  m_pDocument = nullptr;
  m_sUniqueName = szUniqueName;
  setObjectName(GetUniqueName());

  Constructor();
}


ezDocumentWindow::~ezDocumentWindow()
{
  ezUIServices::s_Events.RemoveEventHandler(ezMakeDelegate(&ezDocumentWindow::UIServicesEventHandler, this));

  s_AllDocumentWindows.RemoveSwap(this);

  if (s_AllDocumentWindows.IsEmpty())
  {
    ezActionMapManager::UnregisterActionMap("DocumentWindowTabMenu");
  }

  if (m_pDocument)
  {
    m_pDocument->m_EventsOne.RemoveEventHandler(ezMakeDelegate(&ezDocumentWindow::DocumentEventHandler, this));
    ezDocumentManagerBase::s_Events.RemoveEventHandler(ezMakeDelegate(&ezDocumentWindow::DocumentManagerEventHandler, this));
  }
}

void ezDocumentWindow::SetVisibleInContainer(bool bVisible)
{
  if (m_bIsVisibleInContainer == bVisible)
    return;

  m_bIsVisibleInContainer = bVisible;
  InternalVisibleInContainerChanged(bVisible);

  if (m_bIsVisibleInContainer)
  {
    // if the window is now visible, immediately do a redraw and trigger the timers
    m_bIsDrawingATM = true;
    InternalRedraw();
    m_bIsDrawingATM = false;

    if (m_iTargetFramerate != 0)
      TriggerRedraw();
  }
}

void ezDocumentWindow::SetTargetFramerate(ezInt16 iTargetFPS)
{
  if (m_iTargetFramerate == iTargetFPS)
    return;

  m_iTargetFramerate = iTargetFPS;

  if (m_iTargetFramerate != 0)
    TriggerRedraw();
}

void ezDocumentWindow::TriggerRedraw(float fLastFrameTimeMS)
{
  if (m_bRedrawIsTriggered)
    return;

  // to not set up the timer while we are drawing, this could lead to recursive drawing, which will fail
  if (m_bIsDrawingATM)
  {
    // just store that we got a redraw request while drawing
    m_bTriggerRedrawQueued = true;
    return;
  }

  
  m_bRedrawIsTriggered = true;
  m_bTriggerRedrawQueued = false;

  float fDelay = 1000.0f / 25.0f;

  if (m_iTargetFramerate > 0)
    fDelay = (1000.0f / m_iTargetFramerate);

  if (m_iTargetFramerate < 0)
    fDelay = 0.0f;

  // Subtract the time it took to render the last frame.
  fDelay -= fLastFrameTimeMS;
  fDelay = ezMath::Max(fDelay, 0.0f);

  //ezLog::Info("FT: %.3f, delay: %.3f", fLastFrameTimeMS, fDelay);

  QTimer::singleShot((ezInt32)ezMath::Floor(fDelay), this, SLOT(SlotRedraw()));
}

void ezDocumentWindow::SlotRedraw()
{
  EZ_ASSERT_DEV(!m_bIsDrawingATM, "Implementation error");
  ezTime startTime = ezTime::Now();

  m_bRedrawIsTriggered = false;

  // if our window is not visible, interrupt the redrawing, and do nothing
  if (!m_bIsVisibleInContainer)
    return;

  m_bIsDrawingATM = true;
  InternalRedraw();
  m_bIsDrawingATM = false;

  // immediately trigger the next redraw, if a constant framerate is desired
  if (m_iTargetFramerate != 0 || m_bTriggerRedrawQueued)
  {
    ezTime endTime = ezTime::Now();
    TriggerRedraw((endTime - startTime).GetMilliseconds());
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

void ezDocumentWindow::UIServicesEventHandler(const ezUIServices::Event& e)
{
  switch (e.m_Type)
  {
  case ezUIServices::Event::Type::ShowDocumentStatusBarText:
    {
      if (statusBar() == nullptr)
        setStatusBar(new QStatusBar());

      statusBar()->setHidden(e.m_sText.IsEmpty());
      statusBar()->showMessage(QString::fromUtf8(e.m_sText.GetData()), (int) e.m_Time.GetMilliseconds());
    }
    break;
  }
}

ezString ezDocumentWindow::GetDisplayNameShort() const
{
  ezStringBuilder s = GetDisplayName();
  s = s.GetFileName();

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
  if (!m_pContainerWindow || !m_pContainerWindow->m_bWindowLayoutRestored)
  {
    // if the container window has not yet done its own resize (race condition in the timers)
    // then delay our own restore a bit more, to ensure that this window only restores its size
    // inside a correctly resized parent window

    /// \todo Remove the warning, once I am sure this has been the cause for window restore issues
    ezLog::Warning("Race condition with container window detected: Document WindowLayout restore deferred.");
    ScheduleRestoreWindowLayout();
    return;
  }

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

    ezUIServices::MessageBoxStatus(res, s, s2);

    if (res.m_Result.Failed())
    {
      statusBar()->showMessage("Failed to save document", 10000);
      return res;
    }
  }

  statusBar()->showMessage("Document saved", 5000);
  return ezStatus(EZ_SUCCESS);

}

bool ezDocumentWindow::CanCloseWindow()
{
  return InternalCanCloseWindow();
}

bool ezDocumentWindow::InternalCanCloseWindow()
{
  setFocus();
  clearFocus();

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
  QMetaObject::invokeMethod(this, "SlotQueuedDelete", Qt::ConnectionType::QueuedConnection);
}

void ezDocumentWindow::SlotQueuedDelete()
{
  setFocus();
  clearFocus();

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
  e.m_Type = Event::Type::WindowClosing;
  s_Events.Broadcast(e);

  InternalDeleteThis();

  e.m_Type = Event::Type::WindowClosed;
  s_Events.Broadcast(e);
}

void ezDocumentWindow::InternalCloseDocumentWindow()
{
}

void ezDocumentWindow::EnsureVisible()
{
  m_pContainerWindow->EnsureVisible(this);
}

void ezDocumentWindow::RequestWindowTabContextMenu(const QPoint& GlobalPos)
{
  ezMenuActionMapView menu(nullptr);

  ezActionContext context;
  context.m_sMapping = "DocumentWindowTabMenu";
  context.m_pDocument = GetDocument();
  context.m_pWindow = this;
  menu.SetActionContext(context);

  menu.exec(GlobalPos);
}

ezDocumentWindow* ezDocumentWindow::FindWindowByDocument(const ezDocumentBase* pDocument)
{
  for (auto pWnd : s_AllDocumentWindows)
  {
    if (pWnd->GetDocument() == pDocument)
      return pWnd;
  }

  return nullptr;
}

ezString ezDocumentWindow::GetWindowIcon() const
{
  if (GetDocument() != nullptr)
    return GetDocument()->GetDocumentTypeDescriptor().m_sIcon;

  return ":/GuiFoundation/Icons/ezEditor16.png";
}
