#include <GameEngine/GameEnginePCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/AI/SensorComponent.h>

// clang-format off
EZ_IMPLEMENT_MESSAGE_TYPE(ezSensorVisibleObjectsChanged);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSensorVisibleObjectsChanged, 1, ezRTTIDefaultAllocator<ezSensorVisibleObjectsChanged>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ARRAY_MEMBER_PROPERTY_READ_ONLY("VisibleObjects", m_VisibleObjects),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_ABSTRACT_COMPONENT_TYPE(ezSensorComponent, 1)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("SpatialCategory", GetSpatialCategory, SetSpatialCategory)->AddAttributes(new ezDynamicStringEnumAttribute("SpatialDataCategoryEnum")),
    EZ_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new ezDynamicEnumAttribute("PhysicsCollisionLayer")),
    EZ_ENUM_MEMBER_PROPERTY("UpdateRate", ezUpdateRate, m_UpdateRate),
  }
  EZ_END_PROPERTIES;
}
EZ_END_ABSTRACT_COMPONENT_TYPE
// clang-format on

ezSensorComponent::ezSensorComponent() = default;
ezSensorComponent::~ezSensorComponent() = default;

void ezSensorComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_sSpatialCategory;
  s << m_uiCollisionLayer;
}

void ezSensorComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  s >> m_sSpatialCategory;
  s >> m_uiCollisionLayer;
}

void ezSensorComponent::OnActivated()
{
  SUPER::OnActivated();

  UpdateSpatialCategory();
}

void ezSensorComponent::SetSpatialCategory(const char* szCategory)
{
  m_sSpatialCategory.Assign(szCategory);

  UpdateSpatialCategory();
}

const char* ezSensorComponent::GetSpatialCategory() const
{
  return m_sSpatialCategory;
}

void ezSensorComponent::UpdateSpatialCategory()
{
  if (!m_sSpatialCategory.IsEmpty())
  {
    m_SpatialCategory = ezSpatialData::RegisterCategory(m_sSpatialCategory, ezSpatialData::Flags::None);
  }
  else
  {
    m_SpatialCategory = ezInvalidSpatialDataCategory;
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezSensorSphereComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Radius", m_fRadius)->AddAttributes(new ezDefaultValueAttribute(10.0f), new ezClampValueAttribute(0.0f, ezVariant()), new ezSuffixAttribute(" m")),
  }
  EZ_END_PROPERTIES;

  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("AI"),
    new ezSphereManipulatorAttribute("Radius"),
    new ezSphereVisualizerAttribute("Radius", ezColor::DarkOrange),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezSensorSphereComponent::ezSensorSphereComponent() = default;
ezSensorSphereComponent::~ezSensorSphereComponent() = default;

void ezSensorSphereComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();
}

void ezSensorSphereComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();
}

void ezSensorSphereComponent::OnActivated()
{
  SUPER::OnActivated();

  auto pModule = GetWorld()->GetOrCreateModule<ezSensorWorldModule>();
  pModule->AddComponentToSchedule(this, m_UpdateRate);
}

void ezSensorSphereComponent::OnDeactivated()
{
  SUPER::OnDeactivated();

  auto pModule = GetWorld()->GetOrCreateModule<ezSensorWorldModule>();
  pModule->RemoveComponentToSchedule(this);
}

void ezSensorSphereComponent::GetObjectsInSensorVolume(ezDynamicArray<ezGameObject*>& out_Objects) const
{
  ezBoundingSphere sphere = ezBoundingSphere(GetOwner()->GetGlobalPosition(), m_fRadius);

  ezSpatialSystem::QueryParams params;
  params.m_uiCategoryBitmask = m_SpatialCategory.GetBitmask();

  GetWorld()->GetSpatialSystem()->FindObjectsInSphere(sphere, params, out_Objects);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_IMPLEMENT_WORLD_MODULE(ezSensorWorldModule);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSensorWorldModule, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezSensorWorldModule::ezSensorWorldModule(ezWorld* pWorld)
  : ezWorldModule(pWorld)
{
}

void ezSensorWorldModule::Initialize()
{
  SUPER::Initialize();

  {
    auto updateDesc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezSensorWorldModule::UpdateSensors, this);
    updateDesc.m_Phase = ezWorldModule::UpdateFunctionDesc::Phase::Async;
    updateDesc.m_bOnlyUpdateWhenSimulating = true;

    RegisterUpdateFunction(updateDesc);
  }

  m_pPhysicsWorldModule = GetWorld()->GetOrCreateModule<ezPhysicsWorldModuleInterface>();
}

void ezSensorWorldModule::AddComponentToSchedule(ezSensorComponent* pComponent, ezUpdateRate::Enum updateRate)
{
  m_Scheduler.AddOrUpdateWork(pComponent->GetHandle(), ezUpdateRate::GetInterval(updateRate));
}

void ezSensorWorldModule::RemoveComponentToSchedule(ezSensorComponent* pComponent)
{
  m_Scheduler.RemoveWork(pComponent->GetHandle());
}

void ezSensorWorldModule::UpdateSensors(const ezWorldModule::UpdateContext& context)
{
  if (m_pPhysicsWorldModule == nullptr)
    return;

  const ezTime deltaTime = GetWorld()->GetClock().GetTimeDiff();
  m_Scheduler.Update(deltaTime, [this](const ezComponentHandle& hComponent, ezTime deltaTime)
    {
      ezSensorComponent* pSensorComponent = nullptr;
      EZ_VERIFY(GetWorld()->TryGetComponent(hComponent, pSensorComponent), "Invalid component handle");

      pSensorComponent->GetObjectsInSensorVolume(m_ObjectsInSensorVolume);
      const ezGameObject* pSensorOwner = pSensorComponent->GetOwner();

      const ezVec3 rayStart = pSensorOwner->GetGlobalPosition();
      m_VisibleObjects.Clear();
      for (auto pObject : m_ObjectsInSensorVolume)
      {
        const ezVec3 rayEnd = pObject->GetGlobalPosition();
        ezVec3 rayDir = rayEnd - rayStart;
        const float fDistance = rayDir.GetLengthAndNormalize();

        ezPhysicsCastResult hitResult;
        ezPhysicsQueryParameters params(pSensorComponent->m_uiCollisionLayer);
        if (m_pPhysicsWorldModule->Raycast(hitResult, rayStart, rayDir, fDistance, params))
        {
          // hit something in between -> not visible
          continue;
        }

        m_VisibleObjects.PushBack(pObject->GetHandle());
      }

      m_VisibleObjects.Sort();
      if (m_VisibleObjects != pSensorComponent->m_LastVisibleObjects)
      {
        m_VisibleObjects.Swap(pSensorComponent->m_LastVisibleObjects);

        ezSensorVisibleObjectsChanged msg;
        msg.m_VisibleObjects = pSensorComponent->m_LastVisibleObjects;

        pSensorOwner->PostEventMessage(msg, pSensorComponent, ezTime::Zero(), ezObjectMsgQueueType::PostAsync);
      }
    });
}
