#include <GuiFoundation/PCH.h>
#include <GuiFoundation/Widgets/DoubleSpinBox.moc.h>
#include <Foundation/Math/Math.h>
#include <QLineEdit>
#include <QApplication>
#include <QDesktopWidget>

inline QDoubleSpinBoxLessAnnoying::QDoubleSpinBoxLessAnnoying(QWidget* pParent) : QDoubleSpinBox(pParent)
{
  m_fDisplayedValue = ezMath::BasicType<float>::GetNaN();
  m_bInvalid = false;
  m_fDefaultValue = 0.0;
  m_bDragging = false;
  m_bModified = false;

  setSingleStep(0.1f);
  setContextMenuPolicy(Qt::CustomContextMenu);
  connect(this, &QWidget::customContextMenuRequested, this, &QDoubleSpinBoxLessAnnoying::onCustomContextMenuRequested);
}

void QDoubleSpinBoxLessAnnoying::setDisplaySuffix(const char* szSuffix)
{
  m_sSuffix = QString::fromUtf8(szSuffix);
}

void QDoubleSpinBoxLessAnnoying::setDefaultValue(double value)
{
  m_fDefaultValue = value;
}

QString QDoubleSpinBoxLessAnnoying::textFromValue(double val) const
{
  if (m_bInvalid)
    return QString();

  if (hasFocus() && val == m_fDisplayedValue && !ezMath::IsNaN(m_fDisplayedValue))
  {
    return m_sDisplayedText;
  }

  if (val == 0.0)
  {
    m_fDisplayedValue = 0;
    m_sDisplayedText = "0";
    QString res = QLatin1String("0");
    res += m_sSuffix;
    return res;
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

void QDoubleSpinBoxLessAnnoying::mousePressEvent(QMouseEvent* event)
{
  if (event->button() == Qt::LeftButton)
  {
    m_bDragging = true;
    m_bModified = false;
    m_LastDragPos = event->globalPos();
    grabMouse();
    event->accept();
    return;
  }
  QDoubleSpinBox::mousePressEvent(event);
}

void QDoubleSpinBoxLessAnnoying::mouseReleaseEvent(QMouseEvent* event)
{
  if (event->button() == Qt::LeftButton)
  {
    m_bDragging = false;
    if (m_bModified)
    {
      m_bModified = false;
      emit editingFinished();
    }
    releaseMouse();
    event->accept();
    return;
  }

  QDoubleSpinBox::mouseReleaseEvent(event);
}

void QDoubleSpinBoxLessAnnoying::mouseMoveEvent(QMouseEvent* event)
{
  if (m_bDragging)
  {
    int iDelta = m_LastDragPos.y() - event->globalPos().y();
    {
      m_LastDragPos = event->globalPos();
      const QRect dsize = QApplication::desktop()->availableGeometry(m_LastDragPos);
      if (m_LastDragPos.y() < (dsize.top() + 10))
      {
        m_LastDragPos.setY(dsize.bottom() - 10);
        QCursor::setPos(m_LastDragPos);
      }
      else if (m_LastDragPos.y() > (dsize.bottom() - 10))
      {
        m_LastDragPos.setY(dsize.top() + 10);
        QCursor::setPos(m_LastDragPos);
      }
    }
    double fValue = m_bInvalid ? m_fDefaultValue : value();
    fValue += iDelta * 0.01;
    setValue(fValue);
    m_bModified = true;
  }
  QDoubleSpinBox::mouseMoveEvent(event);
}

void QDoubleSpinBoxLessAnnoying::onCustomContextMenuRequested()
{
  m_sDisplayedText = QDoubleSpinBox::textFromValue(m_fDefaultValue);
  setValue(m_fDefaultValue);
}

ezQIntSpinbox::ezQIntSpinbox(QWidget* pParent)
  : QSpinBox(pParent)
{
  m_iDefaultValue = 0;

  setContextMenuPolicy(Qt::CustomContextMenu);
  connect(this, &QWidget::customContextMenuRequested, this, &ezQIntSpinbox::onCustomContextMenuRequested);
}

void ezQIntSpinbox::onCustomContextMenuRequested()
{
  setValue(m_iDefaultValue);
}
