#include <GuiFoundation/PCH.h>
#include <GuiFoundation/UIServices/ProgressBar.h>
#include <Foundation/Math/Math.h>
#include <QProgressDialog>
#include <QApplication>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  #include <QtWinExtras/QWinTaskbarButton>
  #include <QtWinExtras/QWinTaskbarProgress>

  QWinTaskbarButton* QtProgressBar::s_pWinTaskBarButton = nullptr;
  QWinTaskbarProgress* QtProgressBar::s_pWinTaskBarProgress = nullptr;
#endif

QtProgressBar* QtProgressBar::s_pActiveRange = nullptr;
QProgressDialog* QtProgressBar::s_pQtProgressBar = nullptr;

ezEvent<const QtProgressBar::Event&> QtProgressBar::s_Events;

QtProgressBar::QtProgressBar(const char* szMainText, ezUInt32 uiItems, bool bCanBeCanceled, ezTime TimeInvisible)
{
  m_dBaseValue = 0.0;
  m_dPercentageRange = 100.0;
  m_uiMaxItems = ezMath::Max<ezUInt32>(1, uiItems);
  m_uiCurItems = 0;
  m_pParentRange = s_pActiveRange;
  m_bCanBeCanceled = bCanBeCanceled;
  m_sCurrentMainText = szMainText;
  m_sCurrentSubText = "";
  m_uiWorkingOnItems = 0;

  if (m_pParentRange)
  {
    m_dPercentageRange = (m_pParentRange->m_dPercentageRange / m_pParentRange->m_uiMaxItems) * m_pParentRange->m_uiWorkingOnItems;
    m_dBaseValue = m_pParentRange->m_dBaseValue + m_dPercentageRange * m_pParentRange->m_uiCurItems;

    if (!m_pParentRange->m_bCanBeCanceled)
      m_bCanBeCanceled = false;
  }
  else
  {
    Event e;
    e.m_Type = Event::Type::ProgressStarted;
    e.m_uiProMilleValue = 0;

    s_Events.Broadcast(e);
  }

  if (!s_pQtProgressBar)
  {
    s_pQtProgressBar = new QProgressDialog("                                                                                ",
                                           "Cancel", 0, 1000, QApplication::activeWindow());

    s_pQtProgressBar->setWindowModality(Qt::WindowModal);
    s_pQtProgressBar->setMinimumDuration((int) TimeInvisible.GetMilliseconds());
    s_pQtProgressBar->setAutoReset(false);
    s_pQtProgressBar->setAutoClose(false);

    if (QApplication::activeWindow())
    {
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
      s_pWinTaskBarButton = new QWinTaskbarButton(QApplication::activeWindow());
      s_pWinTaskBarButton->setWindow(QApplication::activeWindow()->windowHandle());

      s_pWinTaskBarProgress = s_pWinTaskBarButton->progress();
      s_pWinTaskBarProgress->setMinimum(0);
      s_pWinTaskBarProgress->setMaximum(1000);
      s_pWinTaskBarProgress->setValue(0);
      s_pWinTaskBarProgress->reset();
      s_pWinTaskBarProgress->show();
      s_pWinTaskBarProgress->setVisible(true);
#endif
    }
  }

  if (!m_bCanBeCanceled)
    s_pQtProgressBar->setCancelButton(nullptr);

  s_pActiveRange = this;

  UpdateDisplay();
}

QtProgressBar::~QtProgressBar()
{
  s_pActiveRange->InternalUpdate(nullptr, 0);

  s_pActiveRange = m_pParentRange;

  if (!s_pActiveRange)
  {
    delete s_pQtProgressBar;
    s_pQtProgressBar = nullptr;

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
    if (s_pWinTaskBarProgress)
    {
      s_pWinTaskBarProgress->hide();
      s_pWinTaskBarProgress = nullptr;
    }

    delete s_pWinTaskBarButton;
    s_pWinTaskBarButton = nullptr;
#endif

    Event e;
    e.m_Type = Event::Type::ProgressEnded;
    e.m_uiProMilleValue = 1000;

    s_Events.Broadcast(e);
  }
  else
    s_pActiveRange->UpdateDisplay();
}

void QtProgressBar::UpdateDisplay()
{
  ezStringBuilder sText(m_sCurrentMainText, "\n", m_sCurrentSubText);

  s_pQtProgressBar->setLabelText(QString::fromUtf8(sText.GetData()));
}

void QtProgressBar::WorkingOnNextItem(const char* szSubText, ezUInt32 uiWorkingOnItems)
{
  if (!s_pActiveRange || !s_pQtProgressBar)
    return;

  s_pActiveRange->InternalUpdate(szSubText, uiWorkingOnItems);
}

void QtProgressBar::InternalUpdate(const char* szSubText, ezUInt32 uiWorkingOnItems)
{
  if (szSubText)
    m_sCurrentSubText = szSubText;

  m_uiCurItems = ezMath::Min<ezUInt32>(m_uiCurItems + m_uiWorkingOnItems, m_uiMaxItems);

  double dStep = m_dPercentageRange / m_uiMaxItems;
  double dPercentage = m_dBaseValue + m_uiCurItems * dStep;

  ezUInt32 uiProMille = (ezUInt32)(dPercentage * 10.0);

  if (uiProMille != s_pQtProgressBar->value())
  {
    s_pQtProgressBar->setValue(uiProMille);

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
    if (s_pWinTaskBarProgress)
      s_pWinTaskBarProgress->setValue(uiProMille);
#endif

    Event e;
    e.m_Type = Event::Type::ProgressChanged;
    e.m_uiProMilleValue = uiProMille;

    s_Events.Broadcast(e);
  }

  m_uiWorkingOnItems = uiWorkingOnItems;
  UpdateDisplay();
}

bool QtProgressBar::WasProgressBarCanceled()
{
  if (s_pActiveRange && !s_pActiveRange->m_bCanBeCanceled)
    return false;

  if (s_pQtProgressBar)
    return s_pQtProgressBar->wasCanceled();

  return false;
}

