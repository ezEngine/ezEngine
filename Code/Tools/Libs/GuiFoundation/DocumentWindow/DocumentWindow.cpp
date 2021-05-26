#include <GuiFoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/ActionViews/MenuActionMapView.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ContainerWindow/ContainerWindow.moc.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QDockWidget>
#include <QLabel>
#include <QMessageBox>
#include <QSettings>
#include <QStatusBar>
#include <QTimer>
#include <ToolsFoundation/Document/Document.h>

ezEvent<const ezQtDocumentWindowEvent&> ezQtDocumentWindow::s_Events;
ezDynamicArray<ezQtDocumentWindow*> ezQtDocumentWindow::s_AllDocumentWindows;
bool ezQtDocumentWindow::s_bAllowRestoreWindowLayout = true;

void ezQtDocumentWindow::Constructor()
{
  s_AllDocumentWindows.PushBack(this);

  // status bar
  {
    connect(statusBar(), &QStatusBar::messageChanged, this, &ezQtDocumentWindow::OnStatusBarMessageChanged);

    m_pPermanentDocumentStatusText = new QLabel();
    statusBar()->addWidget(m_pPermanentDocumentStatusText, 1);

    m_pPermanentGlobalStatusButton = new QToolButton();
    m_pPermanentGlobalStatusButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_pPermanentGlobalStatusButton->setVisible(false);
    statusBar()->addPermanentWidget(m_pPermanentGlobalStatusButton, 0);

    EZ_VERIFY(connect(m_pPermanentGlobalStatusButton, &QToolButton::clicked, this, &ezQtDocumentWindow::OnPermanentGlobalStatusClicked), "");
  }

  setDockNestingEnabled(true);

  ezQtMenuBarActionMapView* pMenuBar = new ezQtMenuBarActionMapView(this);
  setMenuBar(pMenuBar);

  ezInt32 iContainerWindowIndex = ezToolsProject::SuggestContainerWindow(m_pDocument);
  ezQtContainerWindow* pContainer = ezQtContainerWindow::GetContainerWindow();
  pContainer->AddDocumentWindow(this);

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
    case ezQtUiServices::Event::Type::ShowDocumentTemporaryStatusBarText:
      ShowTemporaryStatusBarMsg(ezFmt(e.m_sText), e.m_Time);
      break;

    case ezQtUiServices::Event::Type::ShowDocumentPermanentStatusBarText:
    {
      if (m_pPermanentGlobalStatusButton)
      {
        QPalette pal = palette();

        switch (e.m_TextType)
        {
          case ezQtUiServices::Event::Info:
            m_pPermanentGlobalStatusButton->setIcon(QIcon(":/GuiFoundation/Icons/Log.png"));
            break;

          case ezQtUiServices::Event::Warning:
            pal.setColor(QPalette::WindowText, QColor(255, 100, 0));
            m_pPermanentGlobalStatusButton->setIcon(QIcon(":/GuiFoundation/Icons/Warning16.png"));
            break;

          case ezQtUiServices::Event::Error:
            pal.setColor(QPalette::WindowText, QColor(Qt::red));
            m_pPermanentGlobalStatusButton->setIcon(QIcon(":/GuiFoundation/Icons/Error16.png"));
            break;
        }

        m_pPermanentGlobalStatusButton->setPalette(pal);
        m_pPermanentGlobalStatusButton->setText(QString::fromUtf8(e.m_sText));
        m_pPermanentGlobalStatusButton->setVisible(!m_pPermanentGlobalStatusButton->text().isEmpty());
      }
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

void ezQtDocumentWindow::showEvent(QShowEvent* event)
{
  QMainWindow::showEvent(event);
  SetVisibleInContainer(true);
}

void ezQtDocumentWindow::hideEvent(QHideEvent* event)
{
  QMainWindow::hideEvent(event);
  SetVisibleInContainer(false);
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
  // This is a workaround for newer Qt versions (5.13 or so) that seem to change the state of QDockWidgets to "closed" once the parent
  // QMainWindow gets the closeEvent, even though they still exist and the QMainWindow is not yet deleted. Previously this function was
  // called multiple times, including once after the QMainWindow got its closeEvent, which would then save a corrupted state. Therefore,
  // once the parent ezQtContainerWindow gets the closeEvent, we now prevent further saving of the window layout.
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
  if (!s_bAllowRestoreWindowLayout)
    return;

  ezQtScopedUpdatesDisabled _(this);

  ezStringBuilder sGroup;
  sGroup.Format("DocumentWnd_{0}", GetWindowLayoutGroupName());

  {
    QSettings Settings;
    Settings.beginGroup(QString::fromUtf8(sGroup));
    {
      restoreState(Settings.value("WindowState", saveState()).toByteArray());
    }
    Settings.endGroup();

    // with certain Qt versions the window state could be saved corrupted
    // if that is the case, make sure that non-closable widgets get restored to be visible
    // otherwise the user would need to delete the serialized state from the registry
    {
      for (QDockWidget* dockWidget : findChildren<QDockWidget*>())
      {
        // not closable means the user can generally not change the visible state -> make sure it is visible
        if (!dockWidget->features().testFlag(QDockWidget::DockWidgetClosable) && dockWidget->isHidden())
        {
          dockWidget->show();
        }
      }
    }
  }

  statusBar()->clearMessage();
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
              QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::No) != QMessageBox::StandardButton::Yes)
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

void ezQtDocumentWindow::ShowTemporaryStatusBarMsg(const ezFormatString& sMsg, ezTime duration)
{
  ezStringBuilder tmp;
  statusBar()->showMessage(QString::fromUtf8(sMsg.GetText(tmp)), (int)duration.GetMilliseconds());
}


void ezQtDocumentWindow::SetPermanentStatusBarMsg(const ezFormatString& sText)
{
  if (!sText.IsEmpty())
  {
    // clear temporary message
    statusBar()->clearMessage();
  }

  ezStringBuilder tmp;
  m_pPermanentDocumentStatusText->setText(QString::fromUtf8(sText.GetText(tmp)));
}

void ezQtDocumentWindow::CreateImageCapture(const char* szOutputPath)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

bool ezQtDocumentWindow::CanCloseWindow()
{
  return InternalCanCloseWindow();
}

bool ezQtDocumentWindow::InternalCanCloseWindow()
{
  // I guess this is to remove the focus from other widgets like input boxes,
  // such that they may modify the document
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

void ezQtDocumentWindow::OnPermanentGlobalStatusClicked(bool)
{
  ezQtUiServices::Event e;
  e.m_Type = ezQtUiServices::Event::ClickedDocumentPermanentStatusBarText;

  ezQtUiServices::GetSingleton()->s_Events.Broadcast(e);
}

void ezQtDocumentWindow::OnStatusBarMessageChanged(const QString& sNewText)
{
  QPalette pal = palette();

  if (sNewText.startsWith("Error:"))
  {
    pal.setColor(QPalette::WindowText, QColor(Qt::red));
  }
  else if (sNewText.startsWith("Warning:"))
  {
    pal.setColor(QPalette::WindowText, QColor(255, 216, 0));
  }
  else if (sNewText.startsWith("Note:"))
  {
    pal.setColor(QPalette::WindowText, QColor(0, 255, 255));
  }

  statusBar()->setPalette(pal);
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
  m_pContainerWindow->EnsureVisible(this).IgnoreResult();
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

  return ":/GuiFoundation/EZ-logo.svg";
}
