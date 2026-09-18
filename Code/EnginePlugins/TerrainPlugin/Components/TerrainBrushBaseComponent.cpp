#include <TerrainPlugin/TerrainPluginPCH.h>

#include <Core/Messages/TransformChangedMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Reflection/Implementation/PropertyAttributes.h>
#include <Foundation/Types/TagRegistry.h>
#include <RendererCore/Components/SplineComponent.h>
#include <TerrainPlugin/Components/TerrainBrushBaseComponent.h>
#include <TerrainPlugin/TerrainSystem.h>

// clang-format off
EZ_BEGIN_ABSTRACT_COMPONENT_TYPE(ezTerrainBrushBaseComponent, 2)
{
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgTransformChanged, OnMsgTransformChanged),
    EZ_MESSAGE_HANDLER(ezMsgSplineChanged, OnMsgSplineChanged),
  }
  EZ_END_MESSAGEHANDLERS;
}
EZ_END_ABSTRACT_COMPONENT_TYPE;
// clang-format on

ezTerrainBrushBaseComponent::ezTerrainBrushBaseComponent() = default;
ezTerrainBrushBaseComponent::~ezTerrainBrushBaseComponent() = default;

void ezTerrainBrushBaseComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();
  s << m_fHalfSizeX;
  s << m_fInnerRadius;
  s << m_fOuterRadius;
  s << m_fFalloff;
  s << m_fNoiseStrength;
  s << m_fNoiseFrequency;
  s << m_uiMaterialIndex;
  s << m_fMaterialStrength;
  s << m_iPriority;
  m_Tags.Save(s);

  s << m_bAffectPatches;
  s << m_bAffectVolumes;
}

void ezTerrainBrushBaseComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  s >> m_fHalfSizeX;
  s >> m_fInnerRadius;
  s >> m_fOuterRadius;
  s >> m_fFalloff;
  s >> m_fNoiseStrength;
  s >> m_fNoiseFrequency;
  s >> m_uiMaterialIndex;
  s >> m_fMaterialStrength;
  s >> m_iPriority;
  m_Tags.Load(s, ezTagRegistry::GetGlobalRegistry());

  if (uiVersion >= 2)
  {
    s >> m_bAffectPatches;
    s >> m_bAffectVolumes;
  }
}

void ezTerrainBrushBaseComponent::OnActivated()
{
  SUPER::OnActivated();
  GetOwner()->EnableStaticTransformChangesNotifications();

  // Spline changes may have been missed while inactive.
  m_hSplineCacheSource.Invalidate();
  RefreshBrushes();
}

void ezTerrainBrushBaseComponent::OnDeactivated()
{
  ClearBrushes();
  SUPER::OnDeactivated();
}

void ezTerrainBrushBaseComponent::OnMsgTransformChanged(ezMsgTransformChanged& msg)
{
  RefreshBrushes();
}

void ezTerrainBrushBaseComponent::OnMsgSplineChanged(ezMsgSplineChanged& msg)
{
  m_hSplineCacheSource.Invalidate();
  RefreshBrushes();
}

void ezTerrainBrushBaseComponent::ClearBrushes()
{
  if (m_BrushIndices.IsEmpty())
    return;

  auto* pSystem = GetWorld()->GetOrCreateModule<ezTerrainSystem>();
  for (ezUInt32 uiIdx : m_BrushIndices)
    pSystem->RemoveBrushData(uiIdx);
  m_BrushIndices.Clear();
}

void ezTerrainBrushBaseComponent::FillBrush(ezTerrainData_Brush& brush, const ezTransform& transform, float fHalfSizeX)
{
  brush.m_vPosition = transform.m_vPosition;
  brush.m_qRotation = transform.m_qRotation;
  brush.m_vHalfExtents.x = fHalfSizeX;
  brush.m_fInnerRadius = m_fInnerRadius;
  brush.m_fOuterRadius = m_fOuterRadius;
  brush.m_fFalloff = m_fFalloff;
  brush.m_uiMaterialIndex = m_uiMaterialIndex;
  brush.m_fMaterialStrength = m_fMaterialStrength;
  brush.m_bAffectHeightfields = m_bAffectPatches;
  brush.m_bAffectVoxels = m_bAffectVolumes;
  brush.m_fNoiseStrength = m_fNoiseStrength;
  brush.m_fNoiseFrequency = m_fNoiseFrequency;
  brush.m_iPriority = m_iPriority;
  brush.m_Tags = m_Tags;
  // Brush slots are reused, so leftovers from a previous spline brush have to be removed.
  brush.m_SplineNodes.Clear();
  brush.m_fSplineLength = 0.0f;
  brush.m_bSplineClosed = false;
  FillBrushSpecificProperties(brush, fHalfSizeX);
}

void ezTerrainBrushBaseComponent::RefreshBrushes()
{
  ClearBrushes();

  if (!IsActiveAndInitialized())
    return;

  auto* pSystem = GetWorld()->GetOrCreateModule<ezTerrainSystem>();

  const ezSplineComponent* pSpline = nullptr;
  if (GetOwner()->TryGetComponentOfBaseType(pSpline))
  {
    // Also catches a different spline component, which doesn't necessarily send a change message.
    if (m_hSplineCacheSource != pSpline->GetHandle())
    {
      UpdateSplineCache(*pSpline);
    }

    if (m_SplineCache.GetCount() < 2)
      return;

    const ezUInt32 uiIdx = pSystem->CreateBrushData();
    m_BrushIndices.PushBack(uiIdx);

    const ezTransform globalTransform = GetOwner()->GetGlobalTransform();

    ezTerrainData_Brush& brush = pSystem->ModifyBrushData(uiIdx);
    FillBrush(brush, globalTransform, 0.0f);
    brush.m_fSplineLength = m_SplineCache.PeekBack().m_fArcLength;
    brush.m_bSplineClosed = pSpline->GetClosed();

    // Same result as evaluating the spline in global space: the spline space is the owner's space.
    brush.m_SplineNodes.SetCount(m_SplineCache.GetCount());
    for (ezUInt32 i = 0; i < m_SplineCache.GetCount(); ++i)
    {
      const ezTerrainData_SplineNode& src = m_SplineCache[i];
      ezTerrainData_SplineNode& dst = brush.m_SplineNodes[i];

      dst.m_vPosition = globalTransform * src.m_vPosition;
      dst.m_vUpDir = globalTransform.m_qRotation * src.m_vUpDir;
      dst.m_fArcLength = src.m_fArcLength;
    }
  }
  else
  {
    const ezUInt32 uiIdx = pSystem->CreateBrushData();
    m_BrushIndices.PushBack(uiIdx);
    FillBrush(pSystem->ModifyBrushData(uiIdx), GetOwner()->GetGlobalTransform(), m_fHalfSizeX);
  }
}

void ezTerrainBrushBaseComponent::UpdateSplineCache(const ezSplineComponent& spline)
{
  m_SplineCache.Clear();
  m_hSplineCacheSource = spline.GetHandle();

  if (spline.GetTotalLength() <= 0.0f)
    return;

  // The spline component already tessellates itself with an adaptive error bound to build its
  // distance-to-key mapping: every entry is one point of that polyline, with the arc length as key.
  // Reusing it means straight parts get few nodes and curves as many as they need, without evaluating
  // the spline a second time. All of this is in spline space, so for a scaled object the distances are
  // scaled as well.
  const ezArrayMap<float, float>& distanceToKey = spline.GetDistanceToKeyRemapping();

  m_SplineCache.Reserve(distanceToKey.GetCount());

  for (ezUInt32 i = 0; i < distanceToKey.GetCount(); ++i)
  {
    const ezTransform trans = spline.GetTransformAtKey(distanceToKey.GetValue(i), ezSplineComponentSpace::Local);

    ezTerrainData_SplineNode& node = m_SplineCache.ExpandAndGetRef();
    node.m_vPosition = trans.m_vPosition;
    node.m_vUpDir = trans.m_qRotation * ezVec3::MakeAxisZ();
    node.m_fArcLength = distanceToKey.GetKey(i);
  }
}

void ezTerrainBrushBaseComponent::SetHalfSizeX(float fSize)
{
  if (m_fHalfSizeX == fSize)
    return;
  m_fHalfSizeX = fSize;
  RefreshBrushes();
}

void ezTerrainBrushBaseComponent::SetInnerRadius(float fRadius)
{
  if (m_fInnerRadius == fRadius)
    return;
  m_fInnerRadius = fRadius;
  RefreshBrushes();
}

void ezTerrainBrushBaseComponent::SetOuterRadius(float fRadius)
{
  if (m_fOuterRadius == fRadius)
    return;
  m_fOuterRadius = fRadius;
  RefreshBrushes();
}

void ezTerrainBrushBaseComponent::SetFalloff(float fFalloff)
{
  if (m_fFalloff == fFalloff)
    return;
  m_fFalloff = fFalloff;
  RefreshBrushes();
}

void ezTerrainBrushBaseComponent::SetMaterialIndex(ezUInt8 uiIndex)
{
  if (m_uiMaterialIndex == uiIndex)
    return;
  m_uiMaterialIndex = uiIndex;
  RefreshBrushes();
}

void ezTerrainBrushBaseComponent::SetMaterialStrength(float fStrength)
{
  if (m_fMaterialStrength == fStrength)
    return;
  m_fMaterialStrength = fStrength;
  RefreshBrushes();
}

void ezTerrainBrushBaseComponent::SetAffectPatches(bool b)
{
  if (m_bAffectPatches == b)
    return;
  m_bAffectPatches = b;
  RefreshBrushes();
}

void ezTerrainBrushBaseComponent::SetAffectVolumes(bool b)
{
  if (m_bAffectVolumes == b)
    return;
  m_bAffectVolumes = b;
  RefreshBrushes();
}

void ezTerrainBrushBaseComponent::SetNoiseStrength(float fNoise)
{
  if (m_fNoiseStrength == fNoise)
    return;
  m_fNoiseStrength = fNoise;
  RefreshBrushes();
}

void ezTerrainBrushBaseComponent::SetNoiseFrequency(float fNoise)
{
  if (m_fNoiseFrequency == fNoise)
    return;
  m_fNoiseFrequency = fNoise;
  RefreshBrushes();
}

void ezTerrainBrushBaseComponent::SetPriority(ezInt8 iPriority)
{
  if (m_iPriority == iPriority)
    return;
  m_iPriority = iPriority;
  RefreshBrushes();
}

void ezTerrainBrushBaseComponent::Reflection_SetTag(const char* szTagName)
{
  if (ezStringUtils::IsNullOrEmpty(szTagName))
    return;
  const ezTag& tag = ezTagRegistry::GetGlobalRegistry().RegisterTag(szTagName);
  if (m_Tags.IsSet(tag))
    return;
  m_Tags.Set(tag);
  RefreshBrushes();
}

void ezTerrainBrushBaseComponent::Reflection_RemoveTag(const char* szTagName)
{
  if (ezStringUtils::IsNullOrEmpty(szTagName))
    return;
  if (const ezTag* pTag = ezTagRegistry::GetGlobalRegistry().GetTagByName(ezTempHashedString(szTagName)))
  {
    if (!m_Tags.IsSet(*pTag))
      return;
    m_Tags.Remove(*pTag);
    RefreshBrushes();
  }
}
