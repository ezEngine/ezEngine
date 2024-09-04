#include <GameEngine/GameEnginePCH.h>

#include <Core/GameApplication/GameApplicationBase.h>
#include <Core/Messages/TriggerMessage.h>
#include <Core/World/GameObject.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/GameState/GameState.h>
#include <GameEngine/Gameplay/SceneTransitionComponent.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezSceneLoadMode, 1)
  EZ_ENUM_CONSTANT(ezSceneLoadMode::None),
  EZ_ENUM_CONSTANT(ezSceneLoadMode::LoadAndSwitch),
  EZ_ENUM_CONSTANT(ezSceneLoadMode::Preload),
  EZ_ENUM_CONSTANT(ezSceneLoadMode::CancelPreload),
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_COMPONENT_TYPE(ezSceneTransitionComponent, 1 /* version */, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_MEMBER_PROPERTY("Mode", ezSceneLoadMode, m_Mode),
    EZ_MEMBER_PROPERTY("TargetScene", m_sTargetScene)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Scene",  ezDependencyFlags::Package)),
    EZ_MEMBER_PROPERTY("PreloadCollection", m_sPreloadCollectionFile)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_AssetCollection", ezDependencyFlags::Package)),
    EZ_MEMBER_PROPERTY("SpawnPoint", m_sSpawnPoint),
    EZ_MEMBER_PROPERTY("RelativeSpawnPosition", m_bRelativeSpawnPosition)->AddAttributes(new ezDefaultValueAttribute(true)),
  }
  EZ_END_PROPERTIES;

  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgTriggerTriggered, OnMsgTriggerTriggered),
  }
  EZ_END_MESSAGEHANDLERS;

  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Gameplay"),
  }
  EZ_END_ATTRIBUTES;

  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(StartTransition, In, "PositionOffset", In, "RotationOffset"),
    EZ_SCRIPT_FUNCTION_PROPERTY(StartTransitionWithOffsetTo, In, "GlobalPosition", In, "GlobalRotation"),
    EZ_SCRIPT_FUNCTION_PROPERTY(StartPreload),
    EZ_SCRIPT_FUNCTION_PROPERTY(CancelPreload),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezSceneTransitionComponent::ezSceneTransitionComponent() = default;
ezSceneTransitionComponent::~ezSceneTransitionComponent() = default;

void ezSceneTransitionComponent::StartTransition(const ezVec3& vPositionOffset, const ezQuat& qRotationOffset)
{
  if (auto pGameStateBase = ezGameApplicationBase::GetGameApplicationBaseInstance()->GetActiveGameState())
  {
    // we could move these functions into ezGameStateBase, but for now the dynamic cast should be fine
    // there is no good reason to have this functionality on the base class
    if (ezGameState* pGameState = ezDynamicCast<ezGameState*>(pGameStateBase))
    {
      pGameState->LoadScene(m_sTargetScene, m_sPreloadCollectionFile, m_sSpawnPoint, ezTransform(vPositionOffset, qRotationOffset));
    }
  }
}

void ezSceneTransitionComponent::StartTransitionWithOffsetTo(const ezVec3& vGlobalPosition, const ezQuat& qGlobalRotation)
{
  const ezTransform ownGlobal(vGlobalPosition, qGlobalRotation);
  const ezTransform rel = ezTransform::MakeLocalTransform(GetOwner()->GetGlobalTransform(), ownGlobal);

  StartTransition(rel.m_vPosition, rel.m_qRotation);
}

void ezSceneTransitionComponent::StartPreload()
{
  if (auto pGameStateBase = ezGameApplicationBase::GetGameApplicationBaseInstance()->GetActiveGameState())
  {
    if (ezGameState* pGameState = ezDynamicCast<ezGameState*>(pGameStateBase))
    {
      pGameState->StartBackgroundSceneLoading(m_sTargetScene, m_sPreloadCollectionFile);
    }
  }
}

void ezSceneTransitionComponent::CancelPreload()
{
  if (auto pGameStateBase = ezGameApplicationBase::GetGameApplicationBaseInstance()->GetActiveGameState())
  {
    if (ezGameState* pGameState = ezDynamicCast<ezGameState*>(pGameStateBase))
    {
      pGameState->CancelBackgroundSceneLoading();
    }
  }
}

void ezSceneTransitionComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_Mode;
  s << m_sTargetScene;
  s << m_sSpawnPoint;
  s << m_bRelativeSpawnPosition;
  s << m_sPreloadCollectionFile;
}

void ezSceneTransitionComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_Mode;
  s >> m_sTargetScene;
  s >> m_sSpawnPoint;
  s >> m_bRelativeSpawnPosition;
  s >> m_sPreloadCollectionFile;
}

void ezSceneTransitionComponent::OnMsgTriggerTriggered(ezMsgTriggerTriggered& ref_msg)
{
  if (ref_msg.m_TriggerState == ezTriggerState::Activated)
  {
    if (m_Mode == ezSceneLoadMode::None)
    {
      return;
    }

    if (m_Mode == ezSceneLoadMode::LoadAndSwitch)
    {
      ezTransform rel = ezTransform::MakeIdentity();

      if (m_bRelativeSpawnPosition)
      {
        ezGameObject* pPlayer;
        if (GetWorld()->TryGetObject(ref_msg.m_hTriggeringObject, pPlayer))
        {
          rel = ezTransform::MakeLocalTransform(GetOwner()->GetGlobalTransform(), pPlayer->GetGlobalTransform());
        }
      }

      StartTransition(rel.m_vPosition, rel.m_qRotation);

      return;
    }

    if (m_Mode == ezSceneLoadMode::Preload)
    {
      StartPreload();
      return;
    }

    if (m_Mode == ezSceneLoadMode::CancelPreload)
    {
      CancelPreload();
      return;
    }
  }
}


EZ_STATICLINK_FILE(GameEngine, GameEngine_Gameplay_Implementation_SceneTransitionComponent);
