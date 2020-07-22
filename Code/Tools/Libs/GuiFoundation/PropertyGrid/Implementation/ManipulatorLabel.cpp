#include <GuiFoundationPCH.h>

#include <GuiFoundation/PropertyGrid/Implementation/ManipulatorLabel.moc.h>
#include <GuiFoundation/PropertyGrid/ManipulatorManager.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QFont>
#include <qevent.h>

ezQtManipulatorLabel::ezQtManipulatorLabel(QWidget* parent, Qt::WindowFlags f)
  : QLabel(parent, f)
  , m_pItems(nullptr)
  , m_pManipulator(nullptr)
  , m_bActive(false)
{
  setCursor(Qt::WhatsThisCursor);
}

ezQtManipulatorLabel::ezQtManipulatorLabel(const QString& text, QWidget* parent, Qt::WindowFlags f)
  : QLabel(text, parent, f)
  , m_pItems(nullptr)
  , m_pManipulator(nullptr)
  , m_bActive(false)
  , m_bIsDefault(true)
{
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
    setCursor(Qt::PointingHandCursor);
    setForegroundRole(QPalette::ColorRole::Link);
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
    setForegroundRole(m_bActive ? QPalette::ColorRole::LinkVisited : QPalette::ColorRole::Link);
  }
}

void ezQtManipulatorLabel::SetSelection(const ezHybridArray<ezPropertySelection, 8>& items)
{
  m_pItems = &items;
}


void ezQtManipulatorLabel::SetIsDefault(bool bIsDefault)
{
  if (m_bIsDefault != bIsDefault)
  {
    m_bIsDefault = bIsDefault;
    m_font.setBold(!m_bIsDefault);
    setFont(m_font);
  }
}


void ezQtManipulatorLabel::contextMenuEvent(QContextMenuEvent* ev)
{
  Q_EMIT customContextMenuRequested(ev->globalPos());
}

void ezQtManipulatorLabel::showEvent(QShowEvent* event)
{
  // Use of style sheets (ADS) breaks previously set font.
  setFont(m_font);
  QLabel::showEvent(event);
}

void ezQtManipulatorLabel::mousePressEvent(QMouseEvent* ev)
{
  if (ev->button() != Qt::LeftButton)
    return;

  if (m_pManipulator == nullptr)
    return;

  const ezDocument* pDoc = (*m_pItems)[0].m_pObject->GetDocumentObjectManager()->GetDocument();

  if (m_bActive)
    ezManipulatorManager::GetSingleton()->ClearActiveManipulator(pDoc);
  else
    ezManipulatorManager::GetSingleton()->SetActiveManipulator(pDoc, m_pManipulator, *m_pItems);
}

void ezQtManipulatorLabel::enterEvent(QEvent* ev)
{
  if (m_pManipulator)
  {
    m_font.setUnderline(true);
    setFont(m_font);
  }

  QLabel::enterEvent(ev);
}

void ezQtManipulatorLabel::leaveEvent(QEvent* ev)
{
  if (m_pManipulator)
  {
    m_font.setUnderline(false);
    setFont(m_font);
  }

  QLabel::leaveEvent(ev);
}
