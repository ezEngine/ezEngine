#include <PCH.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/InputContexts/EditorInputContext.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <QPaintEvent>
#include <QPushButton>
#include <QHBoxLayout>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <Preferences/EditorPreferences.h>

ezUInt32 ezQtEngineViewWidget::s_uiNextViewID = 0;

ezQtEngineViewWidget::InteractionContext ezQtEngineViewWidget::s_InteractionContext;

void ezObjectPickingResult::Reset()
{
  m_PickedComponent = ezUuid();
  m_PickedObject = ezUuid();
  m_PickedOther = ezUuid();
  m_uiPartIndex = 0;
  m_vPickedPosition.SetZero();
  m_vPickedNormal.SetZero();
  m_vPickingRayStart.SetZero();
}

////////////////////////////////////////////////////////////////////////
// ezQtEngineViewWidget public functions
////////////////////////////////////////////////////////////////////////

ezQtEngineViewWidget::ezQtEngineViewWidget(QWidget* pParent, ezQtEngineDocumentWindow* pDocumentWindow, ezEngineViewConfig* pViewConfig)
  : QWidget(pParent)
  , m_pDocumentWindow(pDocumentWindow)
  , m_pViewConfig(pViewConfig)
{
  m_pRestartButtonLayout = nullptr;
  m_pRestartButton = nullptr;

  setFocusPolicy(Qt::FocusPolicy::StrongFocus);
  //setAttribute(Qt::WA_OpaquePaintEvent);
  setAutoFillBackground(false);
  setMouseTracking(true);
  setMinimumSize(64, 64); // prevent the window from becoming zero sized, otherwise the rendering code may crash

  setAttribute(Qt::WA_PaintOnScreen, true);
  setAttribute(Qt::WA_NativeWindow, true);
  setAttribute(Qt::WA_NoBackground);
  setAttribute(Qt::WA_NoSystemBackground);

  installEventFilter(this);

  m_bUpdatePickingData = false;
  m_bInDragAndDropOperation = false;

  m_uiViewID = s_uiNextViewID;
  ++s_uiNextViewID;

  m_fCameraLerp = 1.0f;
  m_fCameraTargetFovOrDim = 70.0f;

  ezEditorEngineProcessConnection::s_Events.AddEventHandler(ezMakeDelegate(&ezQtEngineViewWidget::EngineViewProcessEventHandler, this));

  if (ezEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    ShowRestartButton(true);
}


ezQtEngineViewWidget::~ezQtEngineViewWidget()
{
  ezEditorEngineProcessConnection::s_Events.RemoveEventHandler(ezMakeDelegate(&ezQtEngineViewWidget::EngineViewProcessEventHandler, this));

  ezViewDestroyedMsgToEngine msg;
  msg.m_uiViewID = GetViewID();
  m_pDocumentWindow->GetDocument()->SendMessageToEngine(&msg);
  m_pDocumentWindow->RemoveViewWidget(this);
}

void ezQtEngineViewWidget::SyncToEngine()
{
  ezViewRedrawMsgToEngine cam;
  cam.m_uiRenderMode = m_pViewConfig->m_RenderMode;
  cam.m_CameraUsageHint = m_pViewConfig->m_CameraUsageHint;

  float fov = m_pViewConfig->m_Camera.GetFovOrDim();
  if (m_pViewConfig->m_Camera.IsPerspective())
  {
    ezEditorPreferencesUser* pPref = ezPreferences::QueryPreferences<ezEditorPreferencesUser>();
    fov = pPref->m_fPerspectiveFieldOfView;
  }

  cam.m_uiViewID = GetViewID();
  cam.m_fNearPlane = m_pViewConfig->m_Camera.GetNearPlane();
  cam.m_fFarPlane = m_pViewConfig->m_Camera.GetFarPlane();
  cam.m_iCameraMode = (ezInt8)m_pViewConfig->m_Camera.GetCameraMode();
  cam.m_bUseCameraTransformOnDevice = m_pViewConfig->m_bUseCameraTransformOnDevice;
  cam.m_fFovOrDim = fov;
  cam.m_vDirForwards = m_pViewConfig->m_Camera.GetCenterDirForwards();
  cam.m_vDirUp = m_pViewConfig->m_Camera.GetCenterDirUp();
  cam.m_vDirRight = m_pViewConfig->m_Camera.GetCenterDirRight();
  cam.m_vPosition = m_pViewConfig->m_Camera.GetCenterPosition();
  cam.m_ViewMatrix = m_pViewConfig->m_Camera.GetViewMatrix();
  m_pViewConfig->m_Camera.GetProjectionMatrix((float)width() / (float)height(), cam.m_ProjMatrix);

  cam.m_uiHWND = (ezUInt64)(winId());
  cam.m_uiWindowWidth = width() * this->devicePixelRatio();
  cam.m_uiWindowHeight = height() * this->devicePixelRatio();
  cam.m_bUpdatePickingData = m_bUpdatePickingData;
  cam.m_bEnablePickingSelected = IsPickingAgainstSelectionAllowed() && (!ezEditorInputContext::IsAnyInputContextActive() || ezEditorInputContext::GetActiveInputContext()->IsPickingSelectedAllowed());

  m_pDocumentWindow->GetEditorEngineConnection()->SendMessage(&cam);
}


void ezQtEngineViewWidget::GetCameraMatrices(ezMat4& out_ViewMatrix, ezMat4& out_ProjectionMatrix) const
{
  out_ViewMatrix = m_pViewConfig->m_Camera.GetViewMatrix();
  m_pViewConfig->m_Camera.GetProjectionMatrix((float)width() / (float)height(), out_ProjectionMatrix);
}

void ezQtEngineViewWidget::UpdateCameraInterpolation()
{
  if (m_fCameraLerp >= 1.0f)
    return;

  const ezTime tNow = ezTime::Now();
  const ezTime tDiff = tNow - m_LastCameraUpdate;
  m_LastCameraUpdate = tNow;

  m_fCameraLerp += tDiff.GetSeconds() * 2.0f;

  if (m_fCameraLerp >= 1.0f)
    m_fCameraLerp = 1.0f;

  ezCamera& cam = m_pViewConfig->m_Camera;

  const float fLerpValue = ezMath::Sin(ezAngle::Degree(90.0f * m_fCameraLerp));

  ezQuat qRot, qRotFinal;
  qRot.SetShortestRotation(m_vCameraStartDirection, m_vCameraTargetDirection);
  qRotFinal.SetSlerp(ezQuat::IdentityQuaternion(), qRot, fLerpValue);

  const ezVec3 vNewDirection = qRotFinal * m_vCameraStartDirection;
  const ezVec3 vNewPosition = ezMath::Lerp(m_vCameraStartPosition, m_vCameraTargetPosition, fLerpValue);
  const float fNewFovOrDim = ezMath::Lerp(m_fCameraStartFovOrDim, m_fCameraTargetFovOrDim, fLerpValue);

  /// \todo Hard coded up vector
  cam.LookAt(vNewPosition, vNewPosition + vNewDirection, m_vCameraUp);
  cam.SetCameraMode(cam.GetCameraMode(), fNewFovOrDim, cam.GetNearPlane(), cam.GetFarPlane());
}

void ezQtEngineViewWidget::InterpolateCameraTo(const ezVec3& vPosition, const ezVec3& vDirection, float fFovOrDim, const ezVec3* pNewUpDirection)
{
  m_vCameraStartPosition = m_pViewConfig->m_Camera.GetPosition();
  m_vCameraTargetPosition = vPosition;

  m_vCameraStartDirection = m_pViewConfig->m_Camera.GetCenterDirForwards();
  m_vCameraTargetDirection = vDirection;

  if (pNewUpDirection)
    m_vCameraUp = *pNewUpDirection;
  else
    m_vCameraUp = m_pViewConfig->m_Camera.GetCenterDirUp();

  m_vCameraStartDirection.Normalize();
  m_vCameraTargetDirection.Normalize();
  m_vCameraUp.Normalize();


  m_fCameraStartFovOrDim = m_pViewConfig->m_Camera.GetFovOrDim();

  if (fFovOrDim > 0.0f)
    m_fCameraTargetFovOrDim = fFovOrDim;


  EZ_ASSERT_DEV(m_fCameraTargetFovOrDim > 0, "Invalid FOV or ortho dimension");

  if (m_vCameraStartPosition == m_vCameraTargetPosition &&
      m_vCameraStartDirection == m_vCameraTargetDirection &&
      m_fCameraStartFovOrDim == m_fCameraTargetFovOrDim)
    return;

  m_LastCameraUpdate = ezTime::Now();

  m_fCameraLerp = 0.0f;
}

void ezQtEngineViewWidget::OpenContextMenu(QPoint globalPos)
{
  s_InteractionContext.m_pLastHoveredViewWidget = this;
  s_InteractionContext.m_pLastPickingResult = &m_LastPickingResult;

  OnOpenContextMenu(globalPos);
}


const ezObjectPickingResult& ezQtEngineViewWidget::PickObject(ezUInt16 uiScreenPosX, ezUInt16 uiScreenPosY) const
{
  if (!ezEditorEngineProcessConnection::GetSingleton()->IsEngineSetup())
  {
    m_LastPickingResult.Reset();
  }
  else
  {
    ezViewPickingMsgToEngine msg;
    msg.m_uiViewID = GetViewID();
    msg.m_uiPickPosX = uiScreenPosX * devicePixelRatio();
    msg.m_uiPickPosY = uiScreenPosY * devicePixelRatio();

    GetDocumentWindow()->GetDocument()->SendMessageToEngine(&msg);
  }

  return m_LastPickingResult;
}

void ezQtEngineViewWidget::HandleViewMessage(const ezEditorEngineViewMsg* pMsg)
{
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezViewPickingResultMsgToEditor>())
  {
    const ezViewPickingResultMsgToEditor* pFullMsg = static_cast<const ezViewPickingResultMsgToEditor*>(pMsg);

    m_LastPickingResult.m_PickedObject = pFullMsg->m_ObjectGuid;
    m_LastPickingResult.m_PickedComponent = pFullMsg->m_ComponentGuid;
    m_LastPickingResult.m_PickedOther = pFullMsg->m_OtherGuid;
    m_LastPickingResult.m_uiPartIndex = pFullMsg->m_uiPartIndex;
    m_LastPickingResult.m_vPickedPosition = pFullMsg->m_vPickedPosition;
    m_LastPickingResult.m_vPickedNormal = pFullMsg->m_vPickedNormal;
    m_LastPickingResult.m_vPickingRayStart = pFullMsg->m_vPickingRayStartPosition;

    return;
  }

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezViewMarqueePickingResultMsgToEditor>())
  {
    const ezViewMarqueePickingResultMsgToEditor* pFullMsg = static_cast<const ezViewMarqueePickingResultMsgToEditor*>(pMsg);

    HandleMarqueePickingResult(pFullMsg);
    return;
  }
}

////////////////////////////////////////////////////////////////////////
// ezQtEngineViewWidget qt overrides
////////////////////////////////////////////////////////////////////////

bool ezQtEngineViewWidget::eventFilter(QObject* object, QEvent* event)
{
  if (event->type() == QEvent::Type::ShortcutOverride)
  {
    if (ezEditorInputContext::IsAnyInputContextActive())
    {
      // if the active input context does not like other shortcuts,
      // accept this event and thus block further shortcut processing
      // instead Qt will then send a keypress event
      if (ezEditorInputContext::GetActiveInputContext()->GetShortcutsDisabled())
        event->accept();
    }
  }

  return false;
}


void ezQtEngineViewWidget::paintEvent(QPaintEvent* event)
{
  //event->accept();

}

void ezQtEngineViewWidget::resizeEvent(QResizeEvent* event)
{
  m_pDocumentWindow->TriggerRedraw();
}

void ezQtEngineViewWidget::keyPressEvent(QKeyEvent* e)
{
  if (e->isAutoRepeat())
    return;

  // if a context is active, it gets exclusive access to the input data
  if (ezEditorInputContext::IsAnyInputContextActive())
  {
    if (ezEditorInputContext::GetActiveInputContext()->KeyPressEvent(e) == ezEditorInut::WasExclusivelyHandled)
      return;
  }

  if (ezEditorInputContext::IsAnyInputContextActive())
    return;

  // if no context is active, pass the input through in a certain order, until someone handles it
  for (auto pContext : m_InputContexts)
  {
    if (pContext->KeyPressEvent(e) == ezEditorInut::WasExclusivelyHandled)
      return;
  }

  QWidget::keyPressEvent(e);
}

void ezQtEngineViewWidget::keyReleaseEvent(QKeyEvent* e)
{
  if (e->isAutoRepeat())
    return;

  // if a context is active, it gets exclusive access to the input data
  if (ezEditorInputContext::IsAnyInputContextActive())
  {
    if (ezEditorInputContext::GetActiveInputContext()->KeyReleaseEvent(e) == ezEditorInut::WasExclusivelyHandled)
      return;
  }

  if (ezEditorInputContext::IsAnyInputContextActive())
    return;

  // if no context is active, pass the input through in a certain order, until someone handles it
  for (auto pContext : m_InputContexts)
  {
    if (pContext->KeyReleaseEvent(e) == ezEditorInut::WasExclusivelyHandled)
      return;
  }

  QWidget::keyReleaseEvent(e);
}

void ezQtEngineViewWidget::mousePressEvent(QMouseEvent* e)
{
  // if a context is active, it gets exclusive access to the input data
  if (ezEditorInputContext::IsAnyInputContextActive())
  {
    if (ezEditorInputContext::GetActiveInputContext()->MousePressEvent(e) == ezEditorInut::WasExclusivelyHandled)
    {
      e->accept();
      return;
    }
  }

  if (ezEditorInputContext::IsAnyInputContextActive())
  {
    e->accept();
    return;
  }

  // if no context is active, pass the input through in a certain order, until someone handles it
  for (auto pContext : m_InputContexts)
  {
    if (pContext->MousePressEvent(e) == ezEditorInut::WasExclusivelyHandled)
    {
      e->accept();
      return;
    }
  }

  QWidget::mousePressEvent(e);
}

void ezQtEngineViewWidget::mouseReleaseEvent(QMouseEvent* e)
{
  // if a context is active, it gets exclusive access to the input data
  if (ezEditorInputContext::IsAnyInputContextActive())
  {
    if (ezEditorInputContext::GetActiveInputContext()->MouseReleaseEvent(e) == ezEditorInut::WasExclusivelyHandled)
    {
      e->accept();
      return;
    }
  }

  if (ezEditorInputContext::IsAnyInputContextActive())
  {
    e->accept();
    return;
  }

  // if no context is active, pass the input through in a certain order, until someone handles it
  for (auto pContext : m_InputContexts)
  {
    if (pContext->MouseReleaseEvent(e) == ezEditorInut::WasExclusivelyHandled)
    {
      e->accept();
      return;
    }
  }

  QWidget::mouseReleaseEvent(e);
}

void ezQtEngineViewWidget::mouseMoveEvent(QMouseEvent* e)
{
  s_InteractionContext.m_pLastHoveredViewWidget = this;
  s_InteractionContext.m_pLastPickingResult = &m_LastPickingResult;

  // kick off the picking
  PickObject(e->pos().x(), e->pos().y());

  // if a context is active, it gets exclusive access to the input data
  if (ezEditorInputContext::IsAnyInputContextActive())
  {
    if (ezEditorInputContext::GetActiveInputContext()->MouseMoveEvent(e) == ezEditorInut::WasExclusivelyHandled)
    {
      e->accept();
      return;
    }
  }

  if (ezEditorInputContext::IsAnyInputContextActive())
  {
    e->accept();
    return;
  }

  // if no context is active, pass the input through in a certain order, until someone handles it
  for (auto pContext : m_InputContexts)
  {
    if (pContext->MouseMoveEvent(e) == ezEditorInut::WasExclusivelyHandled)
    {
      e->accept();
      return;
    }
  }

  QWidget::mouseMoveEvent(e);
}

void ezQtEngineViewWidget::wheelEvent(QWheelEvent* e)
{
  // if a context is active, it gets exclusive access to the input data
  if (ezEditorInputContext::IsAnyInputContextActive())
  {
    if (ezEditorInputContext::GetActiveInputContext()->WheelEvent(e) == ezEditorInut::WasExclusivelyHandled)
      return;
  }

  if (ezEditorInputContext::IsAnyInputContextActive())
    return;

  // if no context is active, pass the input through in a certain order, until someone handles it
  for (auto pContext : m_InputContexts)
  {
    if (pContext->WheelEvent(e) == ezEditorInut::WasExclusivelyHandled)
      return;
  }

  QWidget::wheelEvent(e);
}

void ezQtEngineViewWidget::focusOutEvent(QFocusEvent* e)
{
  if (ezEditorInputContext::IsAnyInputContextActive())
  {
    ezEditorInputContext::GetActiveInputContext()->FocusLost(false);
    ezEditorInputContext::SetActiveInputContext(nullptr);
  }

  QWidget::focusOutEvent(e);
}


void ezQtEngineViewWidget::dragEnterEvent(QDragEnterEvent* e)
{
  m_bInDragAndDropOperation = true;
}


void ezQtEngineViewWidget::dragLeaveEvent(QDragLeaveEvent* e)
{
  m_bInDragAndDropOperation = false;
}


void ezQtEngineViewWidget::dropEvent(QDropEvent* e)
{
  m_bInDragAndDropOperation = false;
}


////////////////////////////////////////////////////////////////////////
// ezQtEngineViewWidget protected functions
////////////////////////////////////////////////////////////////////////

void ezQtEngineViewWidget::EngineViewProcessEventHandler(const ezEditorEngineProcessConnection::Event& e)
{
  switch (e.m_Type)
  {
  case ezEditorEngineProcessConnection::Event::Type::ProcessCrashed:
    {
      ShowRestartButton(true);
    }
    break;

  case ezEditorEngineProcessConnection::Event::Type::ProcessStarted:
    {
      ShowRestartButton(false);
    }
    break;

  case ezEditorEngineProcessConnection::Event::Type::ProcessShutdown:
    break;
  }
}

void ezQtEngineViewWidget::ShowRestartButton(bool bShow)
{
  ezQtScopedUpdatesDisabled _(this);

  if (m_pRestartButtonLayout == nullptr && bShow == true)
  {
    m_pRestartButtonLayout = new QHBoxLayout(this);
    m_pRestartButtonLayout->setMargin(0);

    setLayout(m_pRestartButtonLayout);

    m_pRestartButton = new QPushButton(this);
    m_pRestartButton->setText("Restart Engine View Process");
    m_pRestartButton->setVisible(ezEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed());
    m_pRestartButton->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    m_pRestartButton->connect(m_pRestartButton, &QPushButton::clicked, this, &ezQtEngineViewWidget::SlotRestartEngineProcess);

    m_pRestartButtonLayout->addWidget(m_pRestartButton);
  }

  if (m_pRestartButton)
  {
    m_pRestartButton->setVisible(bShow);

    if (bShow)
      m_pRestartButton->update();
  }
}


////////////////////////////////////////////////////////////////////////
// ezQtEngineViewWidget private slots
////////////////////////////////////////////////////////////////////////

void ezQtEngineViewWidget::SlotRestartEngineProcess()
{
  ezEditorEngineProcessConnection::GetSingleton()->RestartProcess();
}


////////////////////////////////////////////////////////////////////////
// ezQtViewWidgetContainer
////////////////////////////////////////////////////////////////////////

ezQtViewWidgetContainer::ezQtViewWidgetContainer(QWidget* pParent, ezQtEngineViewWidget* pViewWidget, const char* szToolBarMapping)
  : QWidget(pParent)
{
  setBackgroundRole(QPalette::Base);
  setAutoFillBackground(true);

  m_pLayout = new QVBoxLayout(this);
  m_pLayout->setMargin(1);
  m_pLayout->setSpacing(0);
  setLayout(m_pLayout);

  m_pViewWidget = pViewWidget;
  m_pViewWidget->setParent(this);

  if (!ezStringUtils::IsNullOrEmpty(szToolBarMapping))
  {
    // Add Tool Bar
    ezQtToolBarActionMapView* pToolBar = new ezQtToolBarActionMapView("Toolbar", this);
    ezActionContext context;
    context.m_sMapping = szToolBarMapping;
    context.m_pDocument = pViewWidget->GetDocumentWindow()->GetDocument();
    context.m_pWindow = m_pViewWidget;
    pToolBar->SetActionContext(context);
    m_pLayout->addWidget(pToolBar, 0);
  }

  m_pLayout->addWidget(m_pViewWidget, 1);
}

ezQtViewWidgetContainer::~ezQtViewWidgetContainer()
{

}

ezQtEngineViewWidget::InteractionContext::InteractionContext()
{
  m_pLastHoveredViewWidget = nullptr;
  m_pLastPickingResult = nullptr;
}
