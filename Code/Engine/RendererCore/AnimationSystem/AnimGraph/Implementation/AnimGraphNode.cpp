#include <RendererCorePCH.h>

#include <AnimationSystem/AnimGraph/AnimGraph.h>
#include <AnimationSystem/AnimationClipResource.h>
#include <AnimationSystem/SkeletonResource.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimGraphNode, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("CustomTitle", GetCustomNodeTitle, SetCustomNodeTitle),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezComparisonOperator, 1)
  EZ_ENUM_CONSTANTS(ezComparisonOperator::Equal, ezComparisonOperator::NotEqual)
  EZ_ENUM_CONSTANTS(ezComparisonOperator::Less, ezComparisonOperator::LessEqual)
  EZ_ENUM_CONSTANTS(ezComparisonOperator::Greater, ezComparisonOperator::GreaterEqual)
EZ_END_STATIC_REFLECTED_ENUM;
// clang-format on

ezAnimGraphNode::ezAnimGraphNode() = default;
ezAnimGraphNode::~ezAnimGraphNode() = default;

ezResult ezAnimGraphNode::SerializeNode(ezStreamWriter& stream) const
{
  stream.WriteVersion(1);

  // no need to serialize this, not used at runtime
  //stream << m_CustomNodeTitle;

  return EZ_SUCCESS;
}

ezResult ezAnimGraphNode::DeserializeNode(ezStreamReader& stream)
{
  stream.ReadVersion(1);

  // no need to serialize this, not used at runtime
  //stream >> m_CustomNodeTitle;

  return EZ_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezAnimRampUpDown, ezNoBase, 1, ezRTTIDefaultAllocator<ezAnimRampUpDown>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("RampUp", m_RampUp),
    EZ_MEMBER_PROPERTY("RampDown", m_RampDown),
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;
// clang-format on

ezResult ezAnimRampUpDown::Serialize(ezStreamWriter& stream) const
{
  stream << m_RampUp;
  stream << m_RampDown;

  return EZ_SUCCESS;
}

ezResult ezAnimRampUpDown::Deserialize(ezStreamReader& stream)
{
  stream >> m_RampUp;
  stream >> m_RampDown;

  return EZ_SUCCESS;
}

void ezAnimRampUpDown::RampWeightUpOrDown(float& inout_fWeight, float fTargetWeight, ezTime tDiff) const
{
  if (inout_fWeight < fTargetWeight)
  {
    if (m_RampUp.IsZeroOrNegative())
    {
      inout_fWeight = fTargetWeight;
    }
    else
    {
      inout_fWeight += tDiff.AsFloatInSeconds() / m_RampUp.AsFloatInSeconds();
      inout_fWeight = ezMath::Min(inout_fWeight, fTargetWeight);
    }
  }
  else if (inout_fWeight > fTargetWeight)
  {
    if (m_RampDown.IsZeroOrNegative())
    {
      inout_fWeight = fTargetWeight;
    }
    else
    {
      inout_fWeight -= tDiff.AsFloatInSeconds() / m_RampDown.AsFloatInSeconds();
      inout_fWeight = ezMath::Max(fTargetWeight, inout_fWeight);
    }
  }
}

bool ezAnimState::WillStateBeOff(bool bTriggerActive) const
{
  return m_State == State::Off && !bTriggerActive;
}

void ezAnimState::UpdateState(ezTime tDiff)
{
  EZ_ASSERT_DEV(!m_Duration.IsZeroOrNegative(), "Invalid animation clip duration");

  m_bHasTransitioned = false;

  // how much time delta to apply to the weight ramp
  ezTime tRampUpDownDiff = tDiff;

  // first update the state machine transitions

  if (m_State == State::Off)
  {
    // in the OFF state we can only transition to the ramping up state

    if (m_bTriggerActive == false)
      return;

    m_fNormalizedPlaybackPosition = 0.0f;
    m_State = State::StartedRampUp;
  }
  else if (m_State == State::StartedRampUp || m_State == State::RampingUp)
  {
    // StartedRampUp MUST be active for a full frame, even if the weight immediately reaches maximum
    // because other code might use this state to fire events

    m_State = State::RampingUp;

    // we must transition to Running here, instead of after the weight update, otherwise the StartedRampUp state could get skipped
    if (m_fCurWeight >= 1.0f)
    {
      m_bRequireLoopForRampDown = true; // force one frame evaluation, before ramp down is allowed
      m_State = State::Running;
    }

    // already taken care of in the Running state check below
    //if (!m_bTriggerActive && m_bImmediateRampDown)
    //  m_State = State::StartedRampDown;
  }
  else if (m_State == State::StartedRampDown || m_State == State::RampingDown)
  {
    // StartedRampDown MUST be active for a full frame, even if the weight immediately reaches minimum
    // because other code might use this state to fire events

    m_State = State::RampingDown;

    // we must transition to Finished here, instead of after the weight update, otherwise the StartedRampDown state could get skipped
    if (m_fCurWeight <= 0.0f)
      m_State = State::Finished;

    // can only ramp up at arbitrary times, if the animation is looped
    if (m_bTriggerActive && m_bImmediateRampUp && m_bLoop)
      m_State = State::StartedRampUp;
  }
  else if (m_State == State::Finished)
  {
    // in the finished state we can either switch off or restart
    // if m_bImmediateRampUp is off, the Finished state should be visible for a full frame

    if (m_bTriggerActive && m_bImmediateRampUp)
    {
      m_State = State::StartedRampUp;
    }
    else
    {
      m_State = State::Off;
      return;
    }
  }

  // no "else if" here, because the Running state can be skipped and transition immediately to the StartedRampDown state
  if (m_State == State::Running)
  {
    if (!m_bTriggerActive && m_bImmediateRampDown)
    {
      m_State = State::StartedRampDown;
    }
  }

  float fInvDuration = 1.0f / m_Duration.AsFloatInSeconds();
  float fNormalizedStep = tDiff.AsFloatInSeconds() * fInvDuration;

  // calculate the new playback position
  {
    m_fNormalizedPlaybackPosition += m_fPlaybackSpeed * fNormalizedStep;

    if (m_fNormalizedPlaybackPosition > 1.0f && m_DurationOfQueued.IsPositive())
    {
      m_bHasTransitioned = true;

      tDiff = (m_fNormalizedPlaybackPosition - 1.0f) * m_Duration;

      m_Duration = m_DurationOfQueued;
      fInvDuration = 1.0f / m_Duration.AsFloatInSeconds();
      fNormalizedStep = tDiff.AsFloatInSeconds() * fInvDuration;
    }
  }

  if (m_State == State::Running)
  {
    bool bIsInRampDownArea = false;

    const float tRampDownNorm = m_AnimRamp.m_RampDown.AsFloatInSeconds() * fInvDuration;
    float tInRampDownArea;

    if (m_fPlaybackSpeed >= 0)
    {
      // ramp down area at the end of the clip
      const float tEnd = 1.0f - tRampDownNorm;

      bIsInRampDownArea = m_fNormalizedPlaybackPosition > tEnd;
      tInRampDownArea = ezMath::Min(fNormalizedStep, m_fNormalizedPlaybackPosition - tEnd);
    }
    else
    {
      // ramp down area at the beginning of the clip

      bIsInRampDownArea = m_fNormalizedPlaybackPosition < tRampDownNorm;
      tInRampDownArea = ezMath::Min(fNormalizedStep, tRampDownNorm - m_fNormalizedPlaybackPosition);
    }

    if (m_bLoop)
    {
      // exit the loop, if the user deactivated the clip
      if (!m_bTriggerActive)
      {
        if (!m_bRequireLoopForRampDown && bIsInRampDownArea)
        {
          m_State = State::StartedRampDown;

          // recompute tRampUpDownDiff for exact adjustment
          tRampUpDownDiff = tInRampDownArea * m_Duration;
        }
      }
    }
    else
    {
      if (bIsInRampDownArea)
      {
        m_State = State::StartedRampDown;

        // recompute tRampUpDownDiff for exact adjustment
        tRampUpDownDiff = tInRampDownArea * m_Duration;
      }
    }

    m_bRequireLoopForRampDown = bIsInRampDownArea;
  }

  // update the overall weight
  {
    if (m_State == State::StartedRampUp || m_State == State::RampingUp)
    {
      m_AnimRamp.RampWeightUpOrDown(m_fCurWeight, 1.0f, tRampUpDownDiff);
    }
    else if (m_State == State::StartedRampDown || m_State == State::RampingDown)
    {
      m_AnimRamp.RampWeightUpOrDown(m_fCurWeight, 0.0f, tRampUpDownDiff);
    }
  }

  // finally loop or clamp the playback position
  if (m_bLoop)
  {
    // if playback speed is positive
    while (m_fNormalizedPlaybackPosition > 1.0f)
    {
      m_fNormalizedPlaybackPosition -= 1.0f;
      m_bRequireLoopForRampDown = false;
    }

    // if playback speed is negative
    while (m_fNormalizedPlaybackPosition < 0.0)
    {
      m_fNormalizedPlaybackPosition += 1.0f;
      m_bRequireLoopForRampDown = false;
    }
  }
  else
  {
    if (m_fNormalizedPlaybackPosition > 1.0f)
    {
      // if playback speed is positive
      m_fNormalizedPlaybackPosition = 1.0f;
    }
    else if (m_fNormalizedPlaybackPosition < 0.0f)
    {
      // if playback speed is negative
      m_fNormalizedPlaybackPosition = 0.0f;
    }
  }
}

bool ezComparisonOperator::Compare(ezComparisonOperator::Enum cmp, double f1, double f2)
{
  switch (cmp)
  {
    case ezComparisonOperator::Equal:
      return f1 == f2;
    case ezComparisonOperator::NotEqual:
      return f1 != f2;
    case ezComparisonOperator::Less:
      return f1 < f2;
    case ezComparisonOperator::LessEqual:
      return f1 <= f2;
    case ezComparisonOperator::Greater:
      return f1 > f2;
    case ezComparisonOperator::GreaterEqual:
      return f1 >= f2;

      EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return false;
}
