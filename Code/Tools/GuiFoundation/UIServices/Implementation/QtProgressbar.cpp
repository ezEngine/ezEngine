#include <PCH.h>

#include <Foundation/Utilities/Progress.h>
#include <GuiFoundation/UIServices/QtProgressbar.h>
#include <QApplication>
#include <QProgressDialog>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
#include <QtWinExtras/QWinTaskbarButton>
#include <QtWinExtras/QWinTaskbarProgress>
#endif


ezQtProgressbar::ezQtProgressbar()
{
  m_pProgress = nullptr;
  m_pDialog = nullptr;

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  m_pWinTaskBarProgress = nullptr;
  m_pWinTaskBarButton = nullptr;
#endif
}

ezQtProgressbar::~ezQtProgressbar()
{
  SetProgressbar(nullptr);
}

void ezQtProgressbar::SetProgressbar(ezProgress* pProgress)
{
  if (m_pProgress)
  {
    m_pProgress->m_Events.RemoveEventHandler(ezMakeDelegate(&ezQtProgressbar::ProgressbarEventHandler, this));
    m_pProgress = nullptr;
  }

  if (pProgress)
  {
    m_pProgress = pProgress;
    m_pProgress->m_Events.AddEventHandler(ezMakeDelegate(&ezQtProgressbar::ProgressbarEventHandler, this));
  }
}

void ezQtProgressbar::ProgressbarEventHandler(const ezProgressEvent& e)
{
  switch (e.m_Type)
  {
    case ezProgressEvent::Type::ProgressStarted:
    {
      EnsureCreated();
    }
    break;

    case ezProgressEvent::Type::ProgressEnded:
    {
      EnsureDestroyed();
    }
    break;

    case ezProgressEvent::Type::ProgressChanged:
    {
      EnsureCreated();

      ezStringBuilder sText(e.m_pProgressbar->GetMainDisplayText(), "\n", e.m_pProgressbar->GetStepDisplayText());

      m_pDialog->setLabelText(QString::fromUtf8(sText.GetData()));

      const ezUInt32 uiProMille = ezMath::Clamp<ezUInt32>((ezUInt32)(e.m_pProgressbar->GetCompletion() * 1000.0), 0, 1000);
      m_pDialog->setValue(uiProMille);

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
      if (m_pWinTaskBarProgress)
        m_pWinTaskBarProgress->setValue(uiProMille);
#endif

      if (m_pDialog->wasCanceled())
      {
        m_pProgress->UserClickedCancel();
      }

      QCoreApplication::processEvents();
    }
    break;
  }
}

void ezQtProgressbar::EnsureCreated()
{
  if (m_pDialog)
    return;

  m_pDialog = new QProgressDialog("                                                                                ", "Cancel", 0, 1000,
                                  QApplication::activeWindow());

  m_pDialog->setWindowModality(Qt::WindowModal);
  m_pDialog->setMinimumDuration((int)500);
  m_pDialog->setAutoReset(false);
  m_pDialog->setAutoClose(false);
  m_pDialog->show();

  if (!m_pProgress->AllowUserCancel())
    m_pDialog->setCancelButton(nullptr);

  if (QApplication::activeWindow())
  {
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
    m_pWinTaskBarButton = new QWinTaskbarButton(QApplication::activeWindow());
    m_pWinTaskBarButton->setWindow(QApplication::activeWindow()->windowHandle());

    m_pWinTaskBarProgress = m_pWinTaskBarButton->progress();
    m_pWinTaskBarProgress->setMinimum(0);
    m_pWinTaskBarProgress->setMaximum(1000);
    m_pWinTaskBarProgress->setValue(0);
    m_pWinTaskBarProgress->reset();
    m_pWinTaskBarProgress->show();
    m_pWinTaskBarProgress->setVisible(true);
#endif
  }
}

void ezQtProgressbar::EnsureDestroyed()
{
  if (m_pDialog)
  {
    delete m_pDialog;
    m_pDialog = nullptr;
  }

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  if (m_pWinTaskBarProgress)
  {
    m_pWinTaskBarProgress->hide();
    m_pWinTaskBarProgress = nullptr;
  }

  if (m_pWinTaskBarButton)
  {
    delete m_pWinTaskBarButton;
    m_pWinTaskBarButton = nullptr;
  }
#endif
}
