#include <PCH.h>
#include <EditorFramework/DocumentWindow3D/EditorInputContext.h>
#include <QKeyEvent>
#include <QCursor>
#include <QApplication>
#include <QDesktopWidget>
#include <Foundation/Logging/Log.h>

ezEditorInputContext* ezEditorInputContext::s_pActiveInputContext = nullptr;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEditorInputContext, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE


ezEditorInputContext::ezEditorInputContext()
{
  m_pOwnerWindow = nullptr;
  m_pOwnerView = nullptr;
  m_bDisableShortcuts = false;
  m_bJustWrappedMouse = false;
}

ezEditorInputContext::~ezEditorInputContext()
{
  if (s_pActiveInputContext == this)
    SetActiveInputContext(nullptr);
}


void ezEditorInputContext::FocusLost(bool bCancel)
{
  // reset mouse mode, if necessary
  SetMouseMode(MouseMode::Normal);
}

ezEditorInut ezEditorInputContext::doKeyPressEvent(QKeyEvent* e)
{
  if (!IsActiveInputContext())
    return ezEditorInut::MayBeHandledByOthers;

  if (e->key() == Qt::Key_Escape)
  {
    FocusLost(true);
    SetActiveInputContext(nullptr);
    return ezEditorInut::WasExclusivelyHandled;
  }

  return ezEditorInut::MayBeHandledByOthers;
}


ezEditorInut ezEditorInputContext::mouseMoveEvent(QMouseEvent* e)
{
  if (m_MouseMode != MouseMode::Normal)
  {
    if (m_bJustWrappedMouse)
    {
      const ezVec2I32 curPos(e->globalX(), e->globalY());
      const ezVec2I32 diffToOld = curPos - m_MousePosBeforeWrap;
      const ezVec2I32 diffToNew = curPos - m_ExpectedMousePosition;

      if (diffToOld.GetLengthSquared() < diffToNew.GetLengthSquared())
      {
        // this is an invalid message, it was still in the message queue with old coordinates and should be discarded

        ezLog::Debug("Mouse move message was filtered out.");
        return ezEditorInut::WasExclusivelyHandled;
      }

      m_bJustWrappedMouse = false;
    }
  }

  return doMouseMoveEvent(e);
}

void ezEditorInputContext::MakeActiveInputContext(bool bActive /*= true*/)
{
  if (bActive)
    s_pActiveInputContext = this;
  else
    s_pActiveInputContext = nullptr;
}

void ezEditorInputContext::UpdateActiveInputContext()
{
  if (s_pActiveInputContext != nullptr)
    s_pActiveInputContext->UpdateContext();
}

bool ezEditorInputContext::IsActiveInputContext() const
{
  return s_pActiveInputContext == this;
}

void ezEditorInputContext::SetOwner(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView)
{
  m_pOwnerWindow = pOwnerWindow;
  m_pOwnerView = pOwnerView;

  OnSetOwner(m_pOwnerWindow, m_pOwnerView);
}

ezQtEngineDocumentWindow* ezEditorInputContext::GetOwnerWindow() const
{
  EZ_ASSERT_DEBUG(m_pOwnerWindow != nullptr, "Owner window pointer has not been set");
  return m_pOwnerWindow;
}

ezQtEngineViewWidget* ezEditorInputContext::GetOwnerView() const
{
  EZ_ASSERT_DEBUG(m_pOwnerView != nullptr, "Owner view pointer has not been set");
  return m_pOwnerView;
}

void ezEditorInputContext::SetMouseMode(MouseMode mode)
{
  if (m_MouseMode == mode)
    return;

  m_bJustWrappedMouse = false;
  const QPoint curPos = QCursor::pos();

  if (mode == MouseMode::HideAndWrapAtScreenBorders)
  {
    QCursor::setPos(QPoint(m_MouseRestorePosition.x, m_MouseRestorePosition.y));
  }
  
  if (mode != MouseMode::Normal)
  {
    const QRect dsize = QApplication::desktop()->screenGeometry(curPos);

    m_MouseWrapRect.x = dsize.x() + 10;
    m_MouseWrapRect.y = dsize.y() + 10;
    m_MouseWrapRect.width = dsize.width() - 20;
    m_MouseWrapRect.height = dsize.height() - 20;
  }

  if (m_MouseMode == MouseMode::HideAndWrapAtScreenBorders)
  {
    QApplication::restoreOverrideCursor();
  }

  if (mode == MouseMode::HideAndWrapAtScreenBorders)
  {
    QApplication::setOverrideCursor(Qt::BlankCursor);
  }

  m_MouseRestorePosition.Set(curPos.x(), curPos.y());
  m_MouseMode = mode;
}

ezVec2I32 ezEditorInputContext::UpdateMouseMode(QMouseEvent* e)
{
  const ezVec2I32 curPos(e->globalX(), e->globalY());

  if (m_MouseMode == MouseMode::Normal)
    return curPos;

  ezVec2I32 newPos = curPos;

  if (curPos.x > (ezInt32)m_MouseWrapRect.Right())
    newPos.x = m_MouseWrapRect.Left() + (curPos.x - m_MouseWrapRect.Right());

  if (curPos.x < (ezInt32)m_MouseWrapRect.Left())
    newPos.x = m_MouseWrapRect.Right() - (m_MouseWrapRect.Left() - curPos.x);

  if (curPos.y >(ezInt32)m_MouseWrapRect.Bottom())
    newPos.y = m_MouseWrapRect.Top() + (curPos.y - m_MouseWrapRect.Bottom());

  if (curPos.y < (ezInt32)m_MouseWrapRect.Top())
    newPos.y = m_MouseWrapRect.Bottom() - (m_MouseWrapRect.Top() - curPos.y);

  if (curPos != newPos)
  {
    // wrap the mouse around the screen borders
    QCursor::setPos(QPoint(newPos.x, newPos.y));

    // store where we expect the next mouse position
    m_ExpectedMousePosition = newPos;
    m_MousePosBeforeWrap = curPos;

    // next mouse message must be inspected for outliers
    m_bJustWrappedMouse = true;
  }

  return newPos;
}
