#pragma once

#include <GuiFoundation/Basics.h>
#include <QDoubleSpinBox>

class EZ_GUIFOUNDATION_DLL QDoubleSpinBoxLessAnnoying : public QDoubleSpinBox
{
  Q_OBJECT
public:
  explicit QDoubleSpinBoxLessAnnoying(QWidget* pParent);

  void setDisplaySuffix(const char* szSuffix);

  virtual QString textFromValue(double val) const override;
  virtual double valueFromText(const QString &text) const override;

  void setValueInvalid();
  void setValue(double val);
  double value() const;

protected:
  virtual void focusInEvent(QFocusEvent *event) override;
  virtual void focusOutEvent(QFocusEvent *event) override;

private:
  QString m_sSuffix;
  mutable double m_fDisplayedValue;
  mutable QString m_sDisplayedText;
  mutable bool m_bInvalid;
};