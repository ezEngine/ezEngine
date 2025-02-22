#include <GameEngine/GameEnginePCH.h>

#include <Core/Input/InputManager.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Strings/HashedString.h>
#include <GameEngine/Animation/Skeletal/AnimatedMeshComponent.h>
#include <GameEngine/Animation/Skeletal/AnimationControllerComponent.h>
#include <GameEngine/Gameplay/BlackboardComponent.h>
#include <GameEngine/Physics/CharacterControllerComponent.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphResource.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezAnimationControllerComponent, 3, ezComponentMode::Static);
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_RESOURCE_MEMBER_PROPERTY("AnimGraph", m_hAnimGraph)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Keyframe_Graph")),

    EZ_ENUM_MEMBER_PROPERTY("RootMotionMode", ezRootMotionMode, m_RootMotionMode),
    EZ_ENUM_MEMBER_PROPERTY("InvisibleUpdateRate", ezAnimationInvisibleUpdateRate, m_InvisibleUpdateRate),
    EZ_MEMBER_PROPERTY("EnableIK", m_bEnableIK),
  }
  EZ_END_PROPERTIES;

  EZ_BEGIN_ATTRIBUTES
  {
      new ezCategoryAttribute("Animation"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezAnimationControllerComponent::ezAnimationControllerComponent() = default;
ezAnimationControllerComponent::~ezAnimationControllerComponent() = default;

void ezAnimationControllerComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_hAnimGraph;
  s << m_RootMotionMode;
  s << m_InvisibleUpdateRate;
  s << m_bEnableIK;
}

void ezAnimationControllerComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_hAnimGraph;
  s >> m_RootMotionMode;

  if (uiVersion >= 2)
  {
    s >> m_InvisibleUpdateRate;
  }

  if (uiVersion >= 3)
  {
    s >> m_bEnableIK;
  }
}

void ezAnimationControllerComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  if (!m_hAnimGraph.IsValid())
    return;

  ezMsgQueryAnimationSkeleton msg;
  GetOwner()->SendMessage(msg);

  if (!msg.m_hSkeleton.IsValid())
    return;

  m_AnimController.Initialize(msg.m_hSkeleton, m_PoseGenerator, ezBlackboardComponent::FindBlackboard(GetOwner()));
  m_AnimController.AddAnimGraph(m_hAnimGraph);
}

void ezAnimationControllerComponent::Update()
{
  ezTime tMinStep = ezTime::MakeFromSeconds(0);
  ezVisibilityState::Enum visType = GetOwner()->GetVisibilityState();

  if (visType != ezVisibilityState::Direct)
  {
    if (m_InvisibleUpdateRate == ezAnimationInvisibleUpdateRate::Pause && visType == ezVisibilityState::Invisible)
      return;

    tMinStep = ezAnimationInvisibleUpdateRate::GetTimeStep(m_InvisibleUpdateRate);
  }

  m_ElapsedTimeSinceUpdate += GetWorld()->GetClock().GetTimeDiff();

  if (m_ElapsedTimeSinceUpdate < tMinStep)
    return;

  m_AnimController.Update(m_ElapsedTimeSinceUpdate, GetOwner(), m_bEnableIK);
  m_ElapsedTimeSinceUpdate = ezTime::MakeZero();

  ezVec3 translation;
  ezAngle rotationX;
  ezAngle rotationY;
  ezAngle rotationZ;
  m_AnimController.GetRootMotion(translation, rotationX, rotationY, rotationZ);

  ezRootMotionMode::Apply(m_RootMotionMode, GetOwner(), translation, rotationX, rotationY, rotationZ);
}

//////////////////////////////////////////////////////////////////////////


ezAnimationControllerComponentManager::ezAnimationControllerComponentManager(ezWorld* pWorld)
  : ezComponentManager<class ezAnimationControllerComponent, ezBlockStorageType::FreeList>(pWorld)
{
}

void ezAnimationControllerComponentManager::Initialize()
{
  auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezAnimationControllerComponentManager::Update, this);
  desc.m_bOnlyUpdateWhenSimulating = true;
  desc.m_Phase = ezWorldUpdatePhase::PreAsync; // TODO: currently can't run in Async phase

  this->RegisterUpdateFunction(desc);

  ezResourceManager::GetResourceEvents().AddEventHandler(ezMakeDelegate(&ezAnimationControllerComponentManager::ResourceEvent, this));
}

void ezAnimationControllerComponentManager::Deinitialize()
{
  ezResourceManager::GetResourceEvents().RemoveEventHandler(ezMakeDelegate(&ezAnimationControllerComponentManager::ResourceEvent, this));
}

void ezAnimationControllerComponentManager::Update(const ezWorldModule::UpdateContext& context)
{
  {
    for (auto hComponent : m_ComponentsToReset)
    {
      ezAnimationControllerComponent* pComp;
      if (GetWorld()->TryGetComponent(hComponent, pComp))
      {
        pComp->OnSimulationStarted(); // just run this again
      }
    }

    m_ComponentsToReset.Clear();
  }

  for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    ComponentType* pComponent = it;
    if (pComponent->IsActiveAndInitialized())
    {
      pComponent->Update();
    }
  }
}

void ezAnimationControllerComponentManager::ResourceEvent(const ezResourceEvent& e)
{
  if (e.m_Type == ezResourceEvent::Type::ResourceContentUnloading)
  {
    if (e.m_pResource->GetDynamicRTTI() == ezGetStaticRTTI<ezAnimGraphResource>())
    {
      ezAnimGraphResourceHandle hResource((ezAnimGraphResource*)(e.m_pResource));

      for (auto it = GetComponents(); it.IsValid(); it.Next())
      {
        if (!it->IsActiveAndSimulating())
          continue;

        if (it->m_hAnimGraph == hResource)
        {
          if (!m_ComponentsToReset.Contains(it->GetHandle()))
          {
            m_ComponentsToReset.PushBack(it->GetHandle());
          }
        }
      }
    }
  }
}

EZ_STATICLINK_FILE(GameEngine, GameEngine_Animation_Skeletal_Implementation_AnimationControllerComponent);
