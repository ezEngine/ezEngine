#include <PhysXPlugin/PCH.h>
#include <PhysXPlugin/Components/PxCharacterControllerComponent.h>
#include <PhysXPlugin/PhysXWorldModule.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <GameUtils/Components/InputComponent.h>

EZ_BEGIN_COMPONENT_TYPE(ezPxCharacterControllerComponent, 1);
  //EZ_BEGIN_PROPERTIES
  //EZ_END_PROPERTIES
  EZ_BEGIN_MESSAGEHANDLERS
    EZ_MESSAGE_HANDLER(ezInputComponentMessage, InputComponentMessageHandler),
  EZ_END_MESSAGEHANDLERS
EZ_END_COMPONENT_TYPE();

ezPxCharacterControllerComponent::ezPxCharacterControllerComponent()
{
  m_pController = nullptr;
  m_vRelativeMoveDirection.SetZero();
}


void ezPxCharacterControllerComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

}


void ezPxCharacterControllerComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());


}

void ezPxCharacterControllerComponent::Update()
{
  if (m_pController == nullptr)
    return;

  const float tDiff = (float)GetWorld()->GetClock().GetTimeDiff().GetSeconds();

  m_vRelativeMoveDirection *= 2.0f;

  m_vRelativeMoveDirection = GetOwner()->GetGlobalRotation() * m_vRelativeMoveDirection;

  m_vRelativeMoveDirection += ezVec3(0, 0, -1.0f) * tDiff;

  
  //m_vRelativeMoveDirection *= tDiff;

  PxVec3 mov;
  mov.x = m_vRelativeMoveDirection.x;
  mov.y = m_vRelativeMoveDirection.y;
  mov.z = m_vRelativeMoveDirection.z;

  PxControllerFilters filter;
  filter.mCCTFilterCallback = nullptr;
  filter.mFilterCallback = nullptr;
  //filter.mFilterFlags = PxQueryFlag::

  m_pController->move(mov, 0.01f, tDiff, filter);

  m_vRelativeMoveDirection.SetZero();

  auto pos = m_pController->getPosition();

  GetOwner()->SetGlobalPosition(ezVec3((float)pos.x, (float)pos.y, (float)pos.z));

  if (m_RotateZ.GetRadian() != 0.0f)
  {
    ezQuat qRotZ;
    qRotZ.SetFromAxisAndAngle(ezVec3(0, 0, 1), m_RotateZ);

    GetOwner()->SetGlobalRotation(qRotZ * GetOwner()->GetGlobalRotation());

    m_RotateZ.SetRadian(0.0);
  }
}

ezComponent::Initialization ezPxCharacterControllerComponent::Initialize()
{
  ezPhysXSceneModule* pModule = static_cast<ezPhysXSceneModule*>(GetManager()->GetUserData());

  const auto pos = GetOwner()->GetGlobalPosition();
  const auto rot = GetOwner()->GetGlobalRotation();

  PxTransform t = PxTransform::createIdentity();
  t.p = PxVec3(pos.x, pos.y, pos.z);
  t.q = PxQuat(rot.v.x, rot.v.y, rot.v.z, rot.w);

  PxCapsuleControllerDesc cd;
  cd.climbingMode = PxCapsuleClimbingMode::eEASY;
  cd.height = 1.0f;
  cd.radius = 0.25f;
  cd.nonWalkableMode = PxControllerNonWalkableMode::ePREVENT_CLIMBING_AND_FORCE_SLIDING;
  cd.position.set(pos.x, pos.y, pos.z);
  cd.slopeLimit = ezMath::Cos(ezAngle::Degree(45.0f));
  cd.stepOffset = 0.4f;
  cd.upDirection = PxVec3(0, 0, 1);
  cd.userData = this;
  cd.material = ezPhysX::GetSingleton()->GetDefaultMaterial();

  EZ_ASSERT_DEV(cd.isValid(), "Character Controller configuration is invalid");

  m_pController = static_cast<PxCapsuleController*>(pModule->GetCharacterManager()->createController(cd));

  EZ_ASSERT_DEV(m_pController != nullptr, "Failed to create character controller");

  m_vRelativeMoveDirection.SetZero();

  return ezComponent::Initialization::Done;
}

void ezPxCharacterControllerComponent::Deinitialize()
{
  if (m_pController)
  {
    //m_pController->release();
    m_pController = nullptr;
  }
}

void ezPxCharacterControllerComponent::InputComponentMessageHandler(ezInputComponentMessage& msg)
{
  float f = msg.m_fValue;

  if (ezStringUtils::IsEqual(msg.m_szAction, "MoveForwards"))
  {
    m_vRelativeMoveDirection += ezVec3(f, 0, 0);
    return;
  }

  if (ezStringUtils::IsEqual(msg.m_szAction, "MoveBackwards"))
  {
    m_vRelativeMoveDirection += ezVec3(-f, 0, 0);
    return;
  }

  if (ezStringUtils::IsEqual(msg.m_szAction, "StrafeLeft"))
  {
    m_vRelativeMoveDirection += ezVec3(0, -f, 0);
    return;
  }

  if (ezStringUtils::IsEqual(msg.m_szAction, "StrafeRight"))
  {
    m_vRelativeMoveDirection += ezVec3(0, f, 0);
    return;
  }

  if (ezStringUtils::IsEqual(msg.m_szAction, "RotateLeft"))
  {
    m_RotateZ -= ezAngle::Degree(90.0f * f);
    return;
  }

  if (ezStringUtils::IsEqual(msg.m_szAction, "RotateRight"))
  {
    m_RotateZ += ezAngle::Degree(90.0f * f);
    return;
  }
}


