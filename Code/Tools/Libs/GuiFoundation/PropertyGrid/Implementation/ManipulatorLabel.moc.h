#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>
#include <QLabel>

class ezManipulatorAttribute;

class ezQtManipulatorLabel : public QLabel
{
  Q_OBJECT
public:
  explicit ezQtManipulatorLabel(QWidget* pParent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
  explicit ezQtManipulatorLabel(const QString& sText, QWidget* pParent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());

  const ezManipulatorAttribute* GetManipulator() const;
  void SetManipulator(const ezManipulatorAttribute* pManipulator);

  bool GetManipulatorActive() const;
  void SetManipulatorActive(bool bActive);

  void SetSelection(const ezHybridArray<ezPropertySelection, 8>& items);

  void SetIsDefault(bool bIsDefault);

protected:
  virtual void contextMenuEvent(QContextMenuEvent* ev) override;
  virtual void showEvent(QShowEvent* event) override;

private:
  virtual void mousePressEvent(QMouseEvent* ev) override;

#if QT_VERSION > QT_VERSION_CHECK(6, 0, 0)
  virtual void enterEvent(QEnterEvent* ev) override;
#else
  virtual void enterEvent(QEvent* ev) override;
#endif

  virtual void leaveEvent(QEvent* ev) override;

private:
  const ezHybridArray<ezPropertySelection, 8>* m_pItems;
  const ezManipulatorAttribute* m_pManipulator;
  QFont m_Font;
  bool m_bActive;
  bool m_bIsDefault;
};
