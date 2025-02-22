#include <GameEngine/GameEnginePCH.h>

#include <Core/World/World.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <GameEngine/Animation/TransformComponent.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTransformComponent, 3, ezRTTINoAllocator)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Speed", m_fAnimationSpeed), // How many units per second the animation should do.
    EZ_ACCESSOR_PROPERTY("Running", IsRunning, SetRunning)->AddAttributes(new ezDefaultValueAttribute(true)), // Whether the animation should start right away.
    EZ_ACCESSOR_PROPERTY("ReverseAtEnd", GetReverseAtEnd, SetReverseAtEnd)->AddAttributes(new ezDefaultValueAttribute(true)), // If true, after coming back to the start point, the animation won't stop but turn around and continue.
    EZ_ACCESSOR_PROPERTY("ReverseAtStart", GetReverseAtStart, SetReverseAtStart)->AddAttributes(new ezDefaultValueAttribute(true)), // If true, it will not stop at the end, but turn around and continue.
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Animation"),
  }
  EZ_END_ATTRIBUTES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(SetDirectionForwards, In, "Forwards"),
    EZ_SCRIPT_FUNCTION_PROPERTY(IsDirectionForwards),
    EZ_SCRIPT_FUNCTION_PROPERTY(ToggleDirection),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void ezTransformComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  inout_stream.GetStream() << m_Flags.GetValue();
  inout_stream.GetStream() << m_AnimationTime;
  inout_stream.GetStream() << m_fAnimationSpeed;
}


void ezTransformComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  ezTransformComponentFlags::StorageType flags;
  inout_stream.GetStream() >> flags;
  m_Flags.SetValue(flags);

  inout_stream.GetStream() >> m_AnimationTime;
  inout_stream.GetStream() >> m_fAnimationSpeed;
}

void ezTransformComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  // reset to start state
  m_AnimationTime = ezTime::MakeZero();
  m_Flags.AddOrRemove(ezTransformComponentFlags::CurrentlyRunning, m_Flags.IsSet(ezTransformComponentFlags::Running));
  m_Flags.Remove(ezTransformComponentFlags::AnimationReversed);
}

bool ezTransformComponent::IsRunning(void) const
{
  return m_Flags.IsAnySet(ezTransformComponentFlags::CurrentlyRunning);
}

void ezTransformComponent::SetRunning(bool b)
{
  m_Flags.AddOrRemove(ezTransformComponentFlags::Running, b);
  m_Flags.AddOrRemove(ezTransformComponentFlags::CurrentlyRunning, b);
}

bool ezTransformComponent::GetReverseAtStart(void) const
{
  return (m_Flags.IsAnySet(ezTransformComponentFlags::AutoReturnStart));
}

void ezTransformComponent::SetReverseAtStart(bool b)
{
  m_Flags.AddOrRemove(ezTransformComponentFlags::AutoReturnStart, b);
}

bool ezTransformComponent::GetReverseAtEnd(void) const
{
  return (m_Flags.IsAnySet(ezTransformComponentFlags::AutoReturnEnd));
}

void ezTransformComponent::SetReverseAtEnd(bool b)
{
  m_Flags.AddOrRemove(ezTransformComponentFlags::AutoReturnEnd, b);
}

ezTransformComponent::ezTransformComponent() = default;
ezTransformComponent::~ezTransformComponent() = default;

void ezTransformComponent::SetDirectionForwards(bool bForwards)
{
  m_Flags.AddOrRemove(ezTransformComponentFlags::AnimationReversed, !bForwards);
}

void ezTransformComponent::ToggleDirection()
{
  m_Flags.AddOrRemove(ezTransformComponentFlags::AnimationReversed, !m_Flags.IsAnySet(ezTransformComponentFlags::AnimationReversed));
}

bool ezTransformComponent::IsDirectionForwards() const
{
  return !m_Flags.IsAnySet(ezTransformComponentFlags::AnimationReversed);
}

/*! Distance should be given in meters, but can be anything else, too. E.g. "angles" or "radians". All other values need to use the same
units. For example, when distance is given in angles, acceleration has to be in "angles per square seconds". Deceleration can be positive or
negative, internally the absolute value is used. Distance, acceleration, max velocity and time need to be positive. Time is expected to be
"in seconds". The returned value is 0, if time is negative. It is clamped to fDistanceInMeters, if time is too big.
*/

float CalculateAcceleratedMovement(
  float fDistanceInMeters, float fAcceleration, float fMaxVelocity, float fDeceleration, ezTime& ref_timeSinceStartInSec)
{
  // linear motion, if no acceleration or deceleration is present
  if ((fAcceleration <= 0.0f) && (fDeceleration <= 0.0f))
  {
    const float fDist = fMaxVelocity * (float)ref_timeSinceStartInSec.GetSeconds();

    if (fDist > fDistanceInMeters)
    {
      ref_timeSinceStartInSec = ezTime::MakeFromSeconds(fDistanceInMeters / fMaxVelocity);
      return fDistanceInMeters;
    }

    return ezMath::Max(0.0f, fDist);
  }

  // do some sanity-checks
  if ((ref_timeSinceStartInSec.GetSeconds() <= 0.0) || (fMaxVelocity <= 0.0f) || (fDistanceInMeters <= 0.0f))
    return 0.0f;

  // calculate the duration and distance of accelerated movement
  double fAccTime = 0.0;
  if (fAcceleration > 0.0)
    fAccTime = fMaxVelocity / fAcceleration;
  double fAccDist = fMaxVelocity * fAccTime * 0.5;

  // calculate the duration and distance of decelerated movement
  double fDecTime = 0.0f;
  if (fDeceleration > 0.0f)
    fDecTime = fMaxVelocity / fDeceleration;
  double fDecDist = fMaxVelocity * fDecTime * 0.5f;

  // if acceleration and deceleration take longer, than the whole path is long
  if (fAccDist + fDecDist > fDistanceInMeters)
  {
    double fFactor = fDistanceInMeters / (fAccDist + fDecDist);

    // shorten the acceleration path
    if (fAcceleration > 0.0f)
    {
      fAccDist *= fFactor;
      fAccTime = ezMath::Sqrt(2 * fAccDist / fAcceleration);
    }

    // shorten the deceleration path
    if (fDeceleration > 0.0f)
    {
      fDecDist *= fFactor;
      fDecTime = ezMath::Sqrt(2 * fDecDist / fDeceleration);
    }
  }

  // if the time is still within the acceleration phase, return accelerated distance
  if (ref_timeSinceStartInSec.GetSeconds() <= fAccTime)
    return static_cast<float>(0.5 * fAcceleration * ezMath::Square(ref_timeSinceStartInSec.GetSeconds()));

  // calculate duration and length of the path, that has maximum velocity
  const double fMaxVelDistance = fDistanceInMeters - (fAccDist + fDecDist);
  const double fMaxVelTime = fMaxVelDistance / fMaxVelocity;

  // if the time is within this phase, return the accelerated path plus the constant velocity path
  if (ref_timeSinceStartInSec.GetSeconds() <= fAccTime + fMaxVelTime)
    return static_cast<float>(fAccDist + (ref_timeSinceStartInSec.GetSeconds() - fAccTime) * fMaxVelocity);

  // if the time is, however, outside the whole path, just return the upper end
  if (ref_timeSinceStartInSec.GetSeconds() >= fAccTime + fMaxVelTime + fDecTime)
  {
    ref_timeSinceStartInSec = ezTime::MakeFromSeconds(fAccTime + fMaxVelTime + fDecTime); // clamp the time
    return fDistanceInMeters;
  }

  // calculate the time into the decelerated movement
  const double fDecTime2 = ref_timeSinceStartInSec.GetSeconds() - (fAccTime + fMaxVelTime);

  // return the distance with the decelerated movement
  return static_cast<float>(fDistanceInMeters - 0.5 * fDeceleration * ezMath::Square(fDecTime - fDecTime2));
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class ezTransformComponentPatch_1_2 : public ezGraphPatch
{
public:
  ezTransformComponentPatch_1_2()
    : ezGraphPatch("ezTransformComponent", 2)
  {
  }

  virtual void Patch(ezGraphPatchContext& ref_context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Run at Startup", "RunAtStartup");
    pNode->RenameProperty("Reverse at Start", "ReverseAtStart");
    pNode->RenameProperty("Reverse at End", "ReverseAtEnd");
  }
};

ezTransformComponentPatch_1_2 g_ezTransformComponentPatch_1_2;

//////////////////////////////////////////////////////////////////////////

class ezTransformComponentPatch_2_3 : public ezGraphPatch
{
public:
  ezTransformComponentPatch_2_3()
    : ezGraphPatch("ezTransformComponent", 3)
  {
  }

  virtual void Patch(ezGraphPatchContext& ref_context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("RunAtStartup", "Running");
  }
};

ezTransformComponentPatch_2_3 g_ezTransformComponentPatch_2_3;

EZ_STATICLINK_FILE(GameEngine, GameEngine_Animation_Implementation_TransformComponent);
