#include <GuiFoundation/PCH.h>
#include <GuiFoundation/Widgets/DoubleSpinBox.moc.h>
#include <Foundation/Math/Math.h>

inline QDoubleSpinBoxLessAnnoying::QDoubleSpinBoxLessAnnoying(QWidget * pParent) : QDoubleSpinBox(pParent)
{
  m_bInvalid = false;
}

QString QDoubleSpinBoxLessAnnoying::textFromValue(double val) const
{
  if (hasFocus() && val == m_fDisplayedValue)
  {
    return m_sDisplayedText;
  }

  if (val == 0.0)
  {
    m_fDisplayedValue = 0;
    m_sDisplayedText = "0";
    return QLatin1String("0");
  }

  QString sText = QDoubleSpinBox::textFromValue(val);

  while (sText.startsWith('0'))
    sText.remove(0, 1);

  if (sText.contains('.') || sText.contains(','))
  {
    while (sText.endsWith("0"))
      sText.resize(sText.length() - 1);
    if (sText.endsWith(',') || sText.endsWith('.'))
      sText.resize(sText.length() - 1);
    if (sText.startsWith(',') || sText.startsWith('.'))
      sText.insert(0, '0');
  }

  m_fDisplayedValue = val;
  m_sDisplayedText = sText;
  return sText;
}

double QDoubleSpinBoxLessAnnoying::valueFromText(const QString& text) const
{
  if (m_bInvalid && text != specialValueText())
  {
    const_cast<QDoubleSpinBoxLessAnnoying*>(this)->setSpecialValueText(QString());
    m_bInvalid = false;
  }

  QString sFixedText = text;

  if (sFixedText.contains(','))
    sFixedText.replace(',', ".");

  const double val = QDoubleSpinBox::valueFromText(sFixedText);

  if (hasFocus())
  {
    m_sDisplayedText = text;
    m_fDisplayedValue = val;
  }

  return val;
}

void QDoubleSpinBoxLessAnnoying::setValueInvalid()
{
  m_bInvalid = true;
  m_sDisplayedText = " ";
  m_fDisplayedValue = ezMath::BasicType<float>::GetNaN();
  setSpecialValueText(QStringLiteral(" "));
  setValue(minimum());
}

double QDoubleSpinBoxLessAnnoying::value() const
{
  if (m_bInvalid)
    return 0.0;

  return QDoubleSpinBox::value();
}


