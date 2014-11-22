
#pragma once

#include <QWidget>

class ezQtBlockSignals
{
public:
  ezQtBlockSignals(QWidget* pWidget)
  {
    m_pWidget = pWidget;

    if (m_pWidget)
      m_bBlocked = m_pWidget->blockSignals(true);
  }

  ~ezQtBlockSignals()
  {
    if (m_pWidget)
      m_pWidget->blockSignals(m_bBlocked);
  }

private:
  bool m_bBlocked;
  QWidget* m_pWidget;
};