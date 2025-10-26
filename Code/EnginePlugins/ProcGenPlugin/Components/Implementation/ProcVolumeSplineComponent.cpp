#include <ProcGenPlugin/ProcGenPluginPCH.h>

#include <Core/Messages/TransformChangedMessage.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <ProcGenPlugin/Components/ProcVolumeSplineComponent.h>
#include <ProcGenPlugin/Components/VolumeCollection.h>
#include <RendererCore/Components/SplineComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/RenderData.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezProcVolumeSplineComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Radius", GetRadius, SetRadius)->AddAttributes(new ezDefaultValueAttribute(5.0f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_ACCESSOR_PROPERTY("Falloff", GetFalloff, SetFalloff)->AddAttributes(new ezDefaultValueAttribute(0.5f), new ezClampValueAttribute(0.0f, 1.0f)),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgSplineChanged, OnMsgSplineChanged),
    EZ_MESSAGE_HANDLER(ezMsgUpdateLocalBounds, OnMsgUpdateLocalBounds),
    EZ_MESSAGE_HANDLER(ezMsgExtractVolumes, OnMsgExtractVolumes),
    EZ_MESSAGE_HANDLER(ezMsgExtractRenderData, OnMsgExtractRenderData),
  }
  EZ_END_MESSAGEHANDLERS;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezProcVolumeSplineComponent::ezProcVolumeSplineComponent() = default;
ezProcVolumeSplineComponent::~ezProcVolumeSplineComponent() = default;

void ezProcVolumeSplineComponent::SetRadius(float fRadius)
{
  if (m_fRadius != fRadius)
  {
    m_fRadius = fRadius;

    if (IsActiveAndInitialized())
    {
      GetOwner()->UpdateLocalBounds();
    }

    InvalidateArea();

    m_DebugLines.Clear();
  }
}

void ezProcVolumeSplineComponent::SetFalloff(float fFalloff)
{
  if (m_fFalloff != fFalloff)
  {
    m_fFalloff = fFalloff;

    InvalidateArea();
  }
}

void ezProcVolumeSplineComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  ezStreamWriter& s = inout_stream.GetStream();

  s << m_fRadius;
  s << m_fFalloff;
}

void ezProcVolumeSplineComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = inout_stream.GetStream();

  s >> m_fRadius;
  s >> m_fFalloff;
}

void ezProcVolumeSplineComponent::OnMsgSplineChanged(ezMsgSplineChanged& ref_msg)
{
  if (ref_msg.m_uiChangeCounter == ezInvalidIndex || ref_msg.m_uiChangeCounter == m_uiLastChangeCounter)
    return;

  m_uiLastChangeCounter = ref_msg.m_uiChangeCounter;

  GetOwner()->UpdateLocalBounds();

  InvalidateArea();

  m_DebugLines.Clear();
}

void ezProcVolumeSplineComponent::OnMsgUpdateLocalBounds(ezMsgUpdateLocalBounds& ref_msg) const
{
  const ezSplineComponent* pSplineComponent = nullptr;
  if (!GetOwner()->TryGetComponentOfBaseType(pSplineComponent))
    return;

  ezSimdBBoxSphere bounds;
  if (pSplineComponent->GetSpline().CalculateBounds(bounds).Failed())
    return;

  bounds.m_BoxHalfExtents += ezSimdVec4f(m_fRadius);
  bounds.m_CenterAndRadius.SetW(bounds.m_CenterAndRadius.w() + ezSimdFloat(m_fRadius));

  ref_msg.AddBounds(ezSimdConversion::ToBBoxSphere(bounds), s_SpatialCategory);
}

void ezProcVolumeSplineComponent::OnMsgExtractVolumes(ezMsgExtractVolumes& ref_msg) const
{
  const ezSplineComponent* pSplineComponent = nullptr;
  if (!GetOwner()->TryGetComponentOfBaseType(pSplineComponent))
    return;

  if (pSplineComponent->GetSpline().m_ControlPoints.GetCount() < 2)
    return;

  ref_msg.m_pCollection->AddSpline(GetOwner()->GetGlobalTransformSimd(), pSplineComponent->GetSpline(), m_fRadius, m_BlendMode, m_fSortOrder, m_fValue, m_fFalloff);
}

void ezProcVolumeSplineComponent::OnMsgExtractRenderData(ezMsgExtractRenderData& ref_msg) const
{
  if (ref_msg.m_OverrideCategory != ezDefaultRenderDataCategories::Selection)
    return;

  const ezSplineComponent* pSplineComponent = nullptr;
  if (!GetOwner()->TryGetComponentOfBaseType(pSplineComponent))
    return;

  auto& spline = pSplineComponent->GetSpline();
  if (spline.GetNumControlPoints() < 2)
    return;

  if (m_DebugLines.IsEmpty())
  {
    constexpr ezUInt32 uiSteps = 8;
    constexpr ezUInt32 uiRingSteps = 16;
    constexpr ezAngle ringStepAngle = ezAngle::MakeFromDegree(360.0f / (float)uiRingSteps);
    const ezUInt32 uiNumSegments = spline.GetNumSegments();

    const ezUInt32 uiNumRings = uiNumSegments * 2 + (spline.m_bClosed ? 0 : 3);
    const ezUInt32 uiNumLines = uiNumSegments * uiSteps * 4 + uiNumRings * uiRingSteps;
    m_DebugLines.Reserve(uiNumLines);

    auto AddRing = [&](const ezSimdTransform& t, ezBasisAxis::Enum rightAxis, ezBasisAxis::Enum upAxis, ezUInt32 uiNumSteps)
    {
      const ezSimdVec4f vRight = ezSimdConversion::ToVec3(ezBasisAxis::GetBasisVector(rightAxis)) * m_fRadius;
      const ezSimdVec4f vUp = ezSimdConversion::ToVec3(ezBasisAxis::GetBasisVector(upAxis)) * m_fRadius;

      for (ezUInt32 s = 0; s < uiNumSteps; ++s)
      {
        const float fS1 = static_cast<float>(s);
        const float fS2 = static_cast<float>(s + 1);

        const float fCos1 = ezMath::Cos(fS1 * ringStepAngle);
        const float fCos2 = ezMath::Cos(fS2 * ringStepAngle);

        const float fSin1 = ezMath::Sin(fS1 * ringStepAngle);
        const float fSin2 = ezMath::Sin(fS2 * ringStepAngle);

        auto& line = m_DebugLines.ExpandAndGetRef();
        line.m_start = ezSimdConversion::ToVec3(t.TransformPosition(vRight * fSin1 + vUp * fCos1));
        line.m_end = ezSimdConversion::ToVec3(t.TransformPosition(vRight * fSin2 + vUp * fCos2));
      }
    };

    const ezSimdVec4f vOffsets[] = {
      ezSimdVec4f(0, m_fRadius, 0),
      ezSimdVec4f(0, -m_fRadius, 0),
      ezSimdVec4f(0, 0, m_fRadius),
      ezSimdVec4f(0, 0, -m_fRadius),
    };

    const float fStep = 1.0f / static_cast<float>(uiSteps);
    for (ezUInt32 i = 0; i < uiNumSegments; ++i)
    {
      ezSimdTransform t0 = spline.EvaluateTransform(static_cast<float>(i));

      AddRing(t0, ezBasisAxis::PositiveY, ezBasisAxis::PositiveZ, uiRingSteps);
      if (i == 0 && !spline.m_bClosed)
      {
        AddRing(t0, ezBasisAxis::NegativeX, ezBasisAxis::PositiveY, uiRingSteps / 2);
        AddRing(t0, ezBasisAxis::NegativeX, ezBasisAxis::PositiveZ, uiRingSteps / 2);
      }

      for (ezUInt32 uiStep = 1; uiStep <= uiSteps; ++uiStep)
      {
        const float fT = fStep * static_cast<float>(uiStep);
        const ezSimdTransform t1 = spline.EvaluateTransform(fT + i);
        for (const auto& offset : vOffsets)
        {
          auto& line = m_DebugLines.ExpandAndGetRef();
          line.m_start = ezSimdConversion::ToVec3(t0.TransformPosition(offset));
          line.m_end = ezSimdConversion::ToVec3(t1.TransformPosition(offset));
        }

        if (uiStep == uiSteps / 2)
        {
          AddRing(t1, ezBasisAxis::PositiveY, ezBasisAxis::PositiveZ, uiRingSteps);
        }

        t0 = t1;
      }

      if (i == uiNumSegments - 1 && !spline.m_bClosed)
      {
        AddRing(t0, ezBasisAxis::PositiveX, ezBasisAxis::PositiveY, uiRingSteps / 2);
        AddRing(t0, ezBasisAxis::PositiveX, ezBasisAxis::PositiveZ, uiRingSteps / 2);
        AddRing(t0, ezBasisAxis::PositiveY, ezBasisAxis::PositiveZ, uiRingSteps);
      }
    }

    EZ_ASSERT_DEBUG(m_DebugLines.GetCount() == uiNumLines, "Implementation error");
  }

  ezColor c = ezColorScheme::GetCategoryColor("Construction", ezColorScheme::CategoryColorUsage::ViewportIcon);
  ezDebugRenderer::DrawLines(GetWorld(), m_DebugLines, c, GetOwner()->GetGlobalTransform());
}


EZ_STATICLINK_FILE(ProcGenPlugin, ProcGenPlugin_Components_Implementation_ProcVolumeSplineComponent);
