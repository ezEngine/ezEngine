#pragma once

#include <GuiFoundation/Basics.h>
#include <QWidget>

class EZ_GUIFOUNDATION_DLL ezQtGroupBoxBase : public QWidget
{
  Q_OBJECT
public:
  explicit ezQtGroupBoxBase(QWidget* pParent) : QWidget(pParent) {}

  virtual void SetTitle(const char* szTitle) = 0;
  virtual QString GetTitle() const = 0;

  virtual void SetIcon(const QPixmap& icon) = 0;

  virtual void SetFillColor(const QColor& color) = 0;
  virtual QColor GetFillColor() const = 0;

  virtual void SetCollapseState(bool bCollapsed) = 0;
  virtual bool GetCollapseState() const = 0;

  virtual QWidget* GetContent() = 0;
  virtual QWidget* GetHeader() = 0;

signals:
  void CollapseStateChanged(bool bCollapsed);
};


