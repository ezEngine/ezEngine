#include <PCH.h>

#include <Foundation/Utilities/Progress.h>

static ezProgress* s_pGlobal = nullptr;

ezProgress::ezProgress()
{
  m_pActiveRange = nullptr;
  m_bCancelClicked = false;
  m_fLastReportedCompletion = 0.0f;
  m_fCurrentCompletion = 0.0;
}

ezProgress::~ezProgress()
{
  if (s_pGlobal == this)
  {
    s_pGlobal = nullptr;
  }
}

float ezProgress::GetCompletion() const
{
  return m_fCurrentCompletion;
}

void ezProgress::SetCompletion(float fCompletion)
{
  EZ_ASSERT_DEV(fCompletion >= 0.0f && fCompletion <= 1.0f, "Completion value {0} is out of valid range", fCompletion);

  m_fCurrentCompletion = fCompletion;

  if (fCompletion > m_fLastReportedCompletion + 0.05f)
  {
    m_fLastReportedCompletion = fCompletion;

    ezProgressEvent e;
    e.m_pProgressbar = this;
    e.m_Type = ezProgressEvent::Type::ProgressChanged;

    m_Events.Broadcast(e);
  }
}

void ezProgress::SetActiveRange(ezProgressRange* pRange)
{
  if (m_pActiveRange == nullptr && pRange != nullptr)
  {
    m_fLastReportedCompletion = 0.0;
    m_fCurrentCompletion = 0.0;
    m_bCancelClicked = false;
    m_bEnableCancel = pRange->m_bAllowCancel;

    ezProgressEvent e;
    e.m_pProgressbar = this;
    e.m_Type = ezProgressEvent::Type::ProgressStarted;

    m_Events.Broadcast(e);
  }

  if (m_pActiveRange != nullptr && pRange == nullptr)
  {
    ezProgressEvent e;
    e.m_pProgressbar = this;
    e.m_Type = ezProgressEvent::Type::ProgressEnded;

    m_Events.Broadcast(e);
  }

  m_pActiveRange = pRange;
}

const char* ezProgress::GetMainDisplayText() const
{
  if (m_pActiveRange == nullptr)
    return "";

  return m_pActiveRange->m_sDisplayText;
}

const char* ezProgress::GetStepDisplayText() const
{
  if (m_pActiveRange == nullptr)
    return "";

  return m_pActiveRange->m_sStepDisplayText;
}

void ezProgress::UserClickedCancel()
{
  if (m_bCancelClicked)
    return;

  m_bCancelClicked = true;

  ezProgressEvent e;
  e.m_Type = ezProgressEvent::Type::CancelClicked;
  e.m_pProgressbar = this;

  m_Events.Broadcast(e, 1);
}

bool ezProgress::AllowUserCancel() const
{
  return m_bEnableCancel;
}

ezProgress* ezProgress::GetGlobalProgressbar()
{
  if (!s_pGlobal)
  {
    static ezProgress s_Global;
    return &s_Global;
  }

  return s_pGlobal;
}

void ezProgress::SetGlobalProgressbar(ezProgress* pProgress)
{
  s_pGlobal = pProgress;
}

//////////////////////////////////////////////////////////////////////////

ezProgressRange::ezProgressRange(const char* szDisplayText, ezUInt32 uiSteps, bool bAllowCancel, ezProgress* pProgressbar /*= nullptr*/)
{
  EZ_ASSERT_DEV(uiSteps > 0, "Every progress range must have at least one step to complete");

  // add one step for the very first 'BeginNextStep' to have no effect
  uiSteps += 1;

  if (pProgressbar == nullptr)
    m_pProgressbar = ezProgress::GetGlobalProgressbar();
  else
    m_pProgressbar = pProgressbar;

  EZ_ASSERT_DEV(m_pProgressbar != nullptr, "No global progress-bar context available.");

  m_bAllowCancel = bAllowCancel;
  m_uiCurrentStep = 0;
  m_sDisplayText = szDisplayText;
  m_StepWeights.SetCountUninitialized(uiSteps);
  m_SummedWeight = (double)uiSteps - 1.0;

  m_StepWeights[0] = 0; // first 'BeginNextStep' should not advance the progress at all
  for (ezUInt32 s = 1; s < uiSteps; ++s)
    m_StepWeights[s] = 1.0f;

  m_pParentRange = m_pProgressbar->m_pActiveRange;

  if (m_pParentRange == nullptr)
  {
    m_PercentageBase = 0.0;
    m_PercentageRange = 1.0;
  }
  else
  {
    m_pParentRange->ComputeCurStepBaseAndRange(m_PercentageBase, m_PercentageRange);
  }

  m_pProgressbar->SetActiveRange(this);
}

ezProgressRange::~ezProgressRange()
{
  m_pProgressbar->SetCompletion((float)(m_PercentageBase + m_PercentageRange));
  m_pProgressbar->SetActiveRange(m_pParentRange);
}

ezProgress* ezProgressRange::GetProgressbar() const
{
  return m_pProgressbar;
}

void ezProgressRange::SetStepWeighting(ezUInt32 uiStep, float fWeigth)
{
  m_SummedWeight -= m_StepWeights[uiStep + 1];
  m_SummedWeight += fWeigth;
  m_StepWeights[uiStep + 1] = fWeigth;
}

void ezProgressRange::ComputeCurStepBaseAndRange(double& out_base, double& out_range)
{
  const double internalBase = ComputeInternalCompletion();
  const double internalRange = m_StepWeights[ezMath::Min(m_uiCurrentStep, m_StepWeights.GetCount() - 1)] / m_SummedWeight;

  out_range = internalRange * m_PercentageRange;
  out_base = m_PercentageBase + (internalBase * m_PercentageRange);

  EZ_ASSERT_DEBUG(out_base <= 1.0f, "Invalid range");
  EZ_ASSERT_DEBUG(out_range <= 1.0f, "Invalid range");
  EZ_ASSERT_DEBUG(out_base + out_range <= 1.0f, "Invalid range");
}

double ezProgressRange::ComputeInternalCompletion() const
{
  double internalBase = 0.0f;

  for (ezUInt32 step = 0; step < m_uiCurrentStep; ++step)
  {
    internalBase += m_StepWeights[step];
  }

  return internalBase /= m_SummedWeight;
}

void ezProgressRange::BeginNextStep(const char* szStepDisplayText, ezUInt32 uiNumSteps)
{
  m_sStepDisplayText = szStepDisplayText;

  m_uiCurrentStep += uiNumSteps;

  // there is one extra step added as an empty step
  EZ_ASSERT_DEBUG(m_uiCurrentStep < m_StepWeights.GetCount(), "Too many steps completed! ({0} of {1})", m_uiCurrentStep,
                  m_StepWeights.GetCount());

  const double internalCompletion = ComputeInternalCompletion();

  const double finalCompletion = m_PercentageBase + internalCompletion * m_PercentageRange;

  m_pProgressbar->SetCompletion((float)finalCompletion);
}

bool ezProgressRange::WasCanceled() const
{
  if (!m_pProgressbar->m_bCancelClicked)
    return false;

  const ezProgressRange* pCur = this;

  // if there is any action in the stack above, that cannot be canceled
  // all sub actions should be fully executed, even if they could be canceled
  while (pCur)
  {
    if (!pCur->m_bAllowCancel)
      return false;

    pCur = pCur->m_pParentRange;
  }

  return true;
}



EZ_STATICLINK_FILE(Foundation, Foundation_Utilities_Implementation_Progress);

