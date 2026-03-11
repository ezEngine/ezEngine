#include <Foundation/FoundationPCH.h>

#include <Foundation/Utilities/Progress.h>

static ezProgress* s_pGlobal = nullptr;

ezProgress::ezProgress() = default;

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

  if (fCompletion > m_fLastReportedCompletion + 0.001f)
  {
    m_fLastReportedCompletion = fCompletion;

    ezProgressEvent e;
    e.m_pProgressbar = this;
    e.m_Type = ezProgressEvent::Type::ProgressChanged;

    m_Events.Broadcast(e, 1);
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
  m_pRootRange = m_pActiveRange;

  while (m_pRootRange && m_pRootRange->m_pParentRange)
  {
    m_pRootRange = m_pRootRange->m_pParentRange;
  }
}

ezStringView ezProgress::GetMainDisplayText() const
{
  if (m_pRootRange == nullptr)
    return {};

  return m_pRootRange->m_sDisplayText;
}

ezStringView ezProgress::GetStepDisplayText() const
{
  if (m_pRootRange == nullptr)
    return {};

  return m_pRootRange->m_sStepDisplayText;
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

bool ezProgress::WasCanceled() const
{
  return m_bCancelClicked;
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

ezProgressRange::ezProgressRange(ezStringView sDisplayText, ezUInt32 uiSteps, bool bAllowCancel, ezProgress* pProgressbar /*= nullptr*/)
{
  EZ_ASSERT_DEV(uiSteps > 0, "Every progress range must have at least one step to complete");

  m_iCurrentStep = -1;
  m_fWeightedCompletion = -1.0;
  m_fSummedWeight = (double)uiSteps;

  Init(sDisplayText, bAllowCancel, pProgressbar);
}

ezProgressRange::ezProgressRange(ezStringView sDisplayText, bool bAllowCancel, ezProgress* pProgressbar /*= nullptr*/)
{
  Init(sDisplayText, bAllowCancel, pProgressbar);
}

void ezProgressRange::Init(ezStringView sDisplayText, bool bAllowCancel, ezProgress* pProgressbar)
{
  if (pProgressbar == nullptr)
    m_pProgressbar = ezProgress::GetGlobalProgressbar();
  else
    m_pProgressbar = pProgressbar;

  EZ_ASSERT_DEV(m_pProgressbar != nullptr, "No global progress-bar context available.");

  m_bAllowCancel = bAllowCancel;
  m_sDisplayText = sDisplayText;

  m_pParentRange = m_pProgressbar->m_pActiveRange;

  if (m_pParentRange == nullptr)
  {
    m_fPercentageBase = 0.0;
    m_fPercentageRange = 1.0;
  }
  else
  {
    m_pParentRange->ComputeCurStepBaseAndRange(m_fPercentageBase, m_fPercentageRange);
  }

  m_pProgressbar->SetActiveRange(this);
}

ezProgressRange::~ezProgressRange()
{
  m_pProgressbar->SetCompletion((float)(m_fPercentageBase + m_fPercentageRange));
  m_pProgressbar->SetActiveRange(m_pParentRange);
}

ezProgress* ezProgressRange::GetProgressbar() const
{
  return m_pProgressbar;
}

void ezProgressRange::SetStepWeighting(ezUInt32 uiStep, float fWeight)
{
  EZ_ASSERT_DEV(m_fSummedWeight > 0.0, "This function is only supported if ProgressRange was initialized with steps");

  m_fSummedWeight -= GetStepWeight(uiStep);
  m_fSummedWeight += fWeight;
  m_StepWeights[uiStep] = fWeight;
}

float ezProgressRange::GetStepWeight(ezUInt32 uiStep) const
{
  const float* pOldWeight = m_StepWeights.GetValue(uiStep);
  return pOldWeight != nullptr ? *pOldWeight : 1.0f;
}

void ezProgressRange::ComputeCurStepBaseAndRange(double& out_base, double& out_range)
{
  const double internalBase = ezMath::Max(m_fWeightedCompletion, 0.0) / m_fSummedWeight;
  const double internalRange = GetStepWeight(ezMath::Max(m_iCurrentStep, 0)) / m_fSummedWeight;

  out_range = internalRange * m_fPercentageRange;
  out_base = m_fPercentageBase + (internalBase * m_fPercentageRange);

  EZ_ASSERT_DEBUG(out_base <= 1.0f, "Invalid range");
  EZ_ASSERT_DEBUG(out_range <= 1.0f, "Invalid range");
  EZ_ASSERT_DEBUG(out_base + out_range <= 1.0f, "Invalid range");
}

bool ezProgressRange::BeginNextStep(ezStringView sStepDisplayText, ezUInt32 uiNumSteps)
{
  EZ_ASSERT_DEV(m_fSummedWeight > 0.0, "This function is only supported if ProgressRange was initialized with steps");

  m_sStepDisplayText = sStepDisplayText;

  for (ezUInt32 i = 0; i < uiNumSteps; ++i)
  {
    m_fWeightedCompletion += GetStepWeight(m_iCurrentStep + i);
  }
  m_iCurrentStep += uiNumSteps;

  const double internalCompletion = m_fWeightedCompletion / m_fSummedWeight;
  const double finalCompletion = m_fPercentageBase + internalCompletion * m_fPercentageRange;

  m_pProgressbar->SetCompletion((float)finalCompletion);

  return !m_pProgressbar->WasCanceled();
}

bool ezProgressRange::SetCompletion(double fCompletionFactor)
{
  EZ_ASSERT_DEV(m_fSummedWeight == 0.0, "This function is only supported if ProgressRange was initialized without steps");

  const double finalCompletion = m_fPercentageBase + fCompletionFactor * m_fPercentageRange;

  m_pProgressbar->SetCompletion((float)finalCompletion);

  return !m_pProgressbar->WasCanceled();
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
