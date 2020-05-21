#include <GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Profiling/Profiling.h>
#include <GameEngine/XR/DeviceTrackingComponent.h>
#include <GameEngine/XR/StageSpaceComponent.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezXRTransformSpace, 1)
EZ_BITFLAGS_CONSTANTS(ezXRTransformSpace::Local, ezXRTransformSpace::Global)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezXRPoseLocation, 1)
EZ_BITFLAGS_CONSTANTS(ezXRPoseLocation::Grip, ezXRPoseLocation::Aim)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_COMPONENT_TYPE(ezDeviceTrackingComponent, 2, ezComponentMode::Dynamic)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_ACCESSOR_PROPERTY("DeviceType", ezXRDeviceType, GetDeviceType, SetDeviceType),
    EZ_ENUM_ACCESSOR_PROPERTY("PoseLocation", ezXRPoseLocation, GetPoseLocation, SetPoseLocation),
    EZ_ENUM_ACCESSOR_PROPERTY("TransformSpace", ezXRTransformSpace, GetTransformSpace, SetTransformSpace)
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Virtual Reality"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezDeviceTrackingComponent::ezDeviceTrackingComponent() = default;
ezDeviceTrackingComponent::~ezDeviceTrackingComponent() = default;

void ezDeviceTrackingComponent::SetDeviceType(ezEnum<ezXRDeviceType> type)
{
  m_deviceType = type;
}

ezEnum<ezXRDeviceType> ezDeviceTrackingComponent::GetDeviceType() const
{
  return m_deviceType;
}

void ezDeviceTrackingComponent::SetPoseLocation(ezEnum<ezXRPoseLocation> poseLocation)
{
  m_poseLocation = poseLocation;
}

ezEnum<ezXRPoseLocation> ezDeviceTrackingComponent::GetPoseLocation() const
{
  return m_poseLocation;
}

void ezDeviceTrackingComponent::SetTransformSpace(ezEnum<ezXRTransformSpace> space)
{
  m_space = space;
}

ezEnum<ezXRTransformSpace> ezDeviceTrackingComponent::GetTransformSpace() const
{
  return m_space;
}

void ezDeviceTrackingComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  ezStreamWriter& s = stream.GetStream();

  s << m_deviceType;
  s << m_poseLocation;
  s << m_space;
}

void ezDeviceTrackingComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = stream.GetStream();

  s >> m_deviceType;
  if (uiVersion >= 2)
  {
    s >> m_poseLocation;
  }
  s >> m_space;
}

void ezDeviceTrackingComponent::Update()
{
  if (ezXRInterface* pXRInterface = ezSingletonRegistry::GetSingletonInstance<ezXRInterface>())
  {
    if (!pXRInterface->IsInitialized())
      return;

    ezXRDeviceID deviceID = pXRInterface->GetXRInput().GetDeviceIDByType(m_deviceType);
    if (deviceID != -1)
    {
      const ezXRDeviceState& state = pXRInterface->GetXRInput().GetDeviceState(deviceID);
      ezVec3 vPosition;
      ezQuat qRotation;
      if (m_poseLocation == ezXRPoseLocation::Grip && state.m_bGripPoseIsValid)
      {
        vPosition = state.m_vGripPosition;
        qRotation = state.m_qGripRotation;
      }
      else if (m_poseLocation == ezXRPoseLocation::Aim && state.m_bAimPoseIsValid)
      {
        vPosition = state.m_vAimPosition;
        qRotation = state.m_qAimRotation;
      }
      else
      {
        return;
      }
      if (m_space == ezXRTransformSpace::Local)
      {
        GetOwner()->SetLocalPosition(vPosition);
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
        ezTransform local(vPosition, qRotation);
        GetOwner()->SetGlobalTransform(add * local);
      }
    }
  }
}

EZ_STATICLINK_FILE(GameEngine, GameEngine_XR_Implementation_DeviceTrackingComponent);

