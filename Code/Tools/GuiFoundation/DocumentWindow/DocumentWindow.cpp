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

ezEvent<const ezQtDocumentWindow::Event&> ezQtDocumentWindow::s_Events;
ezDynamicArray<ezQtDocumentWindow*> ezQtDocumentWindow::s_AllDocumentWindows;

void ezQtDocumentWindow::Constructor()
{
  if (s_AllDocumentWindows.IsEmpty())
  {
    ezActionMapManager::RegisterActionMap("DocumentWindowTabMenu");
    ezDocumentActions::MapActions("DocumentWindowTabMenu", "", false);
  }

  s_AllDocumentWindows.PushBack(this);

  setStatusBar(new QStatusBar());

  m_uiWindowIndex = 0;
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

  ezUIServices::s_Events.AddEventHandler(ezMakeDelegate(&ezQtDocumentWindow::UIServicesEventHandler, this));
}

ezQtDocumentWindow::ezQtDocumentWindow(ezDocument* pDocument)
{
  m_pDocument = pDocument;
  m_sUniqueName = m_pDocument->GetDocumentPath();
  setObjectName(GetUniqueName());

  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezQtDocumentWindow::DocumentManagerEventHandler, this));
  pDocument->m_EventsOne.AddEventHandler(ezMakeDelegate(&ezQtDocumentWindow::DocumentEventHandler, this));

  Constructor();
}

ezQtDocumentWindow::ezQtDocumentWindow(const char* szUniqueName)
{
  m_pDocument = nullptr;
  m_sUniqueName = szUniqueName;
  setObjectName(GetUniqueName());

  Constructor();
}


ezQtDocumentWindow::~ezQtDocumentWindow()
{
  ezUIServices::s_Events.RemoveEventHandler(ezMakeDelegate(&ezQtDocumentWindow::UIServicesEventHandler, this));

  s_AllDocumentWindows.RemoveSwap(this);

  if (s_AllDocumentWindows.IsEmpty())
  {
    ezActionMapManager::UnregisterActionMap("DocumentWindowTabMenu");
  }

  if (m_pDocument)
  {
    m_pDocument->m_EventsOne.RemoveEventHandler(ezMakeDelegate(&ezQtDocumentWindow::DocumentEventHandler, this));
    ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezQtDocumentWindow::DocumentManagerEventHandler, this));
  }
}

void ezQtDocumentWindow::SetVisibleInContainer(bool bVisible)
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

void ezQtDocumentWindow::SetTargetFramerate(ezInt16 iTargetFPS)
{
  if (m_iTargetFramerate == iTargetFPS)
    return;

  m_iTargetFramerate = iTargetFPS;

  if (m_iTargetFramerate != 0)
    TriggerRedraw();
}

void ezQtDocumentWindow::TriggerRedraw(float fLastFrameTimeMS)
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

void ezQtDocumentWindow::SlotRedraw()
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

void ezQtDocumentWindow::DocumentEventHandler(const ezDocumentEvent& e)
{
  switch (e.m_Type)
  {
  case ezDocumentEvent::Type::ModifiedChanged:
    {
      Event dwe;
      dwe.m_pWindow = this;
      dwe.m_Type = Event::Type::WindowDecorationChanged;
      s_Events.Broadcast(dwe);
    }
    break;

  case ezDocumentEvent::Type::EnsureVisible:
    {
      EnsureVisible();
    }
    break;

  case ezDocumentEvent::Type::DocumentStatusMsg:
    {
      ShowStatusBarMsgNoArgs(e.m_szStatusMsg);
    }
    break;
  }
}

void ezQtDocumentWindow::DocumentManagerEventHandler(const ezDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
  case ezDocumentManager::Event::Type::DocumentClosing:
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

void ezQtDocumentWindow::UIServicesEventHandler(const ezUIServices::Event& e)
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

ezString ezQtDocumentWindow::GetDisplayNameShort() const
{
  ezStringBuilder s = GetDisplayName();
  s = s.GetFileName();

  if (m_pDocument && m_pDocument->IsModified())
    s.Append('*');

  return s;
}

void ezQtDocumentWindow::FinishWindowCreation()
{
  ScheduleRestoreWindowLayout();
}

void ezQtDocumentWindow::ScheduleRestoreWindowLayout()
{
  QTimer::singleShot(0, this, SLOT(SlotRestoreLayout()));
}

void ezQtDocumentWindow::SlotRestoreLayout()
{
  RestoreWindowLayout();
}

void ezQtDocumentWindow::SaveWindowLayout()
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

void ezQtDocumentWindow::RestoreWindowLayout()
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

ezStatus ezQtDocumentWindow::SaveDocument()
{
  if (m_pDocument)
  {
    {
      if (m_pDocument->GetUnknownObjectTypeInstances() > 0)
      {
        if (ezUIServices::MessageBoxQuestion("Warning! This document contained unknown object types that could not be loaded. Saving the document means those objects will get lost permanently.\n\nDo you really want to save this document?",
                                             QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::No) != QMessageBox::StandardButton::Yes)
          return ezStatus(EZ_SUCCESS); // failed successfully
      }
    }

    ezStatus res = m_pDocument->SaveDocument();


    ezStringBuilder s, s2;
    s.Format("Failed to save document:\n'%s'", m_pDocument->GetDocumentPath());
    s2.Format("Successfully saved document:\n'%s'", m_pDocument->GetDocumentPath());

    ezUIServices::MessageBoxStatus(res, s, s2);

    if (res.m_Result.Failed())
    {
      ShowStatusBarMsg("Failed to save document");
      return res;
    }
  }

  ShowStatusBarMsg("Document saved");
  return ezStatus(EZ_SUCCESS);

}

void ezQtDocumentWindow::ShowStatusBarMsgNoArgs(const char* szText)
{
  if (statusBar() == nullptr)
    setStatusBar(new QStatusBar());

  statusBar()->showMessage(QString::fromUtf8(szText), 5000);
}

void ezQtDocumentWindow::ShowStatusBarMsg(const char* szText, ...)
{
  ezStringBuilder sText;
  va_list args;
  va_start(args, szText);
  sText.FormatArgs(szText, args);
  va_end(args);

  ShowStatusBarMsgNoArgs(sText);
}


bool ezQtDocumentWindow::CanCloseWindow()
{
  return InternalCanCloseWindow();
}

bool ezQtDocumentWindow::InternalCanCloseWindow()
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

void ezQtDocumentWindow::CloseDocumentWindow()
{
  QMetaObject::invokeMethod(this, "SlotQueuedDelete", Qt::ConnectionType::QueuedConnection);
}

void ezQtDocumentWindow::SlotQueuedDelete()
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

void ezQtDocumentWindow::ShutdownDocumentWindow()
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

void ezQtDocumentWindow::InternalCloseDocumentWindow()
{
}

void ezQtDocumentWindow::EnsureVisible()
{
  m_pContainerWindow->EnsureVisible(this);
}

void ezQtDocumentWindow::RequestWindowTabContextMenu(const QPoint& GlobalPos)
{
  ezMenuActionMapView menu(nullptr);

  ezActionContext context;
  context.m_sMapping = "DocumentWindowTabMenu";
  context.m_pDocument = GetDocument();
  context.m_pWindow = this;
  menu.SetActionContext(context);

  menu.exec(GlobalPos);
}

ezQtDocumentWindow* ezQtDocumentWindow::FindWindowByDocument(const ezDocument* pDocument)
{
  for (auto pWnd : s_AllDocumentWindows)
  {
    if (pWnd->GetDocument() == pDocument)
      return pWnd;
  }

  return nullptr;
}

ezString ezQtDocumentWindow::GetWindowIcon() const
{
  if (GetDocument() != nullptr)
    return GetDocument()->GetDocumentTypeDescriptor().m_sIcon;

  return ":/GuiFoundation/Icons/ezEditor16.png";
}
