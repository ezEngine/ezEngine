#pragma once

#include <GuiFoundation/Basics.h>
#include <QWidget>
#include <QIcon>

class QMimeData;

class EZ_GUIFOUNDATION_DLL ezQtGroupBoxBase : public QWidget
{
  Q_OBJECT
public:
  ezQtGroupBoxBase(QWidget* pParent, bool bCollapsible);

  virtual void SetTitle(const char* szTitle);
  QString GetTitle() const;

  virtual void SetIcon(const QIcon& icon);
  QIcon GetIcon() const;

  virtual void SetFillColor(const QColor& color);
  QColor GetFillColor() const;

  virtual void SetDraggable(bool bDraggable);
  bool IsDraggable() const;

  virtual void SetCollapseState(bool bCollapsed) = 0;
  virtual bool GetCollapseState() const = 0;

  virtual QWidget* GetContent() = 0;
  virtual QWidget* GetHeader() = 0;

signals:
  void CollapseStateChanged(bool bCollapsed);
  void DragStarted(QMimeData& mimeData);

protected:
  enum Constants
  {
    Rounding = 4,
    Spacing = 1,
  };

  void DrawHeader(QPainter& p, const QRect& rect);
  void HeaderMousePress(QMouseEvent* me);
  void HeaderMouseMove(QMouseEvent* me);
  void HeaderMouseRelease(QMouseEvent* me);

  bool m_bDragging = false;
  QPoint m_startCursor;

  bool m_bCollapsible = false;
  bool m_bDraggable = false;
  QColor m_FillColor;
  QString m_sTitle;
  QIcon m_Icon;
};


