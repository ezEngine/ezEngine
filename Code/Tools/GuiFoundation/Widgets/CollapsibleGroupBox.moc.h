#pragma once


#include <GuiFoundation/Basics.h>
#include <Code/Tools/GuiFoundation/ui_CollapsibleGroupBox.h>
#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>
#include <QGroupBox>
#include <QLayout>

class EZ_GUIFOUNDATION_DLL ezCollapsibleGroupBox : public QWidget, public Ui_CollapsibleGroupBox
{
  Q_OBJECT
public:
  explicit ezCollapsibleGroupBox(QWidget* pParent);

  void setTitle(QString sTitle);
  QString title() const;

  void setInnerLayout(QLayout* pLayout) { Content->setLayout(pLayout); }
  void SetFillColor(const QColor& color);
  QColor GetFillColor() const { return m_FillColor; }

  void SetCollapseState(bool bCollapsed);
  bool GetCollapseState() const;

signals:
  void CollapseStateChanged(bool bCollapsed);

protected:
  virtual bool eventFilter(QObject* object, QEvent* event) override;
  virtual void paintEvent(QPaintEvent* event) override;

protected:
  QColor m_FillColor;
  bool m_bCollapsed;
};


