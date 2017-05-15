#pragma once

#include <GuiFoundation/Basics.h>
#include <QWidget>
#include <QIcon>

class EZ_GUIFOUNDATION_DLL ezQtGroupBoxBase : public QWidget
{
  Q_OBJECT
public:
  explicit ezQtGroupBoxBase(QWidget* pParent) : QWidget(pParent) {}

  virtual void SetTitle(const char* szTitle);
  QString GetTitle() const;

  virtual void SetIcon(const QIcon& icon);
  QIcon GetIcon() const;

  virtual void SetFillColor(const QColor& color);
  QColor GetFillColor() const;

  virtual void SetCollapseState(bool bCollapsed) = 0;
  virtual bool GetCollapseState() const = 0;

  virtual QWidget* GetContent() = 0;
  virtual QWidget* GetHeader() = 0;

signals:
  void CollapseStateChanged(bool bCollapsed);

protected:
  enum Constants
  {
    Rounding = 4,
    Spacing = 1,
  };

  void DrawHeader(QPainter& p, const QRect& rect, const QString& sTitle, const QIcon& icon, bool bCollapsible);

  QColor m_FillColor;
  QString m_sTitle;
  QIcon m_Icon;
};


