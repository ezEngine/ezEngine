#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/PropertyGrid/Implementation/ManipulatorLabel.moc.h>
#include <GuiFoundation/PropertyGrid/ManipulatorManager.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>
#include <qevent.h>

ezQtManipulatorLabel::ezQtManipulatorLabel(QWidget* pParent, Qt::WindowFlags f)
  : QWidget(pParent, f)
{
  QHBoxLayout* pLayout = new QHBoxLayout(this);
  pLayout->setContentsMargins(0, 0, 0, 0);
  pLayout->setSpacing(2);

  m_pLabel = new QLabel(this);
  m_pLabel->setCursor(Qt::WhatsThisCursor);
  m_pLabel->installEventFilter(this);
  pLayout->addWidget(m_pLabel, 1);

  QIcon manipIcon;
  manipIcon.addFile(":/GuiFoundation/Icons/Manipulator-off.svg", QSize(), QIcon::Normal, QIcon::Off);
  manipIcon.addFile(":/GuiFoundation/Icons/Manipulator-on.svg", QSize(), QIcon::Normal, QIcon::On);

  m_pButton = new QToolButton(this);
  m_pButton->setCheckable(true);
  m_pButton->setAutoRaise(true);
  m_pButton->setIcon(manipIcon);
  m_pButton->setFixedSize(24, 24);
  m_pButton->setToolTip("Toggles the manipulator gizmo for this property.");
  m_pButton->setVisible(false);
  connect(m_pButton, &QAbstractButton::clicked, this, &ezQtManipulatorLabel::ToggleManipulator);
  pLayout->addWidget(m_pButton, 0);
}

void ezQtManipulatorLabel::setText(const QString& sText)
{
  m_pLabel->setText(sText);
}

void ezQtManipulatorLabel::setAlignment(Qt::Alignment alignment)
{
  m_pLabel->setAlignment(alignment);
}

const ezManipulatorAttribute* ezQtManipulatorLabel::GetManipulator() const
{
  return m_pManipulator;
}

void ezQtManipulatorLabel::SetManipulator(const ezManipulatorAttribute* pManipulator)
{
  m_pManipulator = pManipulator;

  if (m_pManipulator)
  {
    m_pLabel->setCursor(Qt::PointingHandCursor);
    m_pLabel->setForegroundRole(QPalette::ColorRole::Link);
    m_pButton->setVisible(true);
  }
}

bool ezQtManipulatorLabel::GetManipulatorActive() const
{
  return m_bActive;
}

void ezQtManipulatorLabel::SetManipulatorActive(bool bActive)
{
  m_bActive = bActive;

  if (m_pManipulator)
  {
    m_pLabel->setForegroundRole(m_bActive ? QPalette::ColorRole::LinkVisited : QPalette::ColorRole::Link);
    m_pButton->setChecked(m_bActive);
  }
}

void ezQtManipulatorLabel::SetSelection(const ezArrayPtr<ezPropertySelection>& items)
{
  m_Items = items;
}

void ezQtManipulatorLabel::SetIsDefault(bool bIsDefault)
{
  if (m_bIsDefault != bIsDefault)
  {
    m_bIsDefault = bIsDefault;
    m_Font.setBold(!m_bIsDefault);
    m_pLabel->setFont(m_Font);
  }
}

void ezQtManipulatorLabel::ToggleManipulator()
{
  if (m_pManipulator == nullptr)
    return;

  const ezDocument* pDoc = m_Items[0].m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();

  if (m_bActive)
    ezManipulatorManager::GetSingleton()->ClearActiveManipulator(pDoc);
  else
    ezManipulatorManager::GetSingleton()->SetActiveManipulator(pDoc, m_pManipulator, m_Items);
}

void ezQtManipulatorLabel::showEvent(QShowEvent* event)
{
  // Use of style sheets (ADS) breaks previously set font.
  m_pLabel->setFont(m_Font);
  QWidget::showEvent(event);
}

bool ezQtManipulatorLabel::eventFilter(QObject* pWatched, QEvent* pEvent)
{
  if (pWatched == m_pLabel)
  {
    switch (pEvent->type())
    {
      case QEvent::Enter:
        if (m_pManipulator)
        {
          m_Font.setUnderline(true);
          m_pLabel->setFont(m_Font);
        }
        break;

      case QEvent::Leave:
        if (m_pManipulator)
        {
          m_Font.setUnderline(false);
          m_pLabel->setFont(m_Font);
        }
        break;

      case QEvent::MouseButtonPress:
        if (static_cast<QMouseEvent*>(pEvent)->button() == Qt::LeftButton)
          ToggleManipulator();
        break;

      case QEvent::ContextMenu:
        Q_EMIT customContextMenuRequested(static_cast<QContextMenuEvent*>(pEvent)->globalPos());
        return true;

      default:
        break;
    }
  }

  return QWidget::eventFilter(pWatched, pEvent);
}
