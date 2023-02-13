#pragma once

#include <Foundation/Logging/Log.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <QAbstractItemView>
#include <QHoverEvent>
#include <QItemDelegate>
#include <QStyledItemDelegate>

/// \brief In combination with ezQtItemView this delegate allows for receiving the full range of mouse input.
class EZ_GUIFOUNDATION_DLL ezQtItemDelegate : public QItemDelegate
{
  Q_OBJECT
public:
  explicit ezQtItemDelegate(QObject* parent = nullptr)
    : QItemDelegate(parent)
  {
  }

  virtual bool mouseHoverEvent(QHoverEvent* event, const QStyleOptionViewItem& option, const QModelIndex& index) { return false; }
  virtual bool mousePressEvent(QMouseEvent* event, const QStyleOptionViewItem& option, const QModelIndex& index) { return false; }
  virtual bool mouseReleaseEvent(QMouseEvent* event, const QStyleOptionViewItem& option, const QModelIndex& index) { return false; }
  virtual bool mouseDoubleClickEvent(QMouseEvent* event, const QStyleOptionViewItem& option, const QModelIndex& index) { return false; }
  virtual bool mouseMoveEvent(QMouseEvent* event, const QStyleOptionViewItem& option, const QModelIndex& index) { return false; }
};

/// \brief Template to be used with classes derived from QAbstractItemView. Allows the use of ezQtItemDelegate.
template <typename Base>
class ezQtItemView : public Base
{
public:
  ezQtItemView(QWidget* pParent)
    : Base(pParent)
    , m_pFocusedDelegate(nullptr)
  {
    this->setAttribute(Qt::WA_Hover, true);
  }

  virtual bool event(QEvent* ev) override
  {
    switch (ev->type())
    {
      case QEvent::HoverEnter:
      case QEvent::HoverMove:
      case QEvent::HoverLeave:
      {
        QHoverEvent* pHoeverEvent = static_cast<QHoverEvent*>(ev);


#if QT_VERSION <= QT_VERSION_CHECK(5, 14, 0)
        QPoint pos = pHoeverEvent->pos();
#else
        QPoint pos = pHoeverEvent->position().toPoint();
#endif
        QModelIndex index = this->indexAt(pos);
        if (m_Hovered.isValid() && (ev->type() == QEvent::HoverLeave || index != m_Hovered))
        {
          QHoverEvent hoverEvent(QEvent::HoverLeave, pos, pHoeverEvent->oldPos(), pHoeverEvent->modifiers());
          ForwardEvent(m_Hovered, &hoverEvent);
          m_Hovered = QModelIndex();
        }
        if (index.isValid() && ev->type() != QEvent::HoverLeave && !m_Hovered.isValid())
        {
          QHoverEvent hoverEvent(QEvent::HoverEnter, pos, pHoeverEvent->oldPos(), pHoeverEvent->modifiers());
          m_Hovered = index;
          ForwardEvent(m_Hovered, &hoverEvent);
        }
        else if (m_Hovered.isValid())
        {
          QHoverEvent hoverEvent(QEvent::HoverMove, pos, pHoeverEvent->oldPos(), pHoeverEvent->modifiers());
          ForwardEvent(m_Hovered, &hoverEvent);
        }
        break;
      }
      default:
        break;
    }

    return Base::event(ev);
  }

  virtual void mousePressEvent(QMouseEvent* event) override
  {
    QPoint pos = event->pos();
    QModelIndex index = this->indexAt(pos);
    if (ForwardEvent(index, event))
    {
      if (!m_Focused.isValid())
      {
        m_Focused = index;
        this->viewport()->grabMouse();
      }
    }
    else
    {
      Base::mousePressEvent(event);
    }
  }

  virtual void mouseReleaseEvent(QMouseEvent* event) override
  {
    if (m_Focused.isValid())
    {
      ForwardEvent(m_Focused, event);
      if (event->buttons() == Qt::NoButton)
      {
        m_Focused = QModelIndex();
        this->viewport()->releaseMouse();
      }
    }
    else
    {
      Base::mouseReleaseEvent(event);
    }
  }

  virtual void mouseDoubleClickEvent(QMouseEvent* event) override
  {
    QPoint pos = event->pos();
    QModelIndex index = this->indexAt(pos);
    if (!ForwardEvent(index, event))
    {
      Base::mouseDoubleClickEvent(event);
    }
  }

  virtual void mouseMoveEvent(QMouseEvent* event) override
  {
    QPoint pos = event->pos();
    QModelIndex index = this->indexAt(pos);
    if (!ForwardEvent(index, event))
    {
      Base::mouseMoveEvent(event);
    }
  }

private:
  bool ForwardEvent(QModelIndex index, QEvent* pEvent)
  {
    if (m_Focused.isValid())
      index = m_Focused;
    if (!index.isValid())
      return false;

    if (ezQtItemDelegate* pDelegate = qobject_cast<ezQtItemDelegate*>(this->itemDelegate(m_Hovered)))
    {
      QStyleOptionViewItem option;
      this->initViewItemOption(&option);
      option.rect = this->visualRect(index);
      option.state |= (index == this->currentIndex() ? QStyle::State_HasFocus : QStyle::State_None);

      bool bRes = false;
      switch (pEvent->type())
      {
        case QEvent::HoverEnter:
        case QEvent::HoverMove:
        case QEvent::HoverLeave:
          bRes = pDelegate->mouseHoverEvent(static_cast<QHoverEvent*>(pEvent), option, index);
          break;
        case QEvent::MouseButtonPress:
          bRes = pDelegate->mousePressEvent(static_cast<QMouseEvent*>(pEvent), option, index);
          break;
        case QEvent::MouseButtonRelease:
          bRes = pDelegate->mouseReleaseEvent(static_cast<QMouseEvent*>(pEvent), option, index);
          break;
        case QEvent::MouseButtonDblClick:
          bRes = pDelegate->mouseDoubleClickEvent(static_cast<QMouseEvent*>(pEvent), option, index);
          break;
        case QEvent::MouseMove:
          bRes = pDelegate->mouseMoveEvent(static_cast<QMouseEvent*>(pEvent), option, index);
          break;
        default:
          break;
      }
      this->update(index);
      return bRes;
    }

    return false;
  }

private:
  ezQtItemDelegate* m_pFocusedDelegate;
  QPersistentModelIndex m_Hovered;
  QPersistentModelIndex m_Focused;
};

