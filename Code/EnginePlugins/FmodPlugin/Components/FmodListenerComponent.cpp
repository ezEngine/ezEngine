#include <FmodPlugin/FmodPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <FmodPlugin/Components/FmodListenerComponent.h>
#include <FmodPlugin/FmodIncludes.h>
#include <FmodPlugin/FmodSingleton.h>

ezFmodListenerComponentManager::ezFmodListenerComponentManager(ezWorld* pWorld)
  : ezComponentManager(pWorld)
{
}

void ezFmodListenerComponentManager::Initialize()
{
  SUPER::Initialize();

  {
    auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezFmodListenerComponentManager::UpdateListeners, this);
    desc.m_Phase = ezWorldModule::UpdateFunctionDesc::Phase::PostTransform;
    desc.m_bOnlyUpdateWhenSimulating = true;

    this->RegisterUpdateFunction(desc);
  }
}

void ezFmodListenerComponentManager::UpdateListeners(const ezWorldModule::UpdateContext& context)
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
EZ_BEGIN_COMPONENT_TYPE(ezFmodListenerComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("ListenerIndex", m_uiListenerIndex),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezFmodListenerComponent::ezFmodListenerComponent() = default;
ezFmodListenerComponent::~ezFmodListenerComponent() = default;

void ezFmodListenerComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_uiListenerIndex;
}

void ezFmodListenerComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_uiListenerIndex;
}

void ezFmodListenerComponent::Update()
{
  const auto pos = GetOwner()->GetGlobalPosition();
  const auto vel = GetOwner()->GetLinearVelocity();
  const auto fwd = (GetOwner()->GetGlobalRotation() * ezVec3::UnitXAxis()).GetNormalized();
  const auto up = (GetOwner()->GetGlobalRotation() * ezVec3::UnitZAxis()).GetNormalized();

  ezFmod::GetSingleton()->SetListener(m_uiListenerIndex, pos, fwd, up, vel);
}

EZ_STATICLINK_FILE(FmodPlugin, FmodPlugin_Components_FmodListenerComponent);
