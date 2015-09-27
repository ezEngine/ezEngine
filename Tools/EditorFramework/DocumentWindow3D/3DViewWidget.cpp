#include <PCH.h>
#include <EditorFramework/DocumentWindow3D/3DViewWidget.moc.h>
#include <EditorFramework/DocumentWindow3D/DocumentWindow3D.moc.h>
#include <EditorFramework/DocumentWindow3D/EditorInputContext.h>

#include <QPaintEvent>

ezUInt32 ezEngineViewWidget::s_uiNextViewID = 0;

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
  ezViewCameraMsgToEngine cam;
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

  m_pDocumentWindow->GetEditorEngineConnection()->SendMessage(&cam);
}

void ezEngineViewWidget::resizeEvent(QResizeEvent* event)
{
  m_pDocumentWindow->TriggerRedraw();
}

ezEngineViewWidget::ezEngineViewWidget(QWidget* pParent, ezDocumentWindow3D* pDocumentWindow, ezSceneViewConfig* pViewConfig)
  : QWidget(pParent)
  , m_pDocumentWindow(pDocumentWindow)
  , m_pViewConfig(pViewConfig)
{
  setFocusPolicy(Qt::FocusPolicy::StrongFocus);
  //setAttribute(Qt::WA_OpaquePaintEvent);
  setAutoFillBackground(false);
  setMouseTracking(true);

  setAttribute(Qt::WA_PaintOnScreen, true);
  setAttribute(Qt::WA_NativeWindow, true);
  setAttribute(Qt::WA_NoBackground);
  setAttribute(Qt::WA_NoSystemBackground);

  installEventFilter(this);

  m_uiViewID = s_uiNextViewID;
  ++s_uiNextViewID;
  m_pDocumentWindow->m_ViewWidgets.PushBack(this);
}


ezEngineViewWidget::~ezEngineViewWidget()
{
  ezViewDestroyedMsgToEngine msg;
  msg.m_uiViewID = GetViewID();
  m_pDocumentWindow->SendMessageToEngine(&msg);

  m_pDocumentWindow->m_ViewWidgets.RemoveSwap(this);
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
    ezEditorInputContext::GetActiveInputContext()->FocusLost();
    ezEditorInputContext::SetActiveInputContext(nullptr);
  }

  QWidget::focusOutEvent(e);
}