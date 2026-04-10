#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>
#include <QWidget>

class ezManipulatorAttribute;
class QLabel;
class QToolButton;

class ezQtManipulatorLabel : public QWidget
{
  Q_OBJECT
public:
  explicit ezQtManipulatorLabel(QWidget* pParent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());

  void setText(const QString& sText);
  void setAlignment(Qt::Alignment alignment);

  const ezManipulatorAttribute* GetManipulator() const;
  void SetManipulator(const ezManipulatorAttribute* pManipulator);

  bool GetManipulatorActive() const;
  void SetManipulatorActive(bool bActive);

  void SetSelection(const ezArrayPtr<ezPropertySelection>& items);

  void ToggleManipulator();

  void SetIsDefault(bool bIsDefault);

protected:
  virtual bool eventFilter(QObject* pWatched, QEvent* pEvent) override;
  virtual void showEvent(QShowEvent* event) override;

private:
  QLabel* m_pLabel = nullptr;
  QToolButton* m_pButton = nullptr;
  ezArrayPtr<ezPropertySelection> m_Items;
  const ezManipulatorAttribute* m_pManipulator = nullptr;
  QFont m_Font;
  bool m_bActive = false;
  bool m_bIsDefault = true;
};
