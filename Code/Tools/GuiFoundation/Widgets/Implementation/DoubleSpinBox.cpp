#include <GuiFoundation/PCH.h>
#include <GuiFoundation/Widgets/DoubleSpinBox.moc.h>
#include <Foundation/Math/Math.h>
#include <QLineEdit>
#include <QApplication>
#include <QDesktopWidget>
#include <qevent.h>
#include <qstyleoption.h>

inline ezQDoubleSpinBox::ezQDoubleSpinBox(QWidget* pParent, bool bIntMode) : QDoubleSpinBox(pParent)
{
  m_fDisplayedValue = ezMath::BasicType<float>::GetNaN();
  m_bInvalid = false;
  m_fDefaultValue = 0.0;
  m_bDragging = false;
  m_bModified = false;
  m_bIntMode = bIntMode;

  setSingleStep(0.1f);
  setContextMenuPolicy(Qt::CustomContextMenu);
  connect(this, &QWidget::customContextMenuRequested, this, &ezQDoubleSpinBox::onCustomContextMenuRequested);
}

void ezQDoubleSpinBox::setDisplaySuffix(const char* szSuffix)
{
  m_sSuffix = QString::fromUtf8(szSuffix);
}

void ezQDoubleSpinBox::setDefaultValue(double value)
{
  m_fDefaultValue = value;
}

QString ezQDoubleSpinBox::textFromValue(double val) const
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

  if (m_bIntMode)
    val = ezMath::Round(QDoubleSpinBox::value());

  QString sText = QDoubleSpinBox::textFromValue(val);

  while (sText.startsWith('0'))
    sText.remove(0, 1);

  if (sText.contains('.') || sText.contains(','))
  {
    while (sText.endsWith("0"))
      sText.chop(1);
    if (sText.endsWith(',') || sText.endsWith('.'))
      sText.chop(1);
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

double ezQDoubleSpinBox::valueFromText(const QString& text) const
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

void ezQDoubleSpinBox::setValueInvalid()
{
  m_bInvalid = true;
  m_sDisplayedText = QString();
  m_fDisplayedValue = ezMath::BasicType<float>::GetNaN();
  QDoubleSpinBox::setValue(minimum());
}

void ezQDoubleSpinBox::setValue(double val)
{
  EZ_ASSERT_DEBUG(ezMath::IsFinite(val), "Spin box value must be finite!");
  m_bInvalid = false;
  m_fDisplayedValue = ezMath::BasicType<float>::GetNaN();
  QDoubleSpinBox::setValue(val);
}

double ezQDoubleSpinBox::value() const
{
  if (m_bInvalid)
    return 0.0;

  EZ_ASSERT_DEBUG(!ezMath::IsNaN(QDoubleSpinBox::value()), "Spin box valid value should never be NaN!");
  return m_bIntMode ? ezMath::Round(QDoubleSpinBox::value()) : QDoubleSpinBox::value();
}

void ezQDoubleSpinBox::focusInEvent(QFocusEvent *event)
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

void ezQDoubleSpinBox::focusOutEvent(QFocusEvent *event)
{
  QDoubleSpinBox::focusOutEvent(event);

}

void ezQDoubleSpinBox::mousePressEvent(QMouseEvent* event)
{
  QStyleOptionSpinBox opt;
  initStyleOption(&opt);
  opt.subControls = QStyle::SC_All;
  QStyle::SubControl hoverControl = style()->hitTestComplexControl(QStyle::CC_SpinBox, &opt, event->pos(), this);

  if (event->button() == Qt::LeftButton && (hoverControl == QStyle::SC_SpinBoxUp || hoverControl == QStyle::SC_SpinBoxDown))
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

void ezQDoubleSpinBox::mouseReleaseEvent(QMouseEvent* event)
{
  if (event->button() == Qt::LeftButton && m_bDragging)
  {
    m_bDragging = false;
    if (m_bModified)
    {
      m_bModified = false;
      emit editingFinished();
    }
    else
    {
      QStyleOptionSpinBox opt;
      initStyleOption(&opt);
      opt.subControls = QStyle::SC_All;
      QStyle::SubControl hoverControl = style()->hitTestComplexControl(QStyle::CC_SpinBox, &opt, event->pos(), this);
      if (hoverControl == QStyle::SC_SpinBoxUp)
      {
        stepUp();
      }
      else if (hoverControl == QStyle::SC_SpinBoxDown)
      {
        stepDown();
      }
      // editingFinished sent on leave focus
    }
    releaseMouse();
    event->accept();
    return;
  }

  QDoubleSpinBox::mouseReleaseEvent(event);
}

void ezQDoubleSpinBox::mouseMoveEvent(QMouseEvent* event)
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
    double fValue = m_bInvalid ? m_fDefaultValue : QDoubleSpinBox::value();
    fValue += m_bIntMode ? (iDelta / 8.0) : (iDelta * 0.01);
    setValue(fValue);
    m_bModified = true;
  }
  QDoubleSpinBox::mouseMoveEvent(event);
}

void ezQDoubleSpinBox::onCustomContextMenuRequested()
{
  m_sDisplayedText = QDoubleSpinBox::textFromValue(m_fDefaultValue);
  setValue(m_fDefaultValue);
}
