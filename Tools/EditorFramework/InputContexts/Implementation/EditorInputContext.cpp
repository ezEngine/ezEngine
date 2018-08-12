#include <PCH.h>

#include "DocumentWindow/EngineViewWidget.moc.h"
#include <EditorFramework/InputContexts/EditorInputContext.h>
#include <Foundation/Logging/Log.h>
#include <QApplication>
#include <QCursor>
#include <QDesktopWidget>
#include <QKeyEvent>

ezEditorInputContext* ezEditorInputContext::s_pActiveInputContext = nullptr;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEditorInputContext, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE;


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
  DoFocusLost(bCancel);

  // reset mouse mode, if necessary
  SetMouseMode(MouseMode::Normal);

  UpdateStatusBarText(GetOwnerWindow());
}

ezEditorInput ezEditorInputContext::DoKeyPressEvent(QKeyEvent* e)
{
  if (!IsActiveInputContext())
    return ezEditorInput::MayBeHandledByOthers;

  if (e->key() == Qt::Key_Escape)
  {
    FocusLost(true);
    SetActiveInputContext(nullptr);
    return ezEditorInput::WasExclusivelyHandled;
  }

  return ezEditorInput::MayBeHandledByOthers;
}


ezEditorInput ezEditorInputContext::MouseMoveEvent(QMouseEvent* e)
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

        return ezEditorInput::WasExclusivelyHandled;
      }

      m_bJustWrappedMouse = false;
    }
  }

  return DoMouseMoveEvent(e);
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
  ezQtEngineViewWidget* pView = m_pOwnerView;

  if (pView == nullptr)
  {
    pView = ezQtEngineViewWidget::GetInteractionContext().m_pLastHoveredViewWidget;
  }

  EZ_ASSERT_DEBUG(pView != nullptr, "Owner view pointer has not been set");
  return pView;
}

ezVec2I32 ezEditorInputContext::SetMouseMode(MouseMode newMode)
{
  const QPoint curPos = QCursor::pos();

  if (m_MouseMode == newMode)
    return ezVec2I32(curPos.x(), curPos.y());

  m_bJustWrappedMouse = false;

  if (newMode != MouseMode::Normal)
  {
    const QRect dsize = QApplication::desktop()->availableGeometry(curPos);

    m_MouseWrapRect.x = dsize.x() + 10;
    m_MouseWrapRect.y = dsize.y() + 10;
    m_MouseWrapRect.width = dsize.width() - 20;
    m_MouseWrapRect.height = dsize.height() - 20;
  }

  if (m_MouseMode == MouseMode::HideAndWrapAtScreenBorders)
  {
    QCursor::setPos(QPoint(m_MouseRestorePosition.x, m_MouseRestorePosition.y));
    QApplication::restoreOverrideCursor();
  }

  if (newMode == MouseMode::HideAndWrapAtScreenBorders)
  {
    m_MouseRestorePosition.Set(curPos.x(), curPos.y());
    QApplication::setOverrideCursor(Qt::BlankCursor);
  }

  m_MouseMode = newMode;

  return ezVec2I32(curPos.x(), curPos.y());
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

  if (curPos.y > (ezInt32)m_MouseWrapRect.Bottom())
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
