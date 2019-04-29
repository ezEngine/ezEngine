#include <GuiFoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/ActionViews/MenuActionMapView.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ContainerWindow/ContainerWindow.moc.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QLabel>
#include <QMessageBox>
#include <QSettings>
#include <QStatusBar>
#include <QTimer>
#include <ToolsFoundation/Document/Document.h>

ezEvent<const ezQtDocumentWindowEvent&> ezQtDocumentWindow::s_Events;
ezDynamicArray<ezQtDocumentWindow*> ezQtDocumentWindow::s_AllDocumentWindows;

void ezQtDocumentWindow::Constructor()
{
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

  ezQtMenuBarActionMapView* pMenuBar = new ezQtMenuBarActionMapView(this);
  setMenuBar(pMenuBar);

  ezInt32 iContainerWindowIndex = ezToolsProject::SuggestContainerWindow(m_pDocument);
  ezQtContainerWindow* pContainer = ezQtContainerWindow::GetOrCreateContainerWindow(iContainerWindowIndex);
  pContainer->MoveDocumentWindowToContainer(this);

  ezQtUiServices::s_Events.AddEventHandler(ezMakeDelegate(&ezQtDocumentWindow::UIServicesEventHandler, this));
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
  ezQtUiServices::s_Events.RemoveEventHandler(ezMakeDelegate(&ezQtDocumentWindow::UIServicesEventHandler, this));

  s_AllDocumentWindows.RemoveAndSwap(this);

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

    // \todo While testing with Kraut it had to be like this (with comments), to not crash:
    /*
    //m_bIsDrawingATM = true;
    //InternalRedraw();
    //m_bIsDrawingATM = false;

    //if (m_iTargetFramerate != 0)
      TriggerRedraw();
    */
    /*
        m_bIsDrawingATM = true;
        InternalRedraw();
        m_bIsDrawingATM = false;

        if (m_iTargetFramerate != 0)
          TriggerRedraw();*/
    SlotRedraw();
  }
}

void ezQtDocumentWindow::SetTargetFramerate(ezInt16 iTargetFPS)
{
  if (m_iTargetFramerate == iTargetFPS)
    return;

  m_iTargetFramerate = iTargetFPS;

  if (m_iTargetFramerate != 0)
    SlotRedraw();
}

void ezQtDocumentWindow::TriggerRedraw(ezTime LastFrameTime)
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

  ezTime delay = ezTime::Milliseconds(1000.0f / 25.0f);

  int iTargetFramerate = m_iTargetFramerate;

  // if the application does not have focus, drastically reduce the update rate to limit CPU draw etc.
  if (QApplication::activeWindow() == nullptr)
    iTargetFramerate = ezMath::Min(10, m_iTargetFramerate / 4);

  if (iTargetFramerate > 0)
    delay = ezTime::Milliseconds(1000.0f / iTargetFramerate);

  if (iTargetFramerate < 0)
    delay.SetZero();

  // Subtract the time it took to render the last frame.
  delay -= LastFrameTime;
  delay = ezMath::Max(delay, ezTime::Zero());

  // ezLog::Info("FT: {0}, delay: {1}", ezArgF(fLastFrameTimeMS, 3), ezArgF(fDelay, 3));

  QTimer::singleShot((ezInt32)ezMath::Floor(delay.GetMilliseconds()), this, SLOT(SlotRedraw()));
}

void ezQtDocumentWindow::SlotRedraw()
{
  {
    ezQtDocumentWindowEvent e;
    e.m_Type = ezQtDocumentWindowEvent::Type::BeforeRedraw;
    e.m_pWindow = this;
    s_Events.Broadcast(e, 1);
  }

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
    const ezTime endTime = ezTime::Now();
    TriggerRedraw(endTime - startTime);
  }
}

void ezQtDocumentWindow::DocumentEventHandler(const ezDocumentEvent& e)
{
  switch (e.m_Type)
  {
    case ezDocumentEvent::Type::ModifiedChanged:
    {
      ezQtDocumentWindowEvent dwe;
      dwe.m_pWindow = this;
      dwe.m_Type = ezQtDocumentWindowEvent::Type::WindowDecorationChanged;
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
      ShowTemporaryStatusBarMsg(e.m_szStatusMsg);
    }
    break;

    default:
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

    default:
      break;
  }
}

void ezQtDocumentWindow::UIServicesEventHandler(const ezQtUiServices::Event& e)
{
  switch (e.m_Type)
  {
    case ezQtUiServices::Event::Type::ShowDocumentStatusBarText:
    {
      if (statusBar() == nullptr)
        setStatusBar(new QStatusBar());

      statusBar()->setHidden(e.m_sText.IsEmpty());
      statusBar()->showMessage(QString::fromUtf8(e.m_sText.GetData()), (int)e.m_Time.GetMilliseconds());
    }
    break;

    default:
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
  // This is a workaround for newer Qt versions (5.13 or so) that seem to change the state of QDockWidgets to "closed" once the parent QMainWindow
  // gets the closeEvent, even though they still exist and the QMainWindow is not yet deleted.
  // Previously this function was called multiple times, including once after the QMainWindow got its closeEvent, which would then save a corrupted
  // state. Therefore, once the parent ezQtContainerWindow gets the closeEvent, we now prevent further saving of the window layout.
  if (!m_bAllowSaveWindowLayout)
    return;

  const bool bMaximized = isMaximized();

  if (bMaximized)
    showNormal();

  ezStringBuilder sGroup;
  sGroup.Format("DocumentWnd_{0}", GetWindowLayoutGroupName());

  QSettings Settings;
  Settings.beginGroup(QString::fromUtf8(sGroup));
  {
    // All other properties are defined by the outer container window.
    Settings.setValue("WindowState", saveState());
  }
  Settings.endGroup();
}

void ezQtDocumentWindow::RestoreWindowLayout()
{
  ezQtScopedUpdatesDisabled _(this);

  ezStringBuilder sGroup;
  sGroup.Format("DocumentWnd_{0}", GetWindowLayoutGroupName());

  QSettings Settings;
  Settings.beginGroup(QString::fromUtf8(sGroup));
  {
    restoreState(Settings.value("WindowState", saveState()).toByteArray());
  }
  Settings.endGroup();

  qApp->processEvents();
}

void ezQtDocumentWindow::DisableWindowLayoutSaving()
{
  m_bAllowSaveWindowLayout = false;
}

ezStatus ezQtDocumentWindow::SaveDocument()
{
  if (m_pDocument)
  {
    {
      if (m_pDocument->GetUnknownObjectTypeInstances() > 0)
      {
        if (ezQtUiServices::MessageBoxQuestion("Warning! This document contained unknown object types that could not be loaded. Saving the "
                                               "document means those objects will get lost permanently.\n\nDo you really want to save this "
                                               "document?",
                                               QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No,
                                               QMessageBox::StandardButton::No) != QMessageBox::StandardButton::Yes)
          return ezStatus(EZ_SUCCESS); // failed successfully
      }
    }

    ezStatus res = m_pDocument->SaveDocument();

    ezStringBuilder s, s2;
    s.Format("Failed to save document:\n'{0}'", m_pDocument->GetDocumentPath());
    s2.Format("Successfully saved document:\n'{0}'", m_pDocument->GetDocumentPath());

    ezQtUiServices::MessageBoxStatus(res, s, s2);

    if (res.m_Result.Failed())
    {
      ShowTemporaryStatusBarMsg("Failed to save document");
      return res;
    }

    ShowTemporaryStatusBarMsg("Document saved");
  }

  return ezStatus(EZ_SUCCESS);
}

void ezQtDocumentWindow::ShowTemporaryStatusBarMsg(const ezFormatString& sMsg)
{
  if (statusBar() == nullptr)
    setStatusBar(new QStatusBar());

  ezStringBuilder tmp;
  statusBar()->showMessage(QString::fromUtf8(sMsg.GetText(tmp)), 5000);
}


void ezQtDocumentWindow::SetPermanentStatusBarMsg(const ezFormatString& sText)
{
  if (statusBar() == nullptr)
    setStatusBar(new QStatusBar());

  if (m_pPermanentStatusMsg == nullptr)
  {
    m_pPermanentStatusMsg = new QLabel(statusBar());
    statusBar()->insertWidget(0, m_pPermanentStatusMsg);
  }

  ezStringBuilder tmp;
  m_pPermanentStatusMsg->setText(QString::fromUtf8(sText.GetText(tmp)));
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
    QMessageBox::StandardButton res =
        QMessageBox::question(this, QLatin1String("ezEditor"), QLatin1String("Save before closing?"),
                              QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No | QMessageBox::StandardButton::Cancel,
                              QMessageBox::StandardButton::Cancel);

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

  ezQtDocumentWindowEvent e;
  e.m_pWindow = this;
  e.m_Type = ezQtDocumentWindowEvent::Type::WindowClosing;
  s_Events.Broadcast(e);

  InternalDeleteThis();

  e.m_Type = ezQtDocumentWindowEvent::Type::WindowClosed;
  s_Events.Broadcast(e);
}

void ezQtDocumentWindow::InternalCloseDocumentWindow() {}

void ezQtDocumentWindow::EnsureVisible()
{
  m_pContainerWindow->EnsureVisible(this);
}

void ezQtDocumentWindow::RequestWindowTabContextMenu(const QPoint& GlobalPos)
{
  ezQtMenuActionMapView menu(nullptr);

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

ezQtContainerWindow* ezQtDocumentWindow::GetContainerWindow() const
{
  return m_pContainerWindow;
}

ezString ezQtDocumentWindow::GetWindowIcon() const
{
  if (GetDocument() != nullptr)
    return GetDocument()->GetDocumentTypeDescriptor()->m_sIcon;

  return ":/GuiFoundation/Icons/ezEditor16.png";
}
