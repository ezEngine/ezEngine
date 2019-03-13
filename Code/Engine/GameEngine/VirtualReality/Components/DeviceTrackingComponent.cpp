#include <GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Profiling/Profiling.h>
#include <GameEngine/VirtualReality/Components/DeviceTrackingComponent.h>
#include <GameEngine/VirtualReality/Components/StageSpaceComponent.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezVRTransformSpace, 1)
EZ_BITFLAGS_CONSTANTS(ezVRTransformSpace::Local, ezVRTransformSpace::Global)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_COMPONENT_TYPE(ezDeviceTrackingComponent, 1, ezComponentMode::Dynamic)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_ACCESSOR_PROPERTY("DeviceType", ezVRDeviceType, GetDeviceType, SetDeviceType),
    EZ_ENUM_ACCESSOR_PROPERTY("TransformSpace", ezVRTransformSpace, GetTransformSpace, SetTransformSpace)
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

void ezDeviceTrackingComponent::SetDeviceType(ezEnum<ezVRDeviceType> type)
{
  m_deviceType = type;
}

ezEnum<ezVRDeviceType> ezDeviceTrackingComponent::GetDeviceType() const
{
  return m_deviceType;
}

void ezDeviceTrackingComponent::SetTransformSpace(ezEnum<ezVRTransformSpace> space)
{
  m_space = space;
}

ezEnum<ezVRTransformSpace> ezDeviceTrackingComponent::GetTransformSpace() const
{
  return m_space;
}

void ezDeviceTrackingComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  ezStreamWriter& s = stream.GetStream();

  s << m_deviceType;
  s << m_space;
}

void ezDeviceTrackingComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = stream.GetStream();

  s >> m_deviceType;
  s >> m_space;
}

void ezDeviceTrackingComponent::Update()
{
  if (ezVRInterface* pVRInterface = ezSingletonRegistry::GetSingletonInstance<ezVRInterface>())
  {
    ezVRDeviceID deviceID = pVRInterface->GetDeviceIDByType(m_deviceType);
    if (deviceID != -1)
    {
      const ezVRDeviceState& state = pVRInterface->GetDeviceState(deviceID);
      if (state.m_bPoseIsValid)
      {
        if (m_space == ezVRTransformSpace::Local)
        {
          GetOwner()->SetLocalPosition(state.m_vPosition);
          GetOwner()->SetLocalRotation(state.m_qRotation);
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
          ezTransform local(state.m_vPosition, state.m_qRotation);
          GetOwner()->SetGlobalTransform(local * add);

        }
      }
    }
  }
}

EZ_STATICLINK_FILE(GameEngine, GameEngine_VirtualReality_Components_DeviceTrackingComponent);

