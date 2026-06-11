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
    const float fTotalLength = pSpline->GetTotalLength();
    if (fTotalLength <= 0.0f)
      return;

    constexpr float fMaxStep = 10.0f;
    constexpr float fMinStep = 0.5f;
    constexpr float fTolerance = 0.1f;

    auto PlaceBrush = [&](float d0, float d1)
    {
      const float dMid = (d0 + d1) * 0.5f;
      const float fSegmentHalfLength = (d1 - d0) * 0.5f;
      const ezTransform trans = pSpline->GetTransformAtDistance(dMid, ezSplineComponentSpace::Global);

      const ezUInt32 uiIdx = pSystem->CreateBrushData();
      m_BrushIndices.PushBack(uiIdx);
      FillBrush(pSystem->ModifyBrushData(uiIdx), trans, fSegmentHalfLength);
    };

    auto Subdivide = [&](auto& self, float d0, float d1) -> void
    {
      const float fLength = d1 - d0;
      const float dMid = (d0 + d1) * 0.5f;

      if (fLength <= fMinStep)
      {
        PlaceBrush(d0, d1);
        return;
      }

      if (fLength <= fMaxStep)
      {
        const ezVec3 p0 = pSpline->GetTransformAtDistance(d0, ezSplineComponentSpace::Global).m_vPosition;
        const ezVec3 p1 = pSpline->GetTransformAtDistance(d1, ezSplineComponentSpace::Global).m_vPosition;
        const ezVec3 pMidActual = pSpline->GetTransformAtDistance(dMid, ezSplineComponentSpace::Global).m_vPosition;

        if ((pMidActual - (p0 + p1) * 0.5f).GetLength() <= fTolerance)
        {
          PlaceBrush(d0, d1);
          return;
        }
      }

      self(self, d0, dMid);
      self(self, dMid, d1);
    };

    Subdivide(Subdivide, 0.0f, fTotalLength);
  }
  else
  {
    const ezUInt32 uiIdx = pSystem->CreateBrushData();
    m_BrushIndices.PushBack(uiIdx);
    FillBrush(pSystem->ModifyBrushData(uiIdx), GetOwner()->GetGlobalTransform(), m_fHalfSizeX);
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
