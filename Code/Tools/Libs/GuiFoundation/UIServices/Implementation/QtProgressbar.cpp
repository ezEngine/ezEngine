#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Utilities/Progress.h>
#include <GuiFoundation/UIServices/QtProgressbar.h>
#include <QApplication>
#include <QProgressDialog>

ezQtProgressbar::ezQtProgressbar() = default;

ezQtProgressbar::~ezQtProgressbar()
{
  SetProgressbar(nullptr);
  EnsureDestroyed();
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
      ++m_iNestedProcessEvents;
      EnsureCreated();
    }
    break;

    case ezProgressEvent::Type::ProgressEnded:
    {
      EnsureDestroyed();
      --m_iNestedProcessEvents;
    }
    break;

    case ezProgressEvent::Type::ProgressChanged:
    {
      ++m_iNestedProcessEvents;

      // make sure to fire all queued events before EnsureCreated()
      // because this might delete the progress dialog and then crash
      QCoreApplication::processEvents();

      EnsureCreated();

      ezStringBuilder sText(e.m_pProgressbar->GetMainDisplayText(), "\n", e.m_pProgressbar->GetStepDisplayText());

      m_pDialog->setLabelText(QString::fromUtf8(sText.GetData()));
      EZ_ASSERT_DEV(m_pDialog != nullptr, "Progress dialog was destroyed while being in use");

      const ezUInt32 uiProMille = ezMath::Clamp<ezUInt32>((ezUInt32)(e.m_pProgressbar->GetCompletion() * 1000.0), 0, 1000);
      m_pDialog->setValue(uiProMille);

      if (m_pDialog->wasCanceled())
      {
        m_pProgress->UserClickedCancel();
      }

      QCoreApplication::processEvents();
      --m_iNestedProcessEvents;
    }
    break;

    case ezProgressEvent::Type::CancelClicked:
      break;
  }
}

void ezQtProgressbar::EnsureCreated()
{
  if (m_pDialog)
    return;

  m_pDialog = new QProgressDialog("                                                                                ", "Cancel", 0, 1000, QApplication::activeWindow());

  m_pDialog->setWindowModality(Qt::WindowModal);
  m_pDialog->setMinimumDuration((int)500);
  m_pDialog->setAutoReset(false);
  m_pDialog->setAutoClose(false);
  m_pDialog->show();

  if (!m_pProgress->AllowUserCancel())
    m_pDialog->setCancelButton(nullptr);

  auto ClearDialog = [this]()
  {
    // this can happen during tests
    m_pDialog = nullptr;
  };

  m_OnDialogDestroyed = QObject::connect(m_pDialog, &QObject::destroyed, ClearDialog);
}

void ezQtProgressbar::EnsureDestroyed()
{
  if (m_pDialog)
  {
    delete m_pDialog;
    m_pDialog = nullptr;
  }
}
