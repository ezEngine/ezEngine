#include <GameEngine/GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Profiling/Profiling.h>
#include <GameEngine/XR/DeviceTrackingComponent.h>
#include <GameEngine/XR/StageSpaceComponent.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezXRPoseLocation, 1)
EZ_BITFLAGS_CONSTANTS(ezXRPoseLocation::Grip, ezXRPoseLocation::Aim)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_COMPONENT_TYPE(ezDeviceTrackingComponent, 3, ezComponentMode::Dynamic)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_ACCESSOR_PROPERTY("DeviceType", ezXRDeviceType, GetDeviceType, SetDeviceType),
    EZ_ENUM_ACCESSOR_PROPERTY("PoseLocation", ezXRPoseLocation, GetPoseLocation, SetPoseLocation),
    EZ_ENUM_ACCESSOR_PROPERTY("TransformSpace", ezXRTransformSpace, GetTransformSpace, SetTransformSpace),
    EZ_MEMBER_PROPERTY("Rotation", m_bRotation)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("Scale", m_bScale)->AddAttributes(new ezDefaultValueAttribute(true)),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("XR"),
    new ezInDevelopmentAttribute(ezInDevelopmentAttribute::Phase::Alpha),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezDeviceTrackingComponent::ezDeviceTrackingComponent() = default;
ezDeviceTrackingComponent::~ezDeviceTrackingComponent() = default;

void ezDeviceTrackingComponent::SetDeviceType(ezEnum<ezXRDeviceType> type)
{
  m_DeviceType = type;
}

ezEnum<ezXRDeviceType> ezDeviceTrackingComponent::GetDeviceType() const
{
  return m_DeviceType;
}

void ezDeviceTrackingComponent::SetPoseLocation(ezEnum<ezXRPoseLocation> poseLocation)
{
  m_PoseLocation = poseLocation;
}

ezEnum<ezXRPoseLocation> ezDeviceTrackingComponent::GetPoseLocation() const
{
  return m_PoseLocation;
}

void ezDeviceTrackingComponent::SetTransformSpace(ezEnum<ezXRTransformSpace> space)
{
  m_Space = space;
}

ezEnum<ezXRTransformSpace> ezDeviceTrackingComponent::GetTransformSpace() const
{
  return m_Space;
}

void ezDeviceTrackingComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  ezStreamWriter& s = stream.GetStream();

  s << m_DeviceType;
  s << m_PoseLocation;
  s << m_Space;
  s << m_bRotation;
  s << m_bScale;
}

void ezDeviceTrackingComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = stream.GetStream();

  s >> m_DeviceType;
  if (uiVersion >= 2)
  {
    s >> m_PoseLocation;
  }
  s >> m_Space;
  if (uiVersion >= 3)
  {
    s >> m_bRotation;
    s >> m_bScale;
  }
}

void ezDeviceTrackingComponent::Update()
{
  if (!IsActiveAndSimulating())
    return;

  if (ezXRInterface* pXRInterface = ezSingletonRegistry::GetSingletonInstance<ezXRInterface>())
  {
    if (!pXRInterface->IsInitialized())
      return;

    ezXRDeviceID deviceID = pXRInterface->GetXRInput().GetDeviceIDByType(m_DeviceType);
    if (deviceID != -1)
    {
      const ezXRDeviceState& state = pXRInterface->GetXRInput().GetDeviceState(deviceID);
      ezVec3 vPosition;
      ezQuat qRotation;
      if (m_PoseLocation == ezXRPoseLocation::Grip && state.m_bGripPoseIsValid)
      {
        vPosition = state.m_vGripPosition;
        qRotation = state.m_qGripRotation;
      }
      else if (m_PoseLocation == ezXRPoseLocation::Aim && state.m_bAimPoseIsValid)
      {
        vPosition = state.m_vAimPosition;
        qRotation = state.m_qAimRotation;
      }
      else
      {
        return;
      }
      if (m_Space == ezXRTransformSpace::Local)
      {
        GetOwner()->SetLocalPosition(vPosition);
        if (m_bRotation)
          GetOwner()->SetLocalRotation(qRotation);
      }
      else
      {
        ezTransform add;
        add.SetIdentity();
        if (const ezStageSpaceComponentManager* pStageMan = GetWorld()->GetComponentManager<ezStageSpaceComponentManager>())
        {
          if (const ezStageSpaceComponent* pStage = pStageMan->GetSingletonComponent())
          {
            add = pStage->GetOwner()->GetGlobalTransform();
          }
        }

        const ezTransform global(add * ezTransform(vPosition, qRotation));
        ezTransform local;
        if (GetOwner()->GetParent() != nullptr)
        {
          local.SetLocalTransform(GetOwner()->GetParent()->GetGlobalTransform(), global);
        }
        else
        {
          local = global;
        }
        GetOwner()->SetLocalPosition(local.m_vPosition);
        if (m_bRotation)
          GetOwner()->SetLocalRotation(local.m_qRotation);
        if (m_bScale)
          GetOwner()->SetLocalScaling(local.m_vScale);
      }
    }
  }
}

EZ_STATICLINK_FILE(GameEngine, GameEngine_XR_Implementation_DeviceTrackingComponent);
