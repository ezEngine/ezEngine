#include <GameEngine/GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Effects/PostProcessing/PostProcessingComponent.h>
#include <RendererCore/Components/CameraComponent.h>
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
    EZ_MEMBER_PROPERTY("DefaultValue", m_DefaultValue)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
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
    EZ_ACCESSOR_PROPERTY("VolumeType", GetVolumeType, SetVolumeType)->AddAttributes(new ezDynamicStringEnumAttribute("SpatialDataCategoryEnum"), new ezDefaultValueAttribute("GenericVolume")),
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
ezPostProcessingComponent& ezPostProcessingComponent::operator=(ezPostProcessingComponent&& other) = default;

void ezPostProcessingComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  ezStreamWriter& s = inout_stream.GetStream();

  auto& sCategory = ezSpatialData::GetCategoryName(m_SpatialCategory);
  s << sCategory;

  s.WriteArray(m_Mappings).IgnoreResult();
}

void ezPostProcessingComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = inout_stream.GetStream();

  ezHashedString sCategory;
  s >> sCategory;
  m_SpatialCategory = ezSpatialData::RegisterCategory(sCategory, ezSpatialData::Flags::None);

  s.ReadArray(m_Mappings).IgnoreResult();
}

void ezPostProcessingComponent::SetVolumeType(const char* szType)
{
  m_SpatialCategory = ezSpatialData::RegisterCategory(szType, ezSpatialData::Flags::None);
}

const char* ezPostProcessingComponent::GetVolumeType() const
{
  return ezSpatialData::GetCategoryName(m_SpatialCategory);
}

void ezPostProcessingComponent::Initialize()
{
  m_pSampler = EZ_DEFAULT_NEW(ezVolumeSampler);

  RegisterSamplerValues();
}

void ezPostProcessingComponent::Deinitialize()
{
  m_pSampler = nullptr;
}

void ezPostProcessingComponent::OnActivated()
{
  SUPER::OnActivated();

  ezCameraComponent* pCameraComponent = nullptr;
  if (GetOwner()->TryGetComponentOfBaseType(pCameraComponent))
  {
    m_hCameraComponent = pCameraComponent->GetHandle();
  }
}

void ezPostProcessingComponent::OnDeactivated()
{
  ResetViewProperties();

  m_hCameraComponent.Invalidate();

  SUPER::OnDeactivated();
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

  ResetViewProperties();
  RegisterSamplerValues();
}

ezView* ezPostProcessingComponent::FindView() const
{
  const ezWorld* pWorld = GetWorld();
  ezView* pView = nullptr;

  const ezCameraComponent* pCameraComponent = nullptr;
  if (pWorld->TryGetComponent(m_hCameraComponent, pCameraComponent) && pCameraComponent->GetUsageHint() == ezCameraUsageHint::RenderTarget)
  {
    ezRenderWorld::TryGetView(pCameraComponent->GetRenderTargetView(), pView);
  }

  if (pView == nullptr)
  {
    pView = ezRenderWorld::GetViewByUsageHint(ezCameraUsageHint::MainView, ezCameraUsageHint::EditorView, pWorld);
  }

  return pView;
}

void ezPostProcessingComponent::RegisterSamplerValues()
{
  if (m_pSampler == nullptr)
    return;

  m_pSampler->DeregisterAllValues();

  for (auto& mapping : m_Mappings)
  {
    if (mapping.m_sVolumeValueName.IsEmpty())
      continue;

    m_pSampler->RegisterValue(mapping.m_sVolumeValueName, mapping.m_DefaultValue, mapping.m_InterpolationDuration);
  }
}

void ezPostProcessingComponent::ResetViewProperties()
{
  if (ezView* pView = FindView())
  {
    pView->ResetRenderPassProperties();
  }
}

void ezPostProcessingComponent::SampleAndSetViewProperties()
{
  ezView* pView = FindView();
  if (pView == nullptr)
    return;

  const ezVec3 vSamplePos = pView->GetCullingCamera()->GetCenterPosition();

  ezWorld* pWorld = GetWorld();
  ezTime deltaTime;
  if (pWorld->GetWorldSimulationEnabled())
  {
    deltaTime = pWorld->GetClock().GetTimeDiff();
  }
  else
  {
    deltaTime = ezClock::GetGlobalClock()->GetTimeDiff();
  }

  m_pSampler->SampleAtPosition(*pWorld, m_SpatialCategory, vSamplePos, deltaTime);

  for (auto& mapping : m_Mappings)
  {
    if (mapping.m_sRenderPassName.IsEmpty() || mapping.m_sPropertyName.IsEmpty())
      continue;

    ezVariant value;
    if (mapping.m_sVolumeValueName.IsEmpty())
    {
      value = mapping.m_DefaultValue;
    }
    else
    {
      value = m_pSampler->GetValue(mapping.m_sVolumeValueName);
    }

    pView->SetRenderPassProperty(mapping.m_sRenderPassName, mapping.m_sPropertyName, value);
  }
}


EZ_STATICLINK_FILE(GameEngine, GameEngine_Effects_PostProcessing_Implementation_PostProcessingComponent);
