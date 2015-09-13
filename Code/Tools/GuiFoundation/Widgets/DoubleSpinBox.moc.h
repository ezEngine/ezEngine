#pragma once

#include <GuiFoundation/Basics.h>
#include <QDoubleSpinBox>

class EZ_GUIFOUNDATION_DLL QDoubleSpinBoxLessAnnoying : public QDoubleSpinBox
{
  Q_OBJECT
public:
  explicit QDoubleSpinBoxLessAnnoying(QWidget* pParent);

  virtual QString textFromValue(double val) const override;
  virtual double valueFromText(const QString &text) const override;
  virtual void fixup(QString &str) const override;

  void setValueInvalid();
  double value() const;

private:
  mutable bool m_bInvalid;
};