#include <MiniAudioPlugin/MiniAudioPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <MiniAudioPlugin/Components/MiniAudioListenerComponent.h>
#include <MiniAudioPlugin/MiniAudioSingleton.h>

ezMiniAudioListenerComponentManager::ezMiniAudioListenerComponentManager(ezWorld* pWorld)
  : ezComponentManager(pWorld)
{
}

void ezMiniAudioListenerComponentManager::Initialize()
{
  SUPER::Initialize();

  {
    auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezMiniAudioListenerComponentManager::UpdateListeners, this);
    desc.m_Phase = ezWorldUpdatePhase::PostTransform;
    desc.m_bOnlyUpdateWhenSimulating = true;

    this->RegisterUpdateFunction(desc);
  }
}

void ezMiniAudioListenerComponentManager::UpdateListeners(const ezWorldModule::UpdateContext& context)
{
  for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    ComponentType* pComponent = it;
    if (pComponent->IsActiveAndInitialized())
    {
      pComponent->Update();
    }
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezMiniAudioListenerComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Sound/MiniAudio"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezMiniAudioListenerComponent::ezMiniAudioListenerComponent() = default;
ezMiniAudioListenerComponent::~ezMiniAudioListenerComponent() = default;

void ezMiniAudioListenerComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();
}

void ezMiniAudioListenerComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();
}

void ezMiniAudioListenerComponent::Update()
{
  const auto pos = GetOwner()->GetGlobalPosition();
  const auto vel = GetOwner()->GetLinearVelocity();
  const auto fwd = (GetOwner()->GetGlobalRotation() * ezVec3::MakeAxisX()).GetNormalized();
  const auto up = (GetOwner()->GetGlobalRotation() * ezVec3::MakeAxisZ()).GetNormalized();

  ezMiniAudioSingleton::GetSingleton()->SetListener(0, pos, fwd, up, vel);
}

