#include <PCH.h>
#include <EditorFramework/DocumentWindow3D/3DViewWidget.moc.h>
#include <EditorFramework/DocumentWindow3D/DocumentWindow3D.moc.h>
#include <EditorFramework/DocumentWindow3D/EditorInputContext.h>

#include <QPaintEvent>
#include <QPushButton>
#include <QHBoxLayout>

ezUInt32 ezEngineViewWidget::s_uiNextViewID = 0;


ezEngineViewWidget::ezEngineViewWidget(QWidget* pParent, ezDocumentWindow3D* pDocumentWindow, ezSceneViewConfig* pViewConfig)
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

  setAttribute(Qt::WA_PaintOnScreen, true);
  setAttribute(Qt::WA_NativeWindow, true);
  setAttribute(Qt::WA_NoBackground);
  setAttribute(Qt::WA_NoSystemBackground);

  installEventFilter(this);

  m_bUpdatePickingData = false;
  m_uiViewID = s_uiNextViewID;
  ++s_uiNextViewID;
  m_pDocumentWindow->m_ViewWidgets.PushBack(this);

  m_fCameraLerp = 1.0f;

  ezEditorEngineProcessConnection::s_Events.AddEventHandler(ezMakeDelegate(&ezEngineViewWidget::EngineViewProcessEventHandler, this));

  if (ezEditorEngineProcessConnection::GetInstance()->IsProcessCrashed())
    ShowRestartButton(true);
}


ezEngineViewWidget::~ezEngineViewWidget()
{
  ezEditorEngineProcessConnection::s_Events.RemoveEventHandler(ezMakeDelegate(&ezEngineViewWidget::EngineViewProcessEventHandler, this));

  ezViewDestroyedMsgToEngine msg;
  msg.m_uiViewID = GetViewID();
  m_pDocumentWindow->SendMessageToEngine(&msg);

  m_pDocumentWindow->m_ViewWidgets.RemoveSwap(this);
}

void ezEngineViewWidget::paintEvent(QPaintEvent* event)
{
  //event->accept();

}

bool ezEngineViewWidget::eventFilter(QObject* object, QEvent* event)
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


void ezEngineViewWidget::SyncToEngine()
{
  ezViewRedrawMsgToEngine cam;
  cam.m_uiRenderMode = m_pViewConfig->m_RenderMode;
  cam.m_uiViewID = GetViewID();
  cam.m_fNearPlane = m_pViewConfig->m_Camera.GetNearPlane();
  cam.m_fFarPlane = m_pViewConfig->m_Camera.GetFarPlane();
  cam.m_iCameraMode = (ezInt8)m_pViewConfig->m_Camera.GetCameraMode();
  cam.m_fFovOrDim = m_pViewConfig->m_Camera.GetFovOrDim();
  cam.m_vDirForwards = m_pViewConfig->m_Camera.GetCenterDirForwards();
  cam.m_vDirUp = m_pViewConfig->m_Camera.GetCenterDirUp();
  cam.m_vDirRight = m_pViewConfig->m_Camera.GetCenterDirRight();
  cam.m_vPosition = m_pViewConfig->m_Camera.GetCenterPosition();
  m_pViewConfig->m_Camera.GetViewMatrix(cam.m_ViewMatrix);
  m_pViewConfig->m_Camera.GetProjectionMatrix((float)width() / (float)height(), cam.m_ProjMatrix);

  cam.m_uiHWND = (ezUInt64)(winId());
  cam.m_uiWindowWidth = width();
  cam.m_uiWindowHeight = height();
  cam.m_bUpdatePickingData = m_bUpdatePickingData;

  m_pDocumentWindow->GetEditorEngineConnection()->SendMessage(&cam);
}


void ezEngineViewWidget::UpdateCameraInterpolation()
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
  cam.LookAt(vNewPosition, vNewPosition + vNewDirection, ezVec3(0.0f, 0.0f, 1.0f));
  cam.SetCameraMode(cam.GetCameraMode(), fNewFovOrDim, cam.GetNearPlane(), cam.GetFarPlane());
}

void ezEngineViewWidget::InterpolateCameraTo(const ezVec3& vPosition, const ezVec3& vDirection, float fFovOrDim)
{
  // prevent restarting this in the middle of a move
  if (m_fCameraLerp < 1.0f)
    return;

  m_vCameraStartPosition = m_pViewConfig->m_Camera.GetPosition();
  m_vCameraTargetPosition = vPosition;

  m_vCameraStartDirection = m_pViewConfig->m_Camera.GetCenterDirForwards();
  m_vCameraTargetDirection = vDirection;

  m_vCameraStartDirection.Normalize();
  m_vCameraTargetDirection.Normalize();

  m_fCameraStartFovOrDim = m_pViewConfig->m_Camera.GetFovOrDim();
  m_fCameraTargetFovOrDim = fFovOrDim;

  if (m_vCameraStartPosition == m_vCameraTargetPosition &&
      m_vCameraStartDirection == m_vCameraTargetDirection &&
      m_fCameraStartFovOrDim == m_fCameraTargetFovOrDim)
    return;

  m_LastCameraUpdate = ezTime::Now();

  m_fCameraLerp = 0.0f;
}

void ezEngineViewWidget::resizeEvent(QResizeEvent* event)
{
  m_pDocumentWindow->TriggerRedraw();
}

void ezEngineViewWidget::keyReleaseEvent(QKeyEvent* e)
{
  // if a context is active, it gets exclusive access to the input data
  if (ezEditorInputContext::IsAnyInputContextActive())
  {
    if (ezEditorInputContext::GetActiveInputContext()->keyReleaseEvent(e))
      return;
  }

  if (ezEditorInputContext::IsAnyInputContextActive())
    return;

  // if no context is active, pass the input through in a certain order, until someone handles it
  for (auto pContext : m_InputContexts)
  {
    if (pContext->keyReleaseEvent(e))
      return;
  }

  QWidget::keyReleaseEvent(e);
}

void ezEngineViewWidget::keyPressEvent(QKeyEvent* e)
{
  // if a context is active, it gets exclusive access to the input data
  if (ezEditorInputContext::IsAnyInputContextActive())
  {
    if (ezEditorInputContext::GetActiveInputContext()->keyPressEvent(e))
      return;
  }

  if (ezEditorInputContext::IsAnyInputContextActive())
    return;

  // if no context is active, pass the input through in a certain order, until someone handles it
  for (auto pContext : m_InputContexts)
  {
    if (pContext->keyPressEvent(e))
      return;
  }

  QWidget::keyPressEvent(e);
}

void ezEngineViewWidget::mousePressEvent(QMouseEvent* e)
{
  // if a context is active, it gets exclusive access to the input data
  if (ezEditorInputContext::IsAnyInputContextActive())
  {
    if (ezEditorInputContext::GetActiveInputContext()->mousePressEvent(e))
      return;
  }

  if (ezEditorInputContext::IsAnyInputContextActive())
    return;

  // if no context is active, pass the input through in a certain order, until someone handles it
  for (auto pContext : m_InputContexts)
  {
    if (pContext->mousePressEvent(e))
      return;
  }

  QWidget::mousePressEvent(e);
}

void ezEngineViewWidget::mouseReleaseEvent(QMouseEvent* e)
{
  // if a context is active, it gets exclusive access to the input data
  if (ezEditorInputContext::IsAnyInputContextActive())
  {
    if (ezEditorInputContext::GetActiveInputContext()->mouseReleaseEvent(e))
      return;
  }

  if (ezEditorInputContext::IsAnyInputContextActive())
    return;

  // if no context is active, pass the input through in a certain order, until someone handles it
  for (auto pContext : m_InputContexts)
  {
    if (pContext->mouseReleaseEvent(e))
      return;
  }

  QWidget::mouseReleaseEvent(e);
}

void ezEngineViewWidget::mouseMoveEvent(QMouseEvent* e)
{
  // if a context is active, it gets exclusive access to the input data
  if (ezEditorInputContext::IsAnyInputContextActive())
  {
    if (ezEditorInputContext::GetActiveInputContext()->mouseMoveEvent(e))
      return;
  }

  if (ezEditorInputContext::IsAnyInputContextActive())
    return;

  // if no context is active, pass the input through in a certain order, until someone handles it
  for (auto pContext : m_InputContexts)
  {
    if (pContext->mouseMoveEvent(e))
      return;
  }

  QWidget::mouseMoveEvent(e);
}

void ezEngineViewWidget::wheelEvent(QWheelEvent* e)
{
  // if a context is active, it gets exclusive access to the input data
  if (ezEditorInputContext::IsAnyInputContextActive())
  {
    if (ezEditorInputContext::GetActiveInputContext()->wheelEvent(e))
      return;
  }

  if (ezEditorInputContext::IsAnyInputContextActive())
    return;

  // if no context is active, pass the input through in a certain order, until someone handles it
  for (auto pContext : m_InputContexts)
  {
    if (pContext->wheelEvent(e))
      return;
  }

  QWidget::wheelEvent(e);
}

void ezEngineViewWidget::focusOutEvent(QFocusEvent* e)
{
  if (ezEditorInputContext::IsAnyInputContextActive())
  {
    ezEditorInputContext::GetActiveInputContext()->FocusLost(false);
    ezEditorInputContext::SetActiveInputContext(nullptr);
  }

  QWidget::focusOutEvent(e);
}


void ezEngineViewWidget::EngineViewProcessEventHandler(const ezEditorEngineProcessConnection::Event& e)
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

void ezEngineViewWidget::ShowRestartButton(bool bShow)
{
  QtScopedUpdatesDisabled _(this);

  if (m_pRestartButtonLayout == nullptr && bShow == true)
  {
    m_pRestartButtonLayout = new QHBoxLayout(this);
    m_pRestartButtonLayout->setMargin(0);

    setLayout(m_pRestartButtonLayout);

    m_pRestartButton = new QPushButton(this);
    m_pRestartButton->setText("Restart Engine View Process");
    m_pRestartButton->setVisible(ezEditorEngineProcessConnection::GetInstance()->IsProcessCrashed());
    m_pRestartButton->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    m_pRestartButton->connect(m_pRestartButton, &QPushButton::clicked, this, &ezEngineViewWidget::SlotRestartEngineProcess);

    m_pRestartButtonLayout->addWidget(m_pRestartButton);
  }

  if (m_pRestartButton)
  {
    m_pRestartButton->setVisible(bShow);

    if (bShow)
      m_pRestartButton->update();
  }
}


void ezEngineViewWidget::SlotRestartEngineProcess()
{
  ezEditorEngineProcessConnection::GetInstance()->RestartProcess();
}
