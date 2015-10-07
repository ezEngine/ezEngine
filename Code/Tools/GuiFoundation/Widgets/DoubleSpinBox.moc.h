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

  void setValueInvalid();
  double value() const;

private:
  mutable QString m_sDisplayedText;
  mutable bool m_bInvalid;
};