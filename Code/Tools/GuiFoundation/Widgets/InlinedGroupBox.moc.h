#pragma once

#include <GuiFoundation/Basics.h>
#include <GuiFoundation/Widgets/GroupBoxBase.moc.h>

class EZ_GUIFOUNDATION_DLL ezQtInlinedGroupBox : public ezQtGroupBoxBase
{
  Q_OBJECT
public:
  explicit ezQtInlinedGroupBox(QWidget* pParent);

  virtual void SetTitle(const char* szTitle) override;
  virtual void SetIcon(const QIcon& icon) override;
  virtual void SetFillColor(const QColor& color) override;

  virtual void SetCollapseState(bool bCollapsed) override;
  virtual bool GetCollapseState() const override;

  virtual QWidget* GetContent() override;
  virtual QWidget* GetHeader() override;

protected:
  virtual void paintEvent(QPaintEvent* event) override;

protected:
  QWidget* m_pContent;
  QWidget* m_pHeader;
};


