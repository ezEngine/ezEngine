#include <GameEnginePCH.h>

#include <Core/World/World.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <GameEngine/Animation/TransformComponent.h>
#include <VisualScript/VisualScriptInstance.h>

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
    new ezCategoryAttribute("Transform"),
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

void ezTransformComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  stream.GetStream() << m_Flags.GetValue();
  stream.GetStream() << m_AnimationTime;
  stream.GetStream() << m_fAnimationSpeed;
}


void ezTransformComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  ezTransformComponentFlags::StorageType flags;
  stream.GetStream() >> flags;
  m_Flags.SetValue(flags);

  stream.GetStream() >> m_AnimationTime;
  stream.GetStream() >> m_fAnimationSpeed;
}

bool ezTransformComponent::IsRunning(void) const
{
  return m_Flags.IsAnySet(ezTransformComponentFlags::Running);
}

void ezTransformComponent::SetRunning(bool b)
{
  m_Flags.AddOrRemove(ezTransformComponentFlags::Running, b);
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

ezTransformComponent::ezTransformComponent()
{
  m_fAnimationSpeed = 1.0f;
  m_AnimationTime.SetZero();
}

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
  float fDistanceInMeters, float fAcceleration, float fMaxVelocity, float fDeceleration, ezTime& fTimeSinceStartInSec)
{
  // linear motion, if no acceleration or deceleration is present
  if ((fAcceleration <= 0.0f) && (fDeceleration <= 0.0f))
  {
    const float fDist = fMaxVelocity * (float)fTimeSinceStartInSec.GetSeconds();

    if (fDist > fDistanceInMeters)
    {
      fTimeSinceStartInSec = ezTime::Seconds(fDistanceInMeters / fMaxVelocity);
      return fDistanceInMeters;
    }

    return ezMath::Max(0.0f, fDist);
  }

  // do some sanity-checks
  if ((fTimeSinceStartInSec.GetSeconds() <= 0.0) || (fMaxVelocity <= 0.0f) || (fDistanceInMeters <= 0.0f))
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
  float fDecDist = fMaxVelocity * fDecTime * 0.5f;

  // if acceleration and deceleration take longer, than the whole path is long
  if (fAccDist + fDecDist > fDistanceInMeters)
  {
    float fFactor = fDistanceInMeters / (fAccDist + fDecDist);

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
  if (fTimeSinceStartInSec.GetSeconds() <= fAccTime)
    return 0.5 * fAcceleration * ezMath::Square(fTimeSinceStartInSec.GetSeconds());

  // calculate duration and length of the path, that has maximum velocity
  const double fMaxVelDistance = fDistanceInMeters - (fAccDist + fDecDist);
  const double fMaxVelTime = fMaxVelDistance / fMaxVelocity;

  // if the time is within this phase, return the accelerated path plus the constant velocity path
  if (fTimeSinceStartInSec.GetSeconds() <= fAccTime + fMaxVelTime)
    return fAccDist + (fTimeSinceStartInSec.GetSeconds() - fAccTime) * fMaxVelocity;

  // if the time is, however, outside the whole path, just return the upper end
  if (fTimeSinceStartInSec.GetSeconds() >= fAccTime + fMaxVelTime + fDecTime)
  {
    fTimeSinceStartInSec = ezTime::Seconds(fAccTime + fMaxVelTime + fDecTime); // clamp the time
    return fDistanceInMeters;
  }

  // calculate the time into the decelerated movement
  const double fDecTime2 = fTimeSinceStartInSec.GetSeconds() - (fAccTime + fMaxVelTime);

  // return the distance with the decelerated movement
  return fDistanceInMeters - 0.5 * fDeceleration * ezMath::Square(fDecTime - fDecTime2);
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

  virtual void Patch(ezGraphPatchContext& context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
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

  virtual void Patch(ezGraphPatchContext& context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("RunAtStartup", "Running");
  }
};

ezTransformComponentPatch_2_3 g_ezTransformComponentPatch_2_3;

EZ_STATICLINK_FILE(GameEngine, GameEngine_Components_Implementation_TransformComponent);
