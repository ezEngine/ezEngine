#pragma once


#include <GuiFoundation/GuiFoundationDLL.h>
#include <Code/Tools/GuiFoundation/ui_CollapsibleGroupBox.h>
#include <GuiFoundation/Widgets/GroupBoxBase.moc.h>

class EZ_GUIFOUNDATION_DLL ezQtCollapsibleGroupBox : public ezQtGroupBoxBase, protected Ui_CollapsibleGroupBox
{
  Q_OBJECT
public:
  explicit ezQtCollapsibleGroupBox(QWidget* pParent);

  virtual void SetTitle(const char* szTitle) override;
  virtual void SetIcon(const QIcon& icon) override;
  virtual void SetFillColor(const QColor& color) override;

  virtual void SetCollapseState(bool bCollapsed) override;
  virtual bool GetCollapseState() const override;

  virtual QWidget* GetContent() override;
  virtual QWidget* GetHeader() override;

protected:
  virtual bool eventFilter(QObject* object, QEvent* event) override;
  virtual void paintEvent(QPaintEvent* event) override;

protected:
  bool m_bCollapsed;
};


