#include <PacManPlugin/PacManPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Physics/CharacterControllerComponent.h>
#include <PacManPlugin/Components/GhostComponent.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(GhostComponent, 1 /* version */, ezComponentMode::Dynamic) // 'Dynamic' because we want to change the owner's transform
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Speed", m_fSpeed)->AddAttributes(new ezDefaultValueAttribute(2.0f), new ezClampValueAttribute(0.1f, 10.0f)),
  }
  EZ_END_PROPERTIES;

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

  ezHashedString hs;
  hs.Assign("Stats");
  m_pStateBlackboard = ezBlackboard::GetOrCreateGlobal(hs);

  // preload our disappear effect for when the player wins
  m_hDisappear = ezResourceManager::LoadResource<ezPrefabResource>("{ bad55bab-9701-484c-b3f2-90caeb206716 }");
  ezResourceManager::PreloadResource(m_hDisappear);
}

void GhostComponent::Update()
{
  // check the blackboard for whether the player just won
  {
    const PacManState state = static_cast<PacManState>(m_pStateBlackboard->GetEntryValue(ezTempHashedString("PacManState"), PacManState::Alive).Get<ezInt32>());

    if (state == PacManState::WonGame)
    {
      // create the 'disappear' effect
      ezPrefabResource::InstantiatePrefab(m_hDisappear, true, *GetWorld(), GetOwner()->GetGlobalTransform());

      // and delete yourself at the end of the frame
      GetWorld()->DeleteObjectDelayed(GetOwner()->GetHandle());
      return;
    }
  }

  bool bWall[4] = {false, false, false, false};

  if (ezPhysicsWorldModuleInterface* pPhysics = GetWorld()->GetOrCreateModule<ezPhysicsWorldModuleInterface>())
  {
    // do four raycasts into each direction, to detect which direction would be free to walk into
    // if the forwards direction is blocked, we want the ghost to turn

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

  // if the direction into which the ghost currently walks is occluded, randomly turn left or right and check again
  while (bWall[m_Direction])
  {
    m_Direction = static_cast<WalkDirection>((m_Direction + rng.IntMinMax(-1, 1)) % 4);
  }

  // now just change the rotation of the ghost to point into the current direction
  ezQuat rotation;
  rotation.SetFromAxisAndAngle(ezVec3::UnitZAxis(), ezAngle::Degree(m_Direction * 90));
  GetOwner()->SetGlobalRotation(rotation);

  // and communicate to the character controller component, that it should move forwards at a fixed speed
  ezMsgMoveCharacterController msg;
  msg.m_fMoveForwards = m_fSpeed;
  GetOwner()->SendMessage(msg);
}

void GhostComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  if (OWNTYPE::GetStaticRTTI()->GetTypeVersion() == 1)
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
