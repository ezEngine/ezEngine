#include <PhysXPlugin/PhysXPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <PhysXPlugin/Components/PxCharacterShapeComponent.h>
#include <PhysXPlugin/Shapes/PxShapeComponent.h>
#include <PhysXPlugin/Utilities/PxConversionUtils.h>
#include <PhysXPlugin/WorldModule/Implementation/PhysX.h>
#include <PhysXPlugin/WorldModule/PhysXWorldModule.h>

using namespace physx;

ezBitflags<ezPxCharacterShapeCollisionFlags> ezPxCharacterShapeCollisionFlags::FromPxFlags(ezUInt32 pxFlags)
{
  ezBitflags<ezPxCharacterShapeCollisionFlags> result = ezPxCharacterShapeCollisionFlags::None;

  if ((pxFlags & PxControllerCollisionFlag::eCOLLISION_SIDES) != 0)
  {
    result.Add(ezPxCharacterShapeCollisionFlags::Sides);
  }

  if ((pxFlags & PxControllerCollisionFlag::eCOLLISION_UP) != 0)
  {
    result.Add(ezPxCharacterShapeCollisionFlags::Above);
  }

  if ((pxFlags & PxControllerCollisionFlag::eCOLLISION_DOWN) != 0)
  {
    result.Add(ezPxCharacterShapeCollisionFlags::Below);
  }

  return result;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_BITFLAGS(ezPxCharacterShapeCollisionFlags, 1)
  EZ_ENUM_CONSTANT(ezPxCharacterShapeCollisionFlags::None),
  EZ_ENUM_CONSTANT(ezPxCharacterShapeCollisionFlags::Sides),
  EZ_ENUM_CONSTANT(ezPxCharacterShapeCollisionFlags::Above),
  EZ_ENUM_CONSTANT(ezPxCharacterShapeCollisionFlags::Below),
EZ_END_STATIC_REFLECTED_BITFLAGS;

EZ_BEGIN_ABSTRACT_COMPONENT_TYPE(ezPxCharacterShapeComponent, 1)
{
  EZ_BEGIN_PROPERTIES
  {
      EZ_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new ezDynamicEnumAttribute("PhysicsCollisionLayer")),
      EZ_MEMBER_PROPERTY("Mass", m_fMass)->AddAttributes(new ezDefaultValueAttribute(50.0f), new ezClampValueAttribute(0.1f, 10000.0f)),
      EZ_MEMBER_PROPERTY("MaxStepHeight", m_fMaxStepHeight)->AddAttributes(new ezDefaultValueAttribute(0.3f), new ezClampValueAttribute(0.0f, 5.0f)),
      EZ_MEMBER_PROPERTY("MaxSlopeAngle", m_MaxClimbingSlope)->AddAttributes(new ezDefaultValueAttribute(ezAngle::Degree(40.0f)), new ezClampValueAttribute(ezAngle::Degree(0.0f), ezAngle::Degree(80.0f))),
      EZ_MEMBER_PROPERTY("ForceSlopeSliding", m_bForceSlopeSliding)->AddAttributes(new ezDefaultValueAttribute(true)),
      EZ_MEMBER_PROPERTY("ConstrainedClimbMode", m_bConstrainedClimbingMode),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(GetCollisionFlags),
    EZ_SCRIPT_FUNCTION_PROPERTY(IsTouchingGround),
    EZ_SCRIPT_FUNCTION_PROPERTY(GetShapeId),
    EZ_SCRIPT_FUNCTION_PROPERTY(GetCurrentHeightValue),
  }
  EZ_END_FUNCTIONS;
  EZ_BEGIN_ATTRIBUTES
  {
      new ezCategoryAttribute("Physics/Special"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_ABSTRACT_COMPONENT_TYPE
// clang-format on

ezPxCharacterShapeComponent::ezPxCharacterShapeComponent() = default;
ezPxCharacterShapeComponent::~ezPxCharacterShapeComponent() = default;

void ezPxCharacterShapeComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_uiCollisionLayer;
  s << m_fMass;
  s << m_fMaxStepHeight;
  s << m_MaxClimbingSlope;
  s << m_bForceSlopeSliding;
  s << m_bConstrainedClimbingMode;
}

void ezPxCharacterShapeComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  s >> m_uiCollisionLayer;
  s >> m_fMass;
  s >> m_fMaxStepHeight;
  s >> m_MaxClimbingSlope;
  s >> m_bForceSlopeSliding;
  s >> m_bConstrainedClimbingMode;
}

void ezPxCharacterShapeComponent::OnActivated()
{
  SUPER::OnActivated();

  GetOwner()->UpdateLocalBounds();
}

void ezPxCharacterShapeComponent::OnDeactivated()
{
  if (m_pController != nullptr)
  {
    EZ_PX_WRITE_LOCK(*(m_pController->getScene()));

    m_pController->release();
    m_pController = nullptr;
  }

  if (ezPhysXWorldModule* pModule = GetWorld()->GetModule<ezPhysXWorldModule>())
  {
    pModule->DeleteShapeId(m_uiShapeId);
    pModule->DeallocateUserData(m_uiUserDataIndex);

    m_uiShapeId = ezInvalidIndex;
    m_uiUserDataIndex = ezInvalidIndex;
  }

  SUPER::OnDeactivated();
}

void ezPxCharacterShapeComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  ezPhysXWorldModule* pModule = GetWorld()->GetOrCreateModule<ezPhysXWorldModule>();
  m_uiShapeId = pModule->CreateShapeId();

  ezPxUserData* pUserData = nullptr;
  m_uiUserDataIndex = pModule->AllocateUserData(pUserData);
  pUserData->Init(this);
}

ezPxUserData* ezPxCharacterShapeComponent::GetUserData()
{
  ezPhysXWorldModule* pModule = GetWorld()->GetOrCreateModule<ezPhysXWorldModule>();

  return &pModule->GetUserData(m_uiUserDataIndex);
}

ezBitflags<ezPxCharacterShapeCollisionFlags> ezPxCharacterShapeComponent::GetCollisionFlags() const
{
  if (m_pController != nullptr)
  {
    EZ_PX_WRITE_LOCK(*(m_pController->getScene()));

    PxControllerState state;
    m_pController->getState(state);

    return ezPxCharacterShapeCollisionFlags::FromPxFlags(state.collisionFlags);
  }

  return ezPxCharacterShapeCollisionFlags::None;
}

ezGameObjectHandle ezPxCharacterShapeComponent::GetStandingOnShape() const
{
  if (m_pController != nullptr)
  {
    EZ_PX_WRITE_LOCK(*(m_pController->getScene()));

    PxControllerState state;
    m_pController->getState(state);

    if (state.touchedShape != nullptr)
    {
      if (ezComponent* pComponent = ezPxUserData::GetShapeComponent(state.touchedShape->userData))
      {
        return pComponent->GetOwner()->GetHandle();
      }
    }
  }

  return ezGameObjectHandle();
}

ezGameObjectHandle ezPxCharacterShapeComponent::GetStandingOnActor() const
{
  if (m_pController != nullptr)
  {
    PxControllerState state;
    m_pController->getState(state);

    if (state.touchedActor != nullptr)
    {
      if (ezComponent* pComponent = ezPxUserData::GetComponent(state.touchedActor->userData))
      {
        return pComponent->GetOwner()->GetHandle();
      }
    }
  }

  return ezGameObjectHandle();
}

void ezPxCharacterShapeComponent::TeleportShape(const ezVec3& vGlobalFootPos)
{
  if (m_pController != nullptr)
  {
    EZ_PX_WRITE_LOCK(*(m_pController->getScene()));

    m_pController->setFootPosition(ezPxConversionUtils::ToExVec3(vGlobalFootPos));

    GetOwner()->SetGlobalPosition(vGlobalFootPos);
  }
}

bool ezPxCharacterShapeComponent::CanResize(float fNewHeightValue)
{
  if (fNewHeightValue < m_fCurrentHeightValue)
    return true;

  return !TestShapeOverlap(GetOwner()->GetGlobalPosition(), fNewHeightValue);
}

bool ezPxCharacterShapeComponent::TryResize(float fNewHeightValue)
{
  if (!m_pController)
    return false;

  EZ_PX_WRITE_LOCK(*(m_pController->getScene()));

  if (!CanResize(fNewHeightValue))
    return false;

  m_fCurrentHeightValue = fNewHeightValue;
  m_pController->resize(fNewHeightValue);

  return true;
}
