#include <PacManPlugin/PacManPluginPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/Utils/Blackboard.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <GameEngine/Physics/CharacterControllerComponent.h>
#include <PacManPlugin/Components/GhostComponent.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(GhostComponent, 1 /* version */, ezComponentMode::Dynamic)
{
  //EZ_BEGIN_PROPERTIES
  //{
  //  EZ_MEMBER_PROPERTY("Amplitude", m_fAmplitude)->AddAttributes(new ezDefaultValueAttribute(1), new ezClampValueAttribute(0, 10)),
  //  EZ_MEMBER_PROPERTY("Speed", m_Speed)->AddAttributes(new ezDefaultValueAttribute(ezAngle::Degree(90))),
  //}
  //EZ_END_PROPERTIES;

  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("PacMan"), // Component menu group
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

GhostComponent::GhostComponent() = default;
GhostComponent::~GhostComponent() = default;

void GhostComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  m_hDisappear = ezResourceManager::LoadResource<ezPrefabResource>("{ bad55bab-9701-484c-b3f2-90caeb206716 }");
  ezResourceManager::PreloadResource(m_hDisappear);
}

void GhostComponent::Update()
{
  if (auto pBlackboard = ezBlackboard::FindGlobal(ezTempHashedString("Stats")))
  {
    const ezInt32 iState = pBlackboard->GetEntryValue(ezTempHashedString("PacManState"), 1).Get<ezInt32>();

    if (iState == 2)
    {
      ezResourceLock<ezPrefabResource> pPrefab(m_hDisappear, ezResourceAcquireMode::BlockTillLoaded);
      if (pPrefab.GetAcquireResult() == ezResourceAcquireResult::Final)
      {
        pPrefab->InstantiatePrefab(*GetWorld(), GetOwner()->GetGlobalTransform(), {});
      }

      // player won! -> delete yourself
      GetWorld()->DeleteObjectDelayed(GetOwner()->GetHandle());
      return;
    }
  }

  bool bWall[4] = {false, false, false, false};

  if (ezPhysicsWorldModuleInterface* pPhysics = GetWorld()->GetOrCreateModule<ezPhysicsWorldModuleInterface>())
  {
    ezPhysicsCastResult res;
    ezPhysicsQueryParameters params;
    params.m_ShapeTypes = ezPhysicsShapeType::Static;

    ezVec3 pos = GetOwner()->GetGlobalPosition();
    pos.z += 0.5f;

    bWall[0] = pPhysics->Raycast(res, pos, ezVec3(1, 0, 0), 0.55f, params);
    bWall[1] = pPhysics->Raycast(res, pos, ezVec3(0, 1, 0), 0.55f, params);
    bWall[2] = pPhysics->Raycast(res, pos, ezVec3(-1, 0, 0), 0.55f, params);
    bWall[3] = pPhysics->Raycast(res, pos, ezVec3(0, -1, 0), 0.55f, params);
  }

  ezRandom& rng = GetWorld()->GetRandomNumberGenerator();

  while (bWall[m_uiDirection])
  {
    m_uiDirection = (m_uiDirection + rng.IntMinMax(-1, 1)) % 4;
  }

  ezQuat rotation;
  rotation.SetFromAxisAndAngle(ezVec3::UnitZAxis(), ezAngle::Degree(m_uiDirection * 90));
  GetOwner()->SetGlobalRotation(rotation);

  ezMsgMoveCharacterController msg;
  msg.m_fMoveForwards = 2.0f;
  GetOwner()->SendMessage(msg);
}

void GhostComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  if (GhostComponent::GetStaticRTTI()->GetTypeVersion() == 1)
  {
    ezReflectionSerializer::WriteObjectToBinary(s, GetDynamicRTTI(), this);
  }
  else
  {
    // do custom serialization
  }
}

void GhostComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  if (uiVersion == 1)
  {
    ezReflectionSerializer::ReadObjectPropertiesFromBinary(s, *GetDynamicRTTI(), this);
  }
  else
  {
    // do custom serialization
  }
}
