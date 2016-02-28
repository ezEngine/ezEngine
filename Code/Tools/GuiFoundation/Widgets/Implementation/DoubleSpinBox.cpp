#include <GuiFoundation/PCH.h>
#include <GuiFoundation/Widgets/DoubleSpinBox.moc.h>
#include <Foundation/Math/Math.h>
#include <QLineEdit>

inline QDoubleSpinBoxLessAnnoying::QDoubleSpinBoxLessAnnoying(QWidget* pParent) : QDoubleSpinBox(pParent)
{
  m_fDisplayedValue = ezMath::BasicType<float>::GetNaN();
  m_bInvalid = false;
}

void QDoubleSpinBoxLessAnnoying::setDisplaySuffix(const char* szSuffix)
{
  m_sSuffix = QString::fromUtf8(szSuffix);
}

QString QDoubleSpinBoxLessAnnoying::textFromValue(double val) const
{
  if (m_bInvalid)
    return QString();

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

  if (!hasFocus())
  {
    sText += m_sSuffix;
  }

  return sText;
}

double QDoubleSpinBoxLessAnnoying::valueFromText(const QString& text) const
{
  if (m_bInvalid)
  {
    m_bInvalid = false;
  }

  QString sFixedText = text;

  if (!m_sSuffix.isEmpty() && sFixedText.endsWith(m_sSuffix))
  {
    sFixedText.chop(m_sSuffix.length());
  }

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
  m_sDisplayedText = QString();
  m_fDisplayedValue = ezMath::BasicType<float>::GetNaN();
  QDoubleSpinBox::setValue(minimum());
}

void QDoubleSpinBoxLessAnnoying::setValue(double val)
{
  EZ_ASSERT_DEBUG(ezMath::IsFinite(val), "Spin box value must be finite!");
  m_bInvalid = false;
  m_fDisplayedValue = ezMath::BasicType<float>::GetNaN();
  QDoubleSpinBox::setValue(val);
}

double QDoubleSpinBoxLessAnnoying::value() const
{
  if (m_bInvalid)
    return 0.0;

  EZ_ASSERT_DEBUG(!ezMath::IsNaN(QDoubleSpinBox::value()), "Spin box valid value should never be NaN!");
  return QDoubleSpinBox::value();
}

void QDoubleSpinBoxLessAnnoying::focusInEvent(QFocusEvent *event)
{
  if (!m_sSuffix.isEmpty())
  {
    QString s = lineEdit()->text();

    if (!m_sSuffix.isEmpty() && s.endsWith(m_sSuffix))
    {
      s.chop(m_sSuffix.length());
    }

    lineEdit()->setText(s);
  }

  QDoubleSpinBox::focusInEvent(event);
}

void QDoubleSpinBoxLessAnnoying::focusOutEvent(QFocusEvent *event)
{
  QDoubleSpinBox::focusOutEvent(event);

}

