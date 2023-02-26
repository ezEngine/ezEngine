#include <PacManPlugin/PacManPluginPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/Interfaces/SoundInterface.h>
#include <Core/Prefabs/PrefabResource.h>
#include <Core/Utils/Blackboard.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Physics/CharacterControllerComponent.h>
#include <PacManPlugin/Components/PacManComponent.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(PacManComponent, 1 /* version */, ezComponentMode::Dynamic)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("PacMan"), // Component menu group
  }
  EZ_END_ATTRIBUTES;

  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgInputActionTriggered, OnMsgInputActionTriggered),
    EZ_MESSAGE_HANDLER(ezMsgTriggerTriggered, OnMsgTriggerTriggered),
  }
  EZ_END_MESSAGEHANDLERS;
}
EZ_END_COMPONENT_TYPE
// clang-format on

PacManComponent::PacManComponent() = default;
PacManComponent::~PacManComponent() = default;

void PacManComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  // preload prefabs needed later
  {
    m_hCollectCoint = ezResourceManager::LoadResource<ezPrefabResource>("{ 9b006872-70ba-4086-8ce5-244304032851 }");
    ezResourceManager::PreloadResource(m_hCollectCoint);

    m_hLoseGame = ezResourceManager::LoadResource<ezPrefabResource>("{ 02314d71-f49e-45f3-89e2-ce4b7b1cba09 }");
    ezResourceManager::PreloadResource(m_hLoseGame);
  }

  ezHashedString hs;

  hs.Assign("Stats");
  auto pBlackboard = ezBlackboard::GetOrCreateGlobal(hs);

  hs.Assign("PacManState");
  pBlackboard->RegisterEntry(hs, 1);
}

void PacManComponent::Update()
{
  if (auto pBlackboard = ezBlackboard::FindGlobal(ezTempHashedString("Stats")))
  {
    const ezInt32 iState = pBlackboard->GetEntryValue(ezTempHashedString("PacManState"), 1).Get<ezInt32>();

    if (iState == 0)
    {
      ezResourceLock<ezPrefabResource> pPrefab(m_hLoseGame, ezResourceAcquireMode::BlockTillLoaded);
      if (pPrefab.GetAcquireResult() == ezResourceAcquireResult::Final)
      {
        pPrefab->InstantiatePrefab(*GetWorld(), GetOwner()->GetGlobalTransform(), {});
      }

      // lost -> delete PacMan !
      GetWorld()->DeleteObjectDelayed(GetOwner()->GetHandle());
      return;
    }

    if (iState == 2)
    {
      pBlackboard->SetEntryValue(ezTempHashedString("PacManState"), 3).AssertSuccess(); // don't play effect twice

      ezSoundInterface::PlaySound("{ a10b9065-0b4d-4eff-a9ac-2f712dc28c1c }", GetOwner()->GetGlobalTransform()).IgnoreResult();
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
    const float dist = 1.0f;
    const float off = 0.35f;
    const ezVec3 offx(off, 0.0f, 0.0f);
    const ezVec3 offy(0.0f, off, 0.0f);

    bWall[0] |= pPhysics->Raycast(res, pos - offy, ezVec3(1, 0, 0), dist, params);
    bWall[0] |= pPhysics->Raycast(res, pos + offy, ezVec3(1, 0, 0), dist, params);

    bWall[1] |= pPhysics->Raycast(res, pos - offx, ezVec3(0, 1, 0), dist, params);
    bWall[1] |= pPhysics->Raycast(res, pos + offx, ezVec3(0, 1, 0), dist, params);

    bWall[2] |= pPhysics->Raycast(res, pos - offy, ezVec3(-1, 0, 0), dist, params);
    bWall[2] |= pPhysics->Raycast(res, pos + offy, ezVec3(-1, 0, 0), dist, params);

    bWall[3] |= pPhysics->Raycast(res, pos - offx, ezVec3(0, -1, 0), dist, params);
    bWall[3] |= pPhysics->Raycast(res, pos + offx, ezVec3(0, -1, 0), dist, params);
  }

  if (!bWall[m_iTargetDirection])
  {
    m_iDirection = m_iTargetDirection;
  }

  ezQuat rotation;
  rotation.SetFromAxisAndAngle(ezVec3::UnitZAxis(), ezAngle::Degree(m_iDirection * 90));
  GetOwner()->SetGlobalRotation(rotation);

  ezMsgMoveCharacterController msg;
  msg.m_fMoveForwards = 2.0f;
  GetOwner()->SendMessage(msg);
}

void PacManComponent::OnMsgInputActionTriggered(ezMsgInputActionTriggered& msg)
{
  if (msg.m_TriggerState != ezTriggerState::Continuing)
    return;

  if (msg.m_sInputAction.GetString() == "Up")
    m_iTargetDirection = 0;

  if (msg.m_sInputAction.GetString() == "Down")
    m_iTargetDirection = 2;

  if (msg.m_sInputAction.GetString() == "Left")
    m_iTargetDirection = 3;

  if (msg.m_sInputAction.GetString() == "Right")
    m_iTargetDirection = 1;
}

void PacManComponent::OnMsgTriggerTriggered(ezMsgTriggerTriggered& msg)
{
  if (msg.m_TriggerState != ezTriggerState::Activated)
    return;

  if (msg.m_sMessage.GetString() == "Ghost")
  {
    ezHashedString hs;

    hs.Assign("Stats");
    auto pBlackboard = ezBlackboard::GetOrCreateGlobal(hs);

    hs.Assign("PacManState");
    pBlackboard->RegisterEntry(hs, 0);
    pBlackboard->SetEntryValue(hs, 0).AssertSuccess();
    return;
  }

  if (!msg.m_hTriggeringObject.IsInvalidated())
  {
    ezGameObject* pObject = nullptr;
    if (GetWorld()->TryGetObject(msg.m_hTriggeringObject, pObject))
    {
      if (msg.m_sMessage.GetString() == "Pickup")
      {
        if (pObject->GetName() == "Coin")
        {
          GetWorld()->DeleteObjectDelayed(pObject->GetHandle());

          ezHashedString hs;
          hs.Assign("Stats");

          auto pBlackboard = ezBlackboard::GetOrCreateGlobal(hs);

          ezTempHashedString ce("CoinsEaten");

          if (pBlackboard->GetEntry(ce) == nullptr)
          {
            hs.Assign("CoinsEaten");
            pBlackboard->RegisterEntry(hs, 0);
          }

          ezVariant value = pBlackboard->GetEntryValue(ezTempHashedString("CoinsEaten"));
          value = value.Get<ezInt32>() + 1;

          pBlackboard->SetEntryValue(ezTempHashedString("CoinsEaten"), value).AssertSuccess();

          // spawn a collect coin effect
          {
            ezResourceLock<ezPrefabResource> pPrefab(m_hCollectCoint, ezResourceAcquireMode::AllowLoadingFallback_NeverFail);
            if (pPrefab.GetAcquireResult() == ezResourceAcquireResult::Final)
            {
              pPrefab->InstantiatePrefab(*GetWorld(), pObject->GetGlobalTransform(), {});
            }
          }
        }
      }
    }
  }
}

void PacManComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();
}

void PacManComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();
}
