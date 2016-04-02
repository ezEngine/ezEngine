#include <GuiFoundation/PCH.h>
#include <GuiFoundation/PropertyGrid/Implementation/ManipulatorLabel.moc.h>
#include <qevent.h>
#include <GuiFoundation/PropertyGrid/ManipulatorManager.h>

ezManipulatorLabel::ezManipulatorLabel(QWidget* parent, Qt::WindowFlags f)
  : QLabel(parent, f), m_pItems(nullptr), m_pManipulator(nullptr), m_bActive(false)
{

}

ezManipulatorLabel::ezManipulatorLabel(const QString& text, QWidget* parent, Qt::WindowFlags f)
  : QLabel(text, parent, f), m_pItems(nullptr), m_pManipulator(nullptr), m_bActive(false)
{

}

const ezManipulatorAttribute* ezManipulatorLabel::GetManipulator() const
{
  return m_pManipulator;
}

void ezManipulatorLabel::SetManipulator(const ezManipulatorAttribute* pManipulator)
{
  m_pManipulator = pManipulator;
}

const bool ezManipulatorLabel::GetManipulatorActive() const
{
  return m_bActive;
}

void ezManipulatorLabel::SetManipulatorActive(bool bActive)
{
  m_bActive = bActive;

  if (m_pManipulator)
  {
    QFont f = font();
    f.setBold(m_bActive);
    setFont(f);
  }
}

void ezManipulatorLabel::SetSelection(const ezHybridArray<ezQtPropertyWidget::Selection, 8>& items)
{
  m_pItems = &items;
}

void ezManipulatorLabel::mousePressEvent(QMouseEvent *ev)
{
  if (ev->button() != Qt::LeftButton)
    return;

  if (m_pManipulator == nullptr)
    return;

  ezManipulatorManager::GetSingleton()->SetActiveManipulator((*m_pItems)[0].m_pObject->GetDocumentObjectManager()->GetDocument(), m_pManipulator, *m_pItems);
}

void ezManipulatorLabel::enterEvent(QEvent* ev)
{
  if (m_pManipulator)
  {
    QFont f = font();
    f.setUnderline(true);
    f.setBold(m_bActive);
    setFont(f);
  }

  QLabel::enterEvent(ev);
}

void ezManipulatorLabel::leaveEvent(QEvent* ev)
{
  if (m_pManipulator)
  {
    QFont f = font();
    f.setUnderline(false);
    f.setBold(m_bActive);
    setFont(f);
  }

  QLabel::leaveEvent(ev);
}
