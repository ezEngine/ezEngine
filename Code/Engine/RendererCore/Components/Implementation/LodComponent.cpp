#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <Core/Messages/TriggerMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Math/BoundingSphere.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <RendererCore/Components/LodComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/View.h>

static float CalculateSphereScreenSpaceCoverage(const ezBoundingSphere& sphere, const ezCamera& camera)
{
  if (camera.IsPerspective())
  {
    return ezGraphicsUtils::CalculateSphereScreenCoverage(sphere, camera.GetCenterPosition(), camera.GetFovY(1.0f));
  }
  else
  {
    return ezGraphicsUtils::CalculateSphereScreenCoverage(sphere.m_fRadius, camera.GetDimensionY(1.0f));
  }
}

struct LodCompFlags
{
  enum Enum
  {
    ShowDebugInfo = 0,
    OverlapRanges = 1,
  };
};

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezLodComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("BoundsOffset", m_vBoundsOffset),
    EZ_MEMBER_PROPERTY("BoundsRadius", m_fBoundsRadius)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.01f, 100.0f)),
    EZ_ACCESSOR_PROPERTY("ShowDebugInfo", GetShowDebugInfo, SetShowDebugInfo),
    EZ_ACCESSOR_PROPERTY("OverlapRanges", GetOverlapRanges, SetOverlapRanges)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_ARRAY_MEMBER_PROPERTY("LodThresholds", m_LodThresholds)->AddAttributes(new ezMaxArraySizeAttribute(4), new ezClampValueAttribute(0.0f, 1.0f)),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Construction"),
    new ezSphereVisualizerAttribute("BoundsRadius", ezColor::MediumVioletRed, nullptr, ezVisualizerAnchor::Center, ezVec3(1.0f), "BoundsOffset"),
    new ezTransformManipulatorAttribute("BoundsOffset"),
  }
  EZ_END_ATTRIBUTES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgExtractRenderData, OnMsgExtractRenderData),
    EZ_MESSAGE_HANDLER(ezMsgComponentInternalTrigger, OnMsgComponentInternalTrigger),
  }
  EZ_END_MESSAGEHANDLERS;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

static const ezTempHashedString sLod0("LOD0");
static const ezTempHashedString sLod1("LOD1");
static const ezTempHashedString sLod2("LOD2");
static const ezTempHashedString sLod3("LOD3");
static const ezTempHashedString sLod4("LOD4");

ezLodComponent::ezLodComponent()
{
  SetOverlapRanges(true);
}

ezLodComponent::~ezLodComponent() = default;

void ezLodComponent::SetShowDebugInfo(bool bShow)
{
  SetUserFlag(LodCompFlags::ShowDebugInfo, bShow);
}

bool ezLodComponent::GetShowDebugInfo() const
{
  return GetUserFlag(LodCompFlags::ShowDebugInfo);
}

void ezLodComponent::SetOverlapRanges(bool bShow)
{
  SetUserFlag(LodCompFlags::OverlapRanges, bShow);
}

bool ezLodComponent::GetOverlapRanges() const
{
  return GetUserFlag(LodCompFlags::OverlapRanges);
}

void ezLodComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_vBoundsOffset;
  s << m_fBoundsRadius;

  s.WriteArray(m_LodThresholds).AssertSuccess();
}

void ezLodComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s >> m_vBoundsOffset;
  s >> m_fBoundsRadius;

  s.ReadArray(m_LodThresholds).AssertSuccess();
}

ezResult ezLodComponent::GetLocalBounds(ezBoundingBoxSphere& out_bounds, bool& out_bAlwaysVisible, ezMsgUpdateLocalBounds& ref_msg)
{
  out_bounds = ezBoundingSphere::MakeFromCenterAndRadius(m_vBoundsOffset, m_fBoundsRadius);
  out_bAlwaysVisible = false;
  return EZ_SUCCESS;
}

void ezLodComponent::OnActivated()
{
  SUPER::OnActivated();

  // start with the highest LOD (lowest detail)
  m_iCurLod = m_LodThresholds.GetCount();

  ezMsgComponentInternalTrigger trig;
  trig.m_iPayload = m_iCurLod;
  OnMsgComponentInternalTrigger(trig);
}

void ezLodComponent::OnDeactivated()
{
  // when the component gets deactivated, activate all LOD children
  // this is important for editing to not behave weirdly
  // not sure whether this can have unintended side-effects at runtime
  // but there this should only be called for objects that get deleted anyway

  ezGameObject* pLod[5];
  pLod[0] = GetOwner()->FindChildByName(sLod0);
  pLod[1] = GetOwner()->FindChildByName(sLod1);
  pLod[2] = GetOwner()->FindChildByName(sLod2);
  pLod[3] = GetOwner()->FindChildByName(sLod3);
  pLod[4] = GetOwner()->FindChildByName(sLod4);

  for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(pLod); ++i)
  {
    if (pLod[i])
    {
      pLod[i]->SetActiveFlag(true);
    }
  }

  SUPER::OnDeactivated();
}

void ezLodComponent::OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const
{
  if (msg.m_pView->GetCameraUsageHint() != ezCameraUsageHint::EditorView &&
      msg.m_pView->GetCameraUsageHint() != ezCameraUsageHint::MainView)
  {
    return;
  }

  // Don't extract render data for selection.
  if (msg.m_OverrideCategory != ezInvalidRenderDataCategory)
    return;

  const ezInt32 iNumLods = (ezInt32)m_LodThresholds.GetCount();

  const ezVec3 vScale = GetOwner()->GetGlobalScaling();
  const float fScale = ezMath::Max(vScale.x, vScale.y, vScale.z);
  const ezVec3 vCenter = GetOwner()->GetGlobalTransform() * m_vBoundsOffset;

  const float fCoverage = CalculateSphereScreenSpaceCoverage(ezBoundingSphere::MakeFromCenterAndRadius(vCenter, fScale * m_fBoundsRadius), *msg.m_pView->GetCullingCamera());

  // clamp the input value, this is to prevent issues while editing the threshold array
  ezInt32 iNewLod = ezMath::Clamp<ezInt32>(m_iCurLod, 0, iNumLods);

  float fCoverageP = 1;
  float fCoverageN = 0;

  if (iNewLod > 0)
  {
    fCoverageP = m_LodThresholds[iNewLod - 1];
  }

  if (iNewLod < iNumLods)
  {
    fCoverageN = m_LodThresholds[iNewLod];
  }

  if (GetOverlapRanges())
  {
    const float fLodRangeOverlap = 0.40f;

    if (iNewLod + 1 < iNumLods)
    {
      float range = (fCoverageN - m_LodThresholds[iNewLod + 1]);
      fCoverageN -= range * fLodRangeOverlap; // overlap into the next range
    }
    else
    {
      float range = (fCoverageN - 0.0f);
      fCoverageN -= range * fLodRangeOverlap; // overlap into the next range
    }
  }

  if (fCoverage < fCoverageN)
  {
    ++iNewLod;
  }
  else if (fCoverage > fCoverageP)
  {
    --iNewLod;
  }

  iNewLod = ezMath::Clamp(iNewLod, 0, iNumLods);

  if (GetShowDebugInfo())
  {
    ezStringBuilder sb;
    sb.SetFormat("Coverage: {}\nLOD {}\nRange: {} - {}", ezArgF(fCoverage, 3), iNewLod, ezArgF(fCoverageP, 3), ezArgF(fCoverageN, 3));
    ezDebugRenderer::Draw3DText(msg.m_pView->GetHandle(), sb, GetOwner()->GetGlobalPosition(), ezColor::White);
  }

  if (iNewLod == m_iCurLod)
    return;

  ezMsgComponentInternalTrigger trig;
  trig.m_iPayload = iNewLod;

  PostMessage(trig);
}

void ezLodComponent::OnMsgComponentInternalTrigger(ezMsgComponentInternalTrigger& msg)
{
  m_iCurLod = msg.m_iPayload;

  // search for direct children named LODn, don't waste performance searching recursively
  ezGameObject* pLod[5];
  pLod[0] = GetOwner()->FindChildByName(sLod0, false);
  pLod[1] = GetOwner()->FindChildByName(sLod1, false);
  pLod[2] = GetOwner()->FindChildByName(sLod2, false);
  pLod[3] = GetOwner()->FindChildByName(sLod3, false);
  pLod[4] = GetOwner()->FindChildByName(sLod4, false);

  // activate the selected LOD, deactivate all others
  for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(pLod); ++i)
  {
    if (pLod[i])
    {
      pLod[i]->SetActiveFlag(m_iCurLod == i);
    }
  }
}


EZ_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_LodComponent);
