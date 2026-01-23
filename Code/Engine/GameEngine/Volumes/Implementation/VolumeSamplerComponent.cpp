#include <GameEngine/GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Volumes/VolumeSamplerComponent.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezVolumeSamplerValue, ezNoBase, 1, ezRTTIDefaultAllocator<ezVolumeSamplerValue>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Name", m_sName),
    EZ_MEMBER_PROPERTY("DefaultValue", m_DefaultValue)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    EZ_MEMBER_PROPERTY("InterpolationDuration", m_InterpolationDuration),
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;
// clang-format on

ezResult ezVolumeSamplerValue::Serialize(ezStreamWriter& inout_stream) const
{
  inout_stream << m_sName;
  inout_stream << m_DefaultValue;
  inout_stream << m_InterpolationDuration;

  return EZ_SUCCESS;
}

ezResult ezVolumeSamplerValue::Deserialize(ezStreamReader& inout_stream)
{
  inout_stream >> m_sName;
  inout_stream >> m_DefaultValue;
  inout_stream >> m_InterpolationDuration;

  return EZ_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezVolumeSamplerComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("VolumeType", GetVolumeType, SetVolumeType)->AddAttributes(new ezDynamicStringEnumAttribute("SpatialDataCategoryEnum"), new ezDefaultValueAttribute("GenericVolume")),
    EZ_ARRAY_ACCESSOR_PROPERTY("Values", Values_GetCount, Values_GetMapping, Values_SetMapping, Values_Insert, Values_Remove),
    EZ_ACCESSOR_PROPERTY("AttachToMainCamera", GetAttachToMainCamera, SetAttachToMainCamera),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(RegisterValue, In, "Name", In, "DefaultValue", In, "InterpolationDuration"),
    EZ_SCRIPT_FUNCTION_PROPERTY(GetValue, In, "Name"),
    EZ_SCRIPT_FUNCTION_PROPERTY(GetFloatValue, In, "Name", In, "FallbackValue"),
    EZ_SCRIPT_FUNCTION_PROPERTY(GetColorValue, In, "Name", In, "FallbackValue"),
  }
  EZ_END_FUNCTIONS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Gameplay"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezVolumeSamplerComponent::ezVolumeSamplerComponent()
{
  m_pSampler = EZ_DEFAULT_NEW(ezVolumeSampler);
}

ezVolumeSamplerComponent::ezVolumeSamplerComponent(ezVolumeSamplerComponent&& other) = default;
ezVolumeSamplerComponent::~ezVolumeSamplerComponent() = default;
ezVolumeSamplerComponent& ezVolumeSamplerComponent::operator=(ezVolumeSamplerComponent&& other) = default;

void ezVolumeSamplerComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  ezStreamWriter& s = inout_stream.GetStream();

  auto& sCategory = ezSpatialData::GetCategoryName(m_SpatialCategory);
  s << sCategory;
  s << m_bAttachToMainCamera;

  s.WriteArray(m_Values).IgnoreResult();
}

void ezVolumeSamplerComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = inout_stream.GetStream();

  ezHashedString sCategory;
  s >> sCategory;
  m_SpatialCategory = ezSpatialData::RegisterCategory(sCategory, ezSpatialData::Flags::None);

  s >> m_bAttachToMainCamera;

  ezDynamicArray<ezVolumeSamplerValue> values;
  s.ReadArray(values).IgnoreResult();
  RegisterSamplerValues(values);
}

void ezVolumeSamplerComponent::SetVolumeType(const char* szType)
{
  m_SpatialCategory = ezSpatialData::RegisterCategory(szType, ezSpatialData::Flags::None);
}

const char* ezVolumeSamplerComponent::GetVolumeType() const
{
  return ezSpatialData::GetCategoryName(m_SpatialCategory);
}

void ezVolumeSamplerComponent::SetAttachToMainCamera(bool bAttach)
{
  m_bAttachToMainCamera = bAttach;
}

void ezVolumeSamplerComponent::RegisterValue(const ezHashedString& sName, const ezVariant& defaultValue, ezTime interpolationDuration)
{
  m_pSampler->RegisterValue(sName, defaultValue, interpolationDuration);
}

ezVariant ezVolumeSamplerComponent::GetValue(const ezHashedString& sName) const
{
  return m_pSampler->GetValue(sName);
}

float ezVolumeSamplerComponent::GetFloatValue(const ezHashedString& sName, float fFallbackValue) const
{
  ezVariant varValue = GetValue(sName);
  if (varValue.CanConvertTo<float>())
  {
    return varValue.ConvertTo<float>();
  }

  return fFallbackValue;
}

ezColor ezVolumeSamplerComponent::GetColorValue(const ezHashedString& sName, const ezColor& fallbackValue) const
{
  ezVariant varValue = GetValue(sName);
  if (varValue.CanConvertTo<ezColor>())
  {
    return varValue.ConvertTo<ezColor>();
  }

  return fallbackValue;
}

void ezVolumeSamplerComponent::Values_SetMapping(ezUInt32 i, const ezVolumeSamplerValue& mapping)
{
  m_Values.EnsureCount(i + 1);
  m_Values[i] = mapping;

  RegisterSamplerValues(m_Values);
}

void ezVolumeSamplerComponent::Values_Insert(ezUInt32 uiIndex, const ezVolumeSamplerValue& mapping)
{
  m_Values.InsertAt(uiIndex, mapping);

  RegisterSamplerValues(m_Values);
}

void ezVolumeSamplerComponent::Values_Remove(ezUInt32 uiIndex)
{
  m_Values.RemoveAtAndCopy(uiIndex);

  RegisterSamplerValues(m_Values);
}

void ezVolumeSamplerComponent::RegisterSamplerValues(ezArrayPtr<const ezVolumeSamplerValue> values)
{
  m_pSampler->DeregisterAllValues();

  for (auto& value : values)
  {
    if (value.m_sName.IsEmpty())
      continue;

    m_pSampler->RegisterValue(value.m_sName, value.m_DefaultValue, value.m_InterpolationDuration);
  }
}

void ezVolumeSamplerComponent::Update()
{
  ezWorld* pWorld = GetWorld();
  ezVec3 vSamplePos = GetOwner()->GetGlobalPosition();

  if (GetAttachToMainCamera())
  {
    if (ezView* pView = ezRenderWorld::GetViewByUsageHint(ezCameraUsageHint::MainView, ezCameraUsageHint::EditorView, pWorld))
    {
      vSamplePos = pView->GetCullingCamera()->GetCenterPosition();
    }
  }

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
}


EZ_STATICLINK_FILE(GameEngine, GameEngine_Volumes_Implementation_VolumeSamplerComponent);
