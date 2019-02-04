#include <PCH.h>

#include <Core/World/World.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <GameEngine/Components/TransformComponent.h>
#include <VisualScript/VisualScriptInstance.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTransformComponent, 2, ezRTTINoAllocator)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Speed", m_fAnimationSpeed), // How many units per second the animation should do.
    EZ_ACCESSOR_PROPERTY("RunAtStartup", GetAnimatingAtStartup, SetAnimatingAtStartup), // Whether the animation should start right away.
    EZ_ACCESSOR_PROPERTY("ReverseAtStart", GetAutoReturnStart, SetAutoReturnStart), // If true, it will not stop at the end, but turn around and continue.
    EZ_ACCESSOR_PROPERTY("ReverseAtEnd", GetAutoReturnEnd, SetAutoReturnEnd), // If true, after coming back to the start point, the animation won't stop but turn around and continue.
    EZ_ACCESSOR_PROPERTY("AutoToggleDirection", GetAutoToggleDirection, SetAutoToggleDirection), // If true, the animation might stop at start/end points, but set toggle its direction state. Triggering the animation again, means it will run in the reverse direction.
  }
  EZ_END_PROPERTIES;
    EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Transform"),
  }
  EZ_END_ATTRIBUTES;
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

ezTransformComponent::ezTransformComponent()
{
  m_fAnimationSpeed = 1.0f;
  m_AnimationTime.SetZero();
}

void ezTransformComponent::ResumeAnimation()
{
  m_Flags.Add(ezTransformComponentFlags::Autorun);
  m_Flags.Remove(ezTransformComponentFlags::Paused);
}

void ezTransformComponent::SetAnimationPaused(bool bPaused)
{
  m_Flags.AddOrRemove(ezTransformComponentFlags::Paused, bPaused);
}

void ezTransformComponent::SetDirectionForwards(bool bForwards)
{
  m_Flags.AddOrRemove(ezTransformComponentFlags::AnimationReversed, !bForwards);
}

void ezTransformComponent::ReverseDirection()
{
  m_Flags.AddOrRemove(ezTransformComponentFlags::AnimationReversed, !m_Flags.IsAnySet(ezTransformComponentFlags::AnimationReversed));
}

bool ezTransformComponent::IsDirectionForwards() const
{
  return !m_Flags.IsAnySet(ezTransformComponentFlags::AnimationReversed);
}

bool ezTransformComponent::IsAnimationRunning() const
{
  return m_Flags.IsAnySet(ezTransformComponentFlags::Autorun);
}

/*! Distance should be given in meters, but can be anything else, too. E.g. "angles" or "radians". All other values need to use the same
units. For example, when distance is given in angles, acceleration has to be in "angles per square seconds". Deceleration can be positive or
negative, internally the absolute value is used. Distance, acceleration, max velocity and time need to be positive. Time is expected to be
"in seconds". The returned value is 0, if time is negative. It is clamped to fDistanceInMeters, if time is too big.
*/

float CalculateAcceleratedMovement(float fDistanceInMeters, float fAcceleration, float fMaxVelocity, float fDeceleration,
                                   ezTime& fTimeSinceStartInSec)
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
  float fAccTime = 0.0f;
  if (fAcceleration > 0.0f)
    fAccTime = fMaxVelocity / fAcceleration;
  float fAccDist = fMaxVelocity * fAccTime * 0.5f;

  // calculate the duration and distance of decelerated movement
  float fDecTime = 0.0f;
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
    return 0.5f * fAcceleration * ezMath::Square((float)fTimeSinceStartInSec.GetSeconds());

  // calculate duration and length of the path, that has maximum velocity
  const float fMaxVelDistance = fDistanceInMeters - (fAccDist + fDecDist);
  const float fMaxVelTime = fMaxVelDistance / fMaxVelocity;

  // if the time is within this phase, return the accelerated path plus the constant velocity path
  if (fTimeSinceStartInSec.GetSeconds() <= fAccTime + fMaxVelTime)
    return fAccDist + ((float)fTimeSinceStartInSec.GetSeconds() - fAccTime) * fMaxVelocity;

  // if the time is, however, outside the whole path, just return the upper end
  if (fTimeSinceStartInSec.GetSeconds() >= fAccTime + fMaxVelTime + fDecTime)
  {
    fTimeSinceStartInSec = ezTime::Seconds(fAccTime + fMaxVelTime + fDecTime); // clamp the time
    return fDistanceInMeters;
  }

  // calculate the time into the decelerated movement
  const float fDecTime2 = (float)fTimeSinceStartInSec.GetSeconds() - (fAccTime + fMaxVelTime);

  // return the distance with the decelerated movement
  return fDistanceInMeters - 0.5f * fDeceleration * ezMath::Square(fDecTime - fDecTime2);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_TransformComponent, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_TransformComponent>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Components/Transform")
  }
  EZ_END_ATTRIBUTES;
  EZ_BEGIN_PROPERTIES
  {
    EZ_INPUT_EXECUTION_PIN("Play", 0),
    EZ_INPUT_EXECUTION_PIN("Pause", 1),
    EZ_INPUT_EXECUTION_PIN("Reverse", 2),
    EZ_INPUT_DATA_PIN("Component", 0, ezVisualScriptDataPinType::ComponentHandle),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezVisualScriptNode_TransformComponent::ezVisualScriptNode_TransformComponent() {}

void ezVisualScriptNode_TransformComponent::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  if (m_hComponent.IsInvalidated())
    return;

  ezComponent* pComponent = nullptr;
  if (!pInstance->GetWorld()->TryGetComponent(m_hComponent, pComponent))
    return;

  if (!pComponent->GetDynamicRTTI()->IsDerivedFrom<ezTransformComponent>())
    return;

  ezTransformComponent* pTransform = static_cast<ezTransformComponent*>(pComponent);

  switch (uiExecPin)
  {
    case 0:
      pTransform->ResumeAnimation();
      return;

    case 1:
      pTransform->SetAnimationPaused(true);
      return;

    case 2:
      pTransform->ReverseDirection();
      return;
  }
}

void* ezVisualScriptNode_TransformComponent::GetInputPinDataPointer(ezUInt8 uiPin)
{
  switch (uiPin)
  {
    case 0:
      return &m_hComponent;
  }

  return nullptr;
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
    pNode->RenameProperty("Auto Toggle Direction", "AutoToggleDirection");
  }
};

ezTransformComponentPatch_1_2 g_ezTransformComponentPatch_1_2;


EZ_STATICLINK_FILE(GameEngine, GameEngine_Components_Implementation_TransformComponent);

