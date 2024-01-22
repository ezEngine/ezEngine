#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Lights/ReflectionProbeComponentBase.h>

#include <Core/Graphics/Camera.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/View.h>

namespace
{
  static ezVariantArray GetDefaultExcludeTags()
  {
    ezVariantArray value(ezStaticsAllocatorWrapper::GetAllocator());
    value.PushBack(ezStringView("SkyLight"));
    return value;
  }
} // namespace


// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezReflectionProbeComponentBase, 2, ezRTTINoAllocator)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_ACCESSOR_PROPERTY("ReflectionProbeMode", ezReflectionProbeMode, GetReflectionProbeMode, SetReflectionProbeMode)->AddAttributes(new ezDefaultValueAttribute(ezReflectionProbeMode::Static), new ezGroupAttribute("Capture Description")),
    EZ_SET_ACCESSOR_PROPERTY("IncludeTags", GetIncludeTags, InsertIncludeTag, RemoveIncludeTag)->AddAttributes(new ezTagSetWidgetAttribute("Default")),
    EZ_SET_ACCESSOR_PROPERTY("ExcludeTags", GetExcludeTags, InsertExcludeTag, RemoveExcludeTag)->AddAttributes(new ezTagSetWidgetAttribute("Default"), new ezDefaultValueAttribute(GetDefaultExcludeTags())),
    EZ_ACCESSOR_PROPERTY("NearPlane", GetNearPlane, SetNearPlane)->AddAttributes(new ezDefaultValueAttribute(0.0f), new ezClampValueAttribute(0.0f, {}), new ezMinValueTextAttribute("Auto")),
    EZ_ACCESSOR_PROPERTY("FarPlane", GetFarPlane, SetFarPlane)->AddAttributes(new ezDefaultValueAttribute(100.0f), new ezClampValueAttribute(0.01f, 10000.0f)),
    EZ_ACCESSOR_PROPERTY("CaptureOffset", GetCaptureOffset, SetCaptureOffset),
    EZ_ACCESSOR_PROPERTY("ShowDebugInfo", GetShowDebugInfo, SetShowDebugInfo),
    EZ_ACCESSOR_PROPERTY("ShowMipMaps", GetShowMipMaps, SetShowMipMaps),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Rendering/Reflections"),
    new ezTransformManipulatorAttribute("CaptureOffset"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezReflectionProbeComponentBase::ezReflectionProbeComponentBase()
{
  m_Desc.m_uniqueID = ezUuid::MakeUuid();
}

ezReflectionProbeComponentBase::~ezReflectionProbeComponentBase() = default;

void ezReflectionProbeComponentBase::SetReflectionProbeMode(ezEnum<ezReflectionProbeMode> mode)
{
  m_Desc.m_Mode = mode;
  m_bStatesDirty = true;
}

ezEnum<ezReflectionProbeMode> ezReflectionProbeComponentBase::GetReflectionProbeMode() const
{
  return m_Desc.m_Mode;
}

const ezTagSet& ezReflectionProbeComponentBase::GetIncludeTags() const
{
  return m_Desc.m_IncludeTags;
}

void ezReflectionProbeComponentBase::InsertIncludeTag(const char* szTag)
{
  m_Desc.m_IncludeTags.SetByName(szTag);
  m_bStatesDirty = true;
}

void ezReflectionProbeComponentBase::RemoveIncludeTag(const char* szTag)
{
  m_Desc.m_IncludeTags.RemoveByName(szTag);
  m_bStatesDirty = true;
}


const ezTagSet& ezReflectionProbeComponentBase::GetExcludeTags() const
{
  return m_Desc.m_ExcludeTags;
}

void ezReflectionProbeComponentBase::InsertExcludeTag(const char* szTag)
{
  m_Desc.m_ExcludeTags.SetByName(szTag);
  m_bStatesDirty = true;
}

void ezReflectionProbeComponentBase::RemoveExcludeTag(const char* szTag)
{
  m_Desc.m_ExcludeTags.RemoveByName(szTag);
  m_bStatesDirty = true;
}

void ezReflectionProbeComponentBase::SetNearPlane(float fNearPlane)
{
  m_Desc.m_fNearPlane = fNearPlane;
  m_bStatesDirty = true;
}

void ezReflectionProbeComponentBase::SetFarPlane(float fFarPlane)
{
  m_Desc.m_fFarPlane = fFarPlane;
  m_bStatesDirty = true;
}

void ezReflectionProbeComponentBase::SetCaptureOffset(const ezVec3& vOffset)
{
  m_Desc.m_vCaptureOffset = vOffset;
  m_bStatesDirty = true;
}

void ezReflectionProbeComponentBase::SetShowDebugInfo(bool bShowDebugInfo)
{
  m_Desc.m_bShowDebugInfo = bShowDebugInfo;
  m_bStatesDirty = true;
}

bool ezReflectionProbeComponentBase::GetShowDebugInfo() const
{
  return m_Desc.m_bShowDebugInfo;
}

void ezReflectionProbeComponentBase::SetShowMipMaps(bool bShowMipMaps)
{
  m_Desc.m_bShowMipMaps = bShowMipMaps;
  m_bStatesDirty = true;
}

bool ezReflectionProbeComponentBase::GetShowMipMaps() const
{
  return m_Desc.m_bShowMipMaps;
}

void ezReflectionProbeComponentBase::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  ezStreamWriter& s = inout_stream.GetStream();

  m_Desc.m_IncludeTags.Save(s);
  m_Desc.m_ExcludeTags.Save(s);
  s << m_Desc.m_Mode;
  s << m_Desc.m_bShowDebugInfo;
  s << m_Desc.m_uniqueID;
  s << m_Desc.m_fNearPlane;
  s << m_Desc.m_fFarPlane;
  s << m_Desc.m_vCaptureOffset;
}

void ezReflectionProbeComponentBase::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = inout_stream.GetStream();

  m_Desc.m_IncludeTags.Load(s, ezTagRegistry::GetGlobalRegistry());
  m_Desc.m_ExcludeTags.Load(s, ezTagRegistry::GetGlobalRegistry());
  s >> m_Desc.m_Mode;
  s >> m_Desc.m_bShowDebugInfo;
  s >> m_Desc.m_uniqueID;
  s >> m_Desc.m_fNearPlane;
  s >> m_Desc.m_fFarPlane;
  s >> m_Desc.m_vCaptureOffset;
}

float ezReflectionProbeComponentBase::ComputePriority(ezMsgExtractRenderData& msg, ezReflectionProbeRenderData* pRenderData, float fVolume, const ezVec3& vScale) const
{
  float fPriority = 0.0f;
  const float fLogVolume = ezMath::Log2(1.0f + fVolume); // +1 to make sure it never goes negative.
  // This sorting is only by size to make sure the probes in a cluster are iterating from smallest to largest on the GPU. Which probes are actually used is determined below by the returned priority.
  pRenderData->m_uiSortingKey = ezMath::FloatToInt(static_cast<float>(ezMath::MaxValue<ezUInt32>()) * fLogVolume / 40.0f);

  // #TODO This is a pretty poor distance / size based score.
  if (msg.m_pView)
  {
    if (auto pCamera = msg.m_pView->GetLodCamera())
    {
      float fDistance = (pCamera->GetPosition() - pRenderData->m_GlobalTransform.m_vPosition).GetLength();
      float fRadius = (ezMath::Abs(vScale.x) + ezMath::Abs(vScale.y) + ezMath::Abs(vScale.z)) / 3.0f;
      fPriority = fRadius / fDistance;
    }
  }

#ifdef EZ_SHOW_REFLECTION_PROBE_PRIORITIES
  ezStringBuilder s;
  s.SetFormat("{}, {}", pRenderData->m_uiSortingKey, fPriority);
  ezDebugRenderer::Draw3DText(GetWorld(), s, pRenderData->m_GlobalTransform.m_vPosition, ezColor::Wheat);
#endif
  return fPriority;
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_ReflectionProbeComponentBase);
