#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Profiling/Profiling.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/ActionViews/MenuActionMapView.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/QtProxy.moc.h>
#include <GuiFoundation/ContainerWindow/ContainerWindow.moc.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QLabel>
#include <QMessageBox>
#include <QSettings>
#include <QStatusBar>
#include <ToolsFoundation/Document/Document.h>
#include <ads/DockManager.h>
#include <ads/DockWidget.h>

ezEvent<const ezQtDocumentWindowEvent&> ezQtDocumentWindow::s_Events;
ezDynamicArray<ezQtDocumentWindow*> ezQtDocumentWindow::s_AllDocumentWindows;

void ezQtDocumentWindow::Constructor()
{
  m_pDockManager = new ads::CDockManager(this);

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

  ezQtMenuBarActionMapView* pMenuBar = new ezQtMenuBarActionMapView(this);
  setMenuBar(pMenuBar);

  ezToolsProject::SuggestContainerWindow(m_pDocument);
  ezQtContainerWindow* pContainer = ezQtContainerWindow::GetContainerWindow();
  pContainer->AddDocumentWindow(this);

  ezQtUiServices::s_Events.AddEventHandler(ezMakeDelegate(&ezQtDocumentWindow::UIServicesEventHandler, this));
  ezQtUiServices::s_TickEvent.AddEventHandler(ezMakeDelegate(&ezQtDocumentWindow::UIServicesTickEventHandler, this));

  // Automatically restore the saved nested layout for this document window type
  QMetaObject::invokeMethod(this, "SlotRestoreDocumentLayout", Qt::ConnectionType::QueuedConnection);
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
  ezQtUiServices::s_TickEvent.RemoveEventHandler(ezMakeDelegate(&ezQtDocumentWindow::UIServicesTickEventHandler, this));

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
    TriggerRedraw();
    // Make sure the window gains focus as well when it becomes visible so that shortcuts will immediately work.
    setFocus();
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

void ezQtDocumentWindow::TriggerRedraw()
{
  m_bRedrawIsTriggered = true;
}

void ezQtDocumentWindow::UIServicesTickEventHandler(const ezQtUiServices::TickEvent& e)
{
  auto ShouldRender = [&]() -> bool
  {
    if (!m_bIsVisibleInContainer)
      return false;
    if (m_bRedrawIsTriggered)
      return true;

    const ezInt32 iSystemFramerate = static_cast<ezInt32>(ezMath::Round(e.m_fRefreshRate));

    ezInt32 iTargetFramerate = m_iTargetFramerate;
    if (iTargetFramerate <= 0)
      iTargetFramerate = iSystemFramerate;

    // if the application does not have focus, drastically reduce the update rate to limit CPU draw etc.
    if (QApplication::activeWindow() == nullptr)
      iTargetFramerate = ezMath::Max(10, iTargetFramerate / 4);

    // We do not hit the requested framerate directly if the system framerate can't be evenly divided. We will chose the next higher framerate.
    if (iTargetFramerate < iSystemFramerate)
    {
      ezUInt32 mod = ezMath::Max(1u, (ezUInt32)ezMath::Floor(iSystemFramerate / (double)iTargetFramerate));
      if ((e.m_uiFrame % mod) != 0)
        return false;
    }
    return true;
  };

  if (e.m_Type == ezQtUiServices::TickEvent::Type::BeforeFrame)
  {
    if (ShouldRender())
    {
      e.m_uiFrameRequest++;
    }
  }
  else if (e.m_Type == ezQtUiServices::TickEvent::Type::StartFrame)
  {
    if (ShouldRender())
    {
      SlotRedraw();
    }
  }
}

void ezQtDocumentWindow::SlotRedraw()
{
  ezStringBuilder sFilename = ezPathUtils::GetFileName(this->GetUniqueName());
  EZ_PROFILE_SCOPE(sFilename.GetData());
  {
    ezQtDocumentWindowEvent e;
    e.m_Type = ezQtDocumentWindowEvent::Type::BeforeRedraw;
    e.m_pWindow = this;
    s_Events.Broadcast(e, 1);
  }

  // if our window is not visible, interrupt the redrawing, and do nothing
  if (!m_bIsVisibleInContainer)
    return;

  m_bIsDrawingATM = true;
  InternalRedraw();
  m_bIsDrawingATM = false;
  m_bRedrawIsTriggered = false;
}

void ezQtDocumentWindow::DocumentEventHandler(const ezDocumentEvent& e)
{
  switch (e.m_Type)
  {
    case ezDocumentEvent::Type::DocumentRenamed:
    {
      m_sUniqueName = m_pDocument->GetDocumentPath();
      setObjectName(GetUniqueName());
      ezQtContainerWindow* pContainer = ezQtContainerWindow::GetContainerWindow();
      pContainer->DocumentWindowRenamed(this);

      [[fallthrough]];
    }
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
      ShowTemporaryStatusBarMsg(e.m_sStatusMsg);
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
            m_pPermanentGlobalStatusButton->setIcon(QIcon(":/GuiFoundation/Icons/Log.svg"));
            break;

          case ezQtUiServices::Event::Warning:
            pal.setColor(QPalette::WindowText, QColor(255, 100, 0));
            m_pPermanentGlobalStatusButton->setIcon(QIcon(":/GuiFoundation/Icons/Warning.svg"));
            break;

          case ezQtUiServices::Event::Error:
            pal.setColor(QPalette::WindowText, QColor(Qt::red));
            m_pPermanentGlobalStatusButton->setIcon(QIcon(":/GuiFoundation/Icons/Error.svg"));
            break;
        }

        m_pPermanentGlobalStatusButton->setPalette(pal);
        m_pPermanentGlobalStatusButton->setText(QString::fromUtf8(e.m_sText, e.m_sText.GetElementCount()));
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

  // Capture the baseline dock layout state once the window first becomes visible and Qt has had a chance to do its layout adjustments.
  if (m_pDockManager != nullptr && m_InitialDocumentLayoutState.isEmpty())
  {
    QMetaObject::invokeMethod(this, "SlotCaptureInitialLayoutState", Qt::ConnectionType::QueuedConnection);
  }
}

void ezQtDocumentWindow::hideEvent(QHideEvent* event)
{
  QMainWindow::hideEvent(event);
  SetVisibleInContainer(false);
}

bool ezQtDocumentWindow::eventFilter(QObject* obj, QEvent* e)
{
  if (e->type() == QEvent::ShortcutOverride || e->type() == QEvent::KeyPress)
  {
    // This filter is added by ezQtContainerWindow::AddDocumentWindow as that ones is the ony code path that can connect dock container to their content.
    // This filter is necessary as clicking any action in a menu bar sets the focus to the parent CDockWidget at which point further shortcuts would stop working.
    if (qobject_cast<ads::CDockWidget*>(obj))
    {
      QKeyEvent* keyEvent = static_cast<QKeyEvent*>(e);
      if (ezQtProxy::TriggerDocumentAction(m_pDocument, keyEvent, e->type() == QEvent::ShortcutOverride))
        return true;
    }

    // Some central widgets consume the shortcut (or any key press for that matter) instead of passing it up the parent hierarchy. For example a QGraphicsView will forward any key-press to the QGraphicsScene which will consume every event. As a workaround, we overrule the central widget by default when it comes to shortcuts.
    if (obj == centralWidget())
    {
      QKeyEvent* keyEvent = static_cast<QKeyEvent*>(e);
      if (ezQtProxy::TriggerDocumentAction(m_pDocument, keyEvent, e->type() == QEvent::ShortcutOverride))
        return true;
    }
  }

  return false;
}

bool ezQtDocumentWindow::event(QEvent* event)
{
  if (event->type() == QEvent::ShortcutOverride || event->type() == QEvent::KeyPress)
  {
    QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
    if (ezQtProxy::TriggerDocumentAction(m_pDocument, keyEvent, event->type() == QEvent::ShortcutOverride))
      return true;
  }
  return QMainWindow::event(event);
}

void ezQtDocumentWindow::FinishWindowCreation()
{
  if (centralWidget())
    centralWidget()->installEventFilter(this);
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
    s.SetFormat("Failed to save document:\n'{0}'", m_pDocument->GetDocumentPath());
    s2.SetFormat("Successfully saved document:\n'{0}'", m_pDocument->GetDocumentPath());

    ezQtUiServices::MessageBoxStatus(res, s, s2);

    if (res.Failed())
    {
      ShowTemporaryStatusBarMsg("Failed to save document");
      return res;
    }

    ShowTemporaryStatusBarMsg("Document saved");
  }

  return ezStatus(EZ_SUCCESS);
}

void ezQtDocumentWindow::ShowTemporaryStatusBarMsg(const ezFormatString& msg, ezTime duration)
{
  ezStringBuilder tmp;
  statusBar()->showMessage(QString::fromUtf8(msg.GetTextCStr(tmp)), (int)duration.GetMilliseconds());
}


void ezQtDocumentWindow::SetPermanentStatusBarMsg(const ezFormatString& text)
{
  if (!text.IsEmpty())
  {
    // clear temporary message
    statusBar()->clearMessage();
  }

  ezStringBuilder tmp;
  m_pPermanentDocumentStatusText->setText(QString::fromUtf8(text.GetTextCStr(tmp)));
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
  // I guess this is to remove the focus from other widgets like input boxes, such that they may modify the document.
  setFocus();
  clearFocus();

  if (m_pDocument && m_pDocument->IsModified())
  {
    QMessageBox::StandardButton res = ezQtUiServices::MessageBoxQuestion("Save before closing?", QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No | QMessageBox::StandardButton::Cancel, QMessageBox::StandardButton::Cancel);

    if (res == QMessageBox::StandardButton::Cancel)
      return false;

    if (res == QMessageBox::StandardButton::Yes)
    {
      ezStatus err = SaveDocument();

      if (err.Failed())
      {
        ezQtUiServices::GetSingleton()->MessageBoxStatus(err, "Saving the scene failed.");
        return false;
      }
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
    pal.setColor(QPalette::WindowText, ezToQtColor(ezColorScheme::LightUI(ezColorScheme::Red)));
  }
  else if (sNewText.startsWith("Warning:"))
  {
    pal.setColor(QPalette::WindowText, ezToQtColor(ezColorScheme::LightUI(ezColorScheme::Yellow)));
  }
  else if (sNewText.startsWith("Note:"))
  {
    pal.setColor(QPalette::WindowText, ezToQtColor(ezColorScheme::LightUI(ezColorScheme::Blue)));
  }

  statusBar()->setPalette(pal);
}

void ezQtDocumentWindow::SlotRestoreDocumentLayout()
{
  if (m_pDockManager == nullptr)
    return;

  const char* szTypeName = metaObject()->className();

  QSettings settings;
  settings.beginGroup("DocumentWindowLayouts");
  QByteArray state = settings.value(szTypeName).toByteArray();
  settings.endGroup();

  if (!state.isEmpty())
  {
    m_pDockManager->restoreState(state);
    OnAfterDocumentLayoutRestored();
  }

  // The actual capture of the baseline state is triggered from showEvent() once the window is visible and Qt has done its layout adjustments.
}

void ezQtDocumentWindow::SlotCaptureInitialLayoutState()
{
  if (m_pDockManager == nullptr || !m_InitialDocumentLayoutState.isEmpty())
    return;

  // The window may have been hidden again before this queued slot ran. In that case showEvent() will trigger the capture again next time the window becomes visible.
  if (!isVisible())
    return;

  // Capture the baseline state after Qt has done its layout adjustments
  m_InitialDocumentLayoutState = m_pDockManager->saveState();
}

void ezQtDocumentWindow::ShutdownDocumentWindow()
{
  // Auto-save the document layout if it changed
  if (m_pDockManager != nullptr && !m_InitialDocumentLayoutState.isEmpty())
  {
    QByteArray currentState = m_pDockManager->saveState();

    // Only save if the layout actually changed
    if (currentState != m_InitialDocumentLayoutState)
    {
      const char* szTypeName = metaObject()->className();

      QSettings settings;
      settings.beginGroup("DocumentWindowLayouts");
      settings.setValue(szTypeName, currentState);
      settings.endGroup();
    }
  }

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

void ezQtDocumentWindow::RequestWindowTabContextMenu(const QPoint& globalPos)
{
  ezQtMenuActionMapView menu(nullptr);

  ezActionContext context;
  context.m_sMapping = "DocumentWindowTabMenu";
  context.m_pDocument = GetDocument();
  context.m_pWindow = this;
  menu.SetActionContext(context);

  menu.exec(globalPos);
}

ezQtDocumentWindow* ezQtDocumentWindow::FindWindowByDocument(const ezDocument* pDocument)
{
  // Sub-documents never have a window, so go to the main document instead
  pDocument = pDocument->GetMainDocument();

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
