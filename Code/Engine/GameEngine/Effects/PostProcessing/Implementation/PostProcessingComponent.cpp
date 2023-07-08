#include <GameEngine/GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Effects/PostProcessing/PostProcessingComponent.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

ezPostProcessingComponentManager::ezPostProcessingComponentManager(ezWorld* pWorld)
  : ezComponentManager(pWorld)
{
}

void ezPostProcessingComponentManager::Initialize()
{
  auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezPostProcessingComponentManager::UpdateComponents, this);
  desc.m_Phase = UpdateFunctionDesc::Phase::PostTransform;

  RegisterUpdateFunction(desc);
}

void ezPostProcessingComponentManager::UpdateComponents(const UpdateContext& context)
{
  for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    ComponentType* pComponent = it;
    if (pComponent->IsActiveAndInitialized())
    {
      pComponent->SampleAndSetViewProperties();
    }
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezPostProcessingValueMapping, ezNoBase, 1, ezRTTIDefaultAllocator<ezPostProcessingValueMapping>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("RenderPass", m_sRenderPassName),
    EZ_MEMBER_PROPERTY("Property", m_sPropertyName),
    EZ_MEMBER_PROPERTY("VolumeValue", m_sVolumeValueName),
    EZ_MEMBER_PROPERTY("DefaultValue", m_DefaultValue)->AddAttributes(new ezDefaultValueAttribute(0)),
    EZ_MEMBER_PROPERTY("InterpolationDuration", m_InterpolationDuration),
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;
// clang-format on

ezResult ezPostProcessingValueMapping::Serialize(ezStreamWriter& inout_stream) const
{
  inout_stream << m_sRenderPassName;
  inout_stream << m_sPropertyName;
  inout_stream << m_sVolumeValueName;
  inout_stream << m_DefaultValue;
  inout_stream << m_InterpolationDuration;

  return EZ_SUCCESS;
}

ezResult ezPostProcessingValueMapping::Deserialize(ezStreamReader& inout_stream)
{
  inout_stream >> m_sRenderPassName;
  inout_stream >> m_sPropertyName;
  inout_stream >> m_sVolumeValueName;
  inout_stream >> m_DefaultValue;
  inout_stream >> m_InterpolationDuration;

  return EZ_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezPostProcessingComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_MEMBER_PROPERTY("UsageHint", ezCameraUsageHint, m_UsageHint)->AddAttributes(new ezDefaultValueAttribute(ezCameraUsageHint::MainView)),
    EZ_ENUM_MEMBER_PROPERTY("AlternativeUsageHint", ezCameraUsageHint, m_AlternativeUsageHint)->AddAttributes(new ezDefaultValueAttribute(ezCameraUsageHint::EditorView)),
    EZ_ARRAY_ACCESSOR_PROPERTY("Mappings", Mappings_GetCount, Mappings_GetMapping, Mappings_SetMapping, Mappings_Insert, Mappings_Remove),
  }
  EZ_END_PROPERTIES;

  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Effects"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezPostProcessingComponent::ezPostProcessingComponent() = default;
ezPostProcessingComponent::ezPostProcessingComponent(ezPostProcessingComponent&& other) = default;
ezPostProcessingComponent::~ezPostProcessingComponent() = default;

void ezPostProcessingComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  ezStreamWriter& s = inout_stream.GetStream();

  s << m_UsageHint;
  s << m_AlternativeUsageHint;
  s.WriteArray(m_Mappings).IgnoreResult();
}

void ezPostProcessingComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = inout_stream.GetStream();

  s >> m_UsageHint;
  s >> m_AlternativeUsageHint;
  s.ReadArray(m_Mappings).IgnoreResult();
}

ezPostProcessingComponent& ezPostProcessingComponent::operator=(ezPostProcessingComponent&& other) = default;

void ezPostProcessingComponent::Initialize()
{
  m_pSampler = EZ_DEFAULT_NEW(ezVolumeSampler);

  RegisterSamplerValues();
}

void ezPostProcessingComponent::Deinitialize()
{
  m_pSampler = nullptr;
}

void ezPostProcessingComponent::Mappings_SetMapping(ezUInt32 i, const ezPostProcessingValueMapping& mapping)
{
  m_Mappings.EnsureCount(i + 1);
  m_Mappings[i] = mapping;

  RegisterSamplerValues();
}

void ezPostProcessingComponent::Mappings_Insert(ezUInt32 uiIndex, const ezPostProcessingValueMapping& mapping)
{
  m_Mappings.Insert(mapping, uiIndex);

  RegisterSamplerValues();
}

void ezPostProcessingComponent::Mappings_Remove(ezUInt32 uiIndex)
{
  m_Mappings.RemoveAtAndCopy(uiIndex);

  RegisterSamplerValues();
}

void ezPostProcessingComponent::RegisterSamplerValues()
{
  m_pSampler->DeregisterAllValues();

  for (auto& mapping : m_Mappings)
  {
    m_pSampler->RegisterValue(mapping.m_sVolumeValueName, mapping.m_DefaultValue, mapping.m_InterpolationDuration);
  }
}

void ezPostProcessingComponent::SampleAndSetViewProperties()
{
  ezWorld* pWorld = GetWorld();

  ezView* pView = ezRenderWorld::GetViewByUsageHint(m_UsageHint, m_AlternativeUsageHint, pWorld);
  if (pView == nullptr)
    return;

  const ezVec3 vSamplePos = GetOwner()->GetGlobalPosition();

  ezTime deltaTime;
  if (pWorld->GetWorldSimulationEnabled())
  {
    deltaTime = pWorld->GetClock().GetTimeDiff();
  }
  else
  {
    deltaTime = ezClock::GetGlobalClock()->GetTimeDiff();
  }

  m_pSampler->SampleAtPosition(*pWorld, vSamplePos, deltaTime);

  for (auto& mapping : m_Mappings)
  {
    ezVariant value = m_pSampler->GetValue(mapping.m_sVolumeValueName);
    pView->SetRenderPassProperty(mapping.m_sRenderPassName, mapping.m_sPropertyName, value);
  }
}
