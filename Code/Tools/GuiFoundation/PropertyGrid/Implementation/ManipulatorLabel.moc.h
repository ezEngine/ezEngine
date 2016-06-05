#pragma once

#include <GuiFoundation/Basics.h>
#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>
#include <QLabel>

class ezManipulatorAttribute;

class ezManipulatorLabel : public QLabel
{
  Q_OBJECT
public:
  explicit ezManipulatorLabel(QWidget* parent = nullptr, Qt::WindowFlags f = 0);
  explicit ezManipulatorLabel(const QString& text, QWidget* parent = nullptr, Qt::WindowFlags f = 0);

  const ezManipulatorAttribute* GetManipulator() const;
  void SetManipulator(const ezManipulatorAttribute* pManipulator);

  const bool GetManipulatorActive() const;
  void SetManipulatorActive(bool bActive);

  void SetSelection(const ezHybridArray<ezQtPropertyWidget::Selection, 8>& items);

  void SetIsDefault(bool bIsDefault);

private:
  virtual void mousePressEvent(QMouseEvent *ev) override;
  virtual void enterEvent(QEvent* ev) override;
  virtual void leaveEvent(QEvent* ev) override;

private:
  const ezHybridArray<ezQtPropertyWidget::Selection, 8>* m_pItems;
  const ezManipulatorAttribute* m_pManipulator;
  bool m_bActive;
  bool m_bIsDefault;
};
