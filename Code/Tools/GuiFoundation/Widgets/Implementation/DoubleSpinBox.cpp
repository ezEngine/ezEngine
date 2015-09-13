#include <GuiFoundation/PCH.h>
#include <GuiFoundation/Widgets/DoubleSpinBox.moc.h>

inline QDoubleSpinBoxLessAnnoying::QDoubleSpinBoxLessAnnoying(QWidget * pParent) : QDoubleSpinBox(pParent)
{
  m_bInvalid = false;
}

inline QString QDoubleSpinBoxLessAnnoying::textFromValue(double val) const
{
  if (val == 0.0)
    return QLatin1String("0");

  QString sText = QDoubleSpinBox::textFromValue(val);

  while (sText.startsWith('0'))
    sText.remove(0, 1);

  if (sText.contains('.') || sText.contains(','))
  {
    while (sText.endsWith('0'))
      sText.resize(sText.length() - 1);
    if (sText.endsWith(',') || sText.endsWith('.'))
      sText.resize(sText.length() - 1);
    if (sText.startsWith(',') || sText.startsWith('.'))
      sText.insert(0, '0');
  }
  return sText;
}

double QDoubleSpinBoxLessAnnoying::valueFromText(const QString& text) const
{
  if (text != specialValueText())
  {
    const_cast<QDoubleSpinBoxLessAnnoying*>(this)->setSpecialValueText(QString());
    m_bInvalid = false;
  }
  return QDoubleSpinBox::valueFromText(text);
}

void QDoubleSpinBoxLessAnnoying::fixup(QString& str) const
{
  QDoubleSpinBox::fixup(str);
}

void QDoubleSpinBoxLessAnnoying::setValueInvalid()
{
  m_bInvalid = true;
  setSpecialValueText(QStringLiteral(" "));
  setValue(minimum());
}

double QDoubleSpinBoxLessAnnoying::value() const
{
  if (m_bInvalid)
    return 0.0;

  return QDoubleSpinBox::value();
}
