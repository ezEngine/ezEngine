#include <GameEngine/GameEnginePCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Utils/BlackboardTemplateResource.h>
#include <GameEngine/Volumes/VolumeComponent.h>

// clang-format off
EZ_BEGIN_ABSTRACT_COMPONENT_TYPE(ezVolumeComponent, 1)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Type", GetVolumeType, SetVolumeType)->AddAttributes(new ezDynamicStringEnumAttribute("SpatialDataCategoryEnum"), new ezDefaultValueAttribute("GenericVolume")),
    EZ_ACCESSOR_PROPERTY("SortOrder", GetSortOrder, SetSortOrder)->AddAttributes(new ezClampValueAttribute(-64.0f, 64.0f)),
    EZ_RESOURCE_ACCESSOR_PROPERTY("Template", GetTemplate, SetTemplate)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_BlackboardTemplate")),
    EZ_MAP_ACCESSOR_PROPERTY("Values", Reflection_GetKeys, Reflection_GetValue, Reflection_InsertValue, Reflection_RemoveValue),
  }
  EZ_END_PROPERTIES;

  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(SetValue, In, "Name", In, "Value"),
    EZ_SCRIPT_FUNCTION_PROPERTY(GetValue, In, "Name"),
  }
  EZ_END_FUNCTIONS;

  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Gameplay"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_ABSTRACT_COMPONENT_TYPE
// clang-format on

ezVolumeComponent::ezVolumeComponent() = default;
ezVolumeComponent::~ezVolumeComponent() = default;

void ezVolumeComponent::OnActivated()
{
  SUPER::OnActivated();

  InitializeFromTemplate();

  GetOwner()->UpdateLocalBounds();
}

void ezVolumeComponent::OnDeactivated()
{
  SUPER::OnDeactivated();

  RemoveReloadFunction();

  GetOwner()->UpdateLocalBounds();
}

void ezVolumeComponent::SetTemplate(const ezBlackboardTemplateResourceHandle& hResource)
{
  RemoveReloadFunction();

  m_hTemplateResource = hResource;

  if (IsActiveAndInitialized())
  {
    ReloadTemplate();
  }
}

void ezVolumeComponent::SetSortOrder(float fOrder)
{
  fOrder = ezMath::Clamp(fOrder, -64.0f, 64.0f);
  m_fSortOrder = fOrder;
}

void ezVolumeComponent::SetVolumeType(const char* szType)
{
  m_SpatialCategory = ezSpatialData::RegisterCategory(szType, ezSpatialData::Flags::None);

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

const char* ezVolumeComponent::GetVolumeType() const
{
  return ezSpatialData::GetCategoryName(m_SpatialCategory);
}

void ezVolumeComponent::SetValue(const ezHashedString& sName, const ezVariant& value)
{
  m_Values.Insert(sName, value);
}

void ezVolumeComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  ezStreamWriter& s = inout_stream.GetStream();

  s << m_fSortOrder;

  auto& sCategory = ezSpatialData::GetCategoryName(m_SpatialCategory);
  s << sCategory;

  s << m_hTemplateResource;

  // Only serialize overwritten values so a template change doesn't require a re-save of all volumes
  ezUInt32 numValues = m_OverwrittenValues.GetCount();
  s << numValues;
  for (auto& sName : m_OverwrittenValues)
  {
    ezVariant value;
    m_Values.TryGetValue(sName, value);

    s << sName;
    s << value;
  }
}

void ezVolumeComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = inout_stream.GetStream();

  s >> m_fSortOrder;

  ezHashedString sCategory;
  s >> sCategory;
  m_SpatialCategory = ezSpatialData::RegisterCategory(sCategory, ezSpatialData::Flags::None);

  s >> m_hTemplateResource;

  // m_OverwrittenValues is only used in editor so we don't write to it here
  ezUInt32 numValues = 0;
  s >> numValues;
  for (ezUInt32 i = 0; i < numValues; ++i)
  {
    ezHashedString sName;
    ezVariant value;
    s >> sName;
    s >> value;

    m_Values.Insert(sName, value);
  }
}

const ezRangeView<const ezString&, ezUInt32> ezVolumeComponent::Reflection_GetKeys() const
{
  return ezRangeView<const ezString&, ezUInt32>([]() -> ezUInt32
    { return 0; },
    [this]() -> ezUInt32
    { return m_OverwrittenValues.GetCount(); },
    [](ezUInt32& ref_uiIt)
    { ++ref_uiIt; },
    [this](const ezUInt32& uiIt) -> const ezString&
    { return m_OverwrittenValues[uiIt].GetString(); });
}

bool ezVolumeComponent::Reflection_GetValue(const char* szName, ezVariant& value) const
{
  return m_Values.TryGetValue(ezTempHashedString(szName), value);
}

void ezVolumeComponent::Reflection_InsertValue(const char* szName, const ezVariant& value)
{
  ezHashedString sName;
  sName.Assign(szName);

  // Only needed in editor
  if (GetUniqueID() != ezInvalidIndex && m_OverwrittenValues.Contains(sName) == false)
  {
    m_OverwrittenValues.PushBack(sName);
  }

  m_Values.Insert(sName, value);
}

void ezVolumeComponent::Reflection_RemoveValue(const char* szName)
{
  ezHashedString sName;
  sName.Assign(szName);

  m_OverwrittenValues.RemoveAndCopy(sName);

  m_Values.Remove(sName);
}

void ezVolumeComponent::InitializeFromTemplate()
{
  if (!m_hTemplateResource.IsValid())
    return;

  ezResourceLock<ezBlackboardTemplateResource> pTemplate(m_hTemplateResource, ezResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pTemplate.GetAcquireResult() != ezResourceAcquireResult::Final)
    return;

  for (const auto& entry : pTemplate->GetDescriptor().m_Entries)
  {
    if (m_Values.Contains(entry.m_sName) == false)
    {
      m_Values.Insert(entry.m_sName, entry.m_InitialValue);
    }
  }

  if (m_bReloadFunctionAdded == false)
  {
    GetWorld()->AddResourceReloadFunction(m_hTemplateResource, GetHandle(), nullptr,
      [](const ezWorld::ResourceReloadContext& context)
      {
        ezStaticCast<ezVolumeComponent*>(context.m_pComponent)->ReloadTemplate();
      });

    m_bReloadFunctionAdded = true;
  }
}

void ezVolumeComponent::ReloadTemplate()
{
  // Remove all values that are not overwritten
  ezHashTable<ezHashedString, ezVariant> overwrittenValues;
  for (auto& sName : m_OverwrittenValues)
  {
    overwrittenValues.Insert(sName, m_Values[sName]);
  }
  m_Values.Swap(overwrittenValues);

  InitializeFromTemplate();
}

void ezVolumeComponent::RemoveReloadFunction()
{
  if (m_bReloadFunctionAdded)
  {
    GetWorld()->RemoveResourceReloadFunction(m_hTemplateResource, GetHandle(), nullptr);

    m_bReloadFunctionAdded = false;
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezVolumeSphereComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Radius", GetRadius, SetRadius)->AddAttributes(new ezDefaultValueAttribute(5.0f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_ACCESSOR_PROPERTY("Falloff", GetFalloff, SetFalloff)->AddAttributes(new ezDefaultValueAttribute(0.5f), new ezClampValueAttribute(0.0f, 1.0f)),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgUpdateLocalBounds, OnUpdateLocalBounds),
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezSphereManipulatorAttribute("Radius"),
    new ezSphereVisualizerAttribute("Radius", ezColorScheme::LightUI(ezColorScheme::Cyan)),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezVolumeSphereComponent::ezVolumeSphereComponent() = default;
ezVolumeSphereComponent::~ezVolumeSphereComponent() = default;

void ezVolumeSphereComponent::SetRadius(float fRadius)
{
  if (m_fRadius != fRadius)
  {
    m_fRadius = fRadius;

    if (IsActiveAndInitialized())
    {
      GetOwner()->UpdateLocalBounds();
    }
  }
}

void ezVolumeSphereComponent::SetFalloff(float fFalloff)
{
  m_fFalloff = ezMath::Max(fFalloff, 0.0001f);
}

void ezVolumeSphereComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  ezStreamWriter& s = inout_stream.GetStream();

  s << m_fRadius;
  s << m_fFalloff;
}

void ezVolumeSphereComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = inout_stream.GetStream();

  s >> m_fRadius;
  s >> m_fFalloff;
}

void ezVolumeSphereComponent::OnUpdateLocalBounds(ezMsgUpdateLocalBounds& ref_msg) const
{
  ref_msg.AddBounds(ezBoundingSphere::MakeFromCenterAndRadius(ezVec3::MakeZero(), m_fRadius), m_SpatialCategory);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezVolumeBoxComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Extents", GetExtents, SetExtents)->AddAttributes(new ezDefaultValueAttribute(ezVec3(10.0f)), new ezClampValueAttribute(ezVec3(0), ezVariant())),
    EZ_ACCESSOR_PROPERTY("Falloff", GetFalloff, SetFalloff)->AddAttributes(new ezDefaultValueAttribute(ezVec3(0.5f)), new ezClampValueAttribute(ezVec3(0.0f), ezVec3(1.0f))),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgUpdateLocalBounds, OnUpdateLocalBounds),
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezBoxManipulatorAttribute("Extents", 1.0f, true),
    new ezBoxVisualizerAttribute("Extents", 1.0f, ezColorScheme::LightUI(ezColorScheme::Cyan)),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezVolumeBoxComponent::ezVolumeBoxComponent() = default;
ezVolumeBoxComponent::~ezVolumeBoxComponent() = default;

void ezVolumeBoxComponent::SetExtents(const ezVec3& vExtents)
{
  if (m_vExtents != vExtents)
  {
    m_vExtents = vExtents;

    if (IsActiveAndInitialized())
    {
      GetOwner()->UpdateLocalBounds();
    }
  }
}

void ezVolumeBoxComponent::SetFalloff(const ezVec3& vFalloff)
{
  m_vFalloff = vFalloff.CompMax(ezVec3(0.0001f));
}

void ezVolumeBoxComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  ezStreamWriter& s = inout_stream.GetStream();

  s << m_vExtents;
  s << m_vFalloff;
}

void ezVolumeBoxComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = inout_stream.GetStream();

  s >> m_vExtents;
  s >> m_vFalloff;
}

void ezVolumeBoxComponent::OnUpdateLocalBounds(ezMsgUpdateLocalBounds& ref_msg) const
{
  ref_msg.AddBounds(ezBoundingBoxSphere::MakeFromBox(ezBoundingBox::MakeFromMinMax(-m_vExtents * 0.5f, m_vExtents * 0.5f)), m_SpatialCategory);
}


EZ_STATICLINK_FILE(GameEngine, GameEngine_Volumes_Implementation_VolumeComponent);
