#include <RendererCore/RendererCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Lights/Implementation/ShadowPool.h>
#include <RendererCore/Lights/PointLightComponent.h>
#include <RendererCore/Pipeline/RenderDataManager.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererFoundation/Shader/ShaderUtils.h>

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
extern ezCVarBool cvar_RenderingLightingVisScreenSpaceSize;
#endif

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPointLightRenderData, 1, ezRTTIDefaultAllocator<ezPointLightRenderData>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_COMPONENT_TYPE(ezPointLightComponent, 4, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Length", GetLength, SetLength)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant()), new ezSuffixAttribute(" m")),
    EZ_ACCESSOR_PROPERTY("Radius", GetRadius, SetRadius)->AddAttributes(new ezClampValueAttribute(0.0f, 0.5f), new ezSuffixAttribute(" m")),
    EZ_ACCESSOR_PROPERTY("Range", GetRange, SetRange)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant()), new ezSuffixAttribute(" m"), new ezMinValueTextAttribute("Auto")),
    EZ_ACCESSOR_PROPERTY("ShadowFadeOutRange", GetShadowFadeOutRange, SetShadowFadeOutRange)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant()), new ezSuffixAttribute(" m"), new ezMinValueTextAttribute("Auto")),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgExtractRenderData, OnMsgExtractRenderData),
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezSphereManipulatorAttribute("Range"),
    new ezPointLightVisualizerAttribute("Length", "Radius", "Range", "Intensity", "LightColor"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezPointLightComponent::ezPointLightComponent() = default;
ezPointLightComponent::~ezPointLightComponent() = default;

ezResult ezPointLightComponent::GetLocalBounds(ezBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, ezMsgUpdateLocalBounds& ref_msg)
{
  m_fEffectiveRange = CalculateEffectiveRange(m_fRange, m_fIntensity);

  const float fBoundingRadius = m_fEffectiveRange + m_fLength * 0.5f;
  ref_bounds = ezBoundingSphere::MakeFromCenterAndRadius(ezVec3::MakeZero(), fBoundingRadius);
  return EZ_SUCCESS;
}

void ezPointLightComponent::SetRange(float fRange)
{
  m_fRange = ezMath::Max(fRange, 0.0f);

  TriggerLocalBoundsUpdate();
}

float ezPointLightComponent::GetRange() const
{
  return m_fRange;
}

float ezPointLightComponent::GetEffectiveRange() const
{
  return m_fEffectiveRange;
}

void ezPointLightComponent::SetLength(float fLength)
{
  m_fLength = ezMath::Max(fLength, 0.0f);
  TriggerLocalBoundsUpdate();
}

float ezPointLightComponent::GetLength() const
{
  return m_fLength;
}

void ezPointLightComponent::SetRadius(float fRadius)
{
  m_fRadius = ezMath::Max(fRadius, 0.0f);
  InvalidateCachedRenderData();
}

float ezPointLightComponent::GetRadius() const
{
  return m_fRadius;
}

void ezPointLightComponent::SetShadowFadeOutRange(float fRange)
{
  m_fShadowFadeOutRange = ezMath::Max(fRange, 0.0f);

  InvalidateCachedRenderData();
}

float ezPointLightComponent::GetShadowFadeOutRange() const
{
  return m_fShadowFadeOutRange;
}

void ezPointLightComponent::OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const
{
  // Don't extract light render data for selection or in shadow views.
  if (msg.m_OverrideCategory != ezInvalidRenderDataCategory || msg.m_pView->GetCameraUsageHint() == ezCameraUsageHint::Shadow)
    return;

  if (m_fIntensity <= 0.0f || m_fEffectiveRange <= 0.0f)
    return;

  const ezTransform t = GetOwner()->GetGlobalTransform();
  const bool bIsTubeLight = (m_fLength > 0.0f || m_fRadius > 0.0f);
  // Clamp to minimum to avoid degenerate TubeLightShading shader math (division by zero when halfLength=0)
  const float fEffectiveLength = bIsTubeLight ? ezMath::Max(m_fLength, 0.001f) : 0.0f;
  const float fEffectiveRadius = bIsTubeLight ? ezMath::Max(m_fRadius, 0.001f) : 0.0f;
  const float fBoundingRadius = m_fEffectiveRange + fEffectiveLength * 0.5f;

  const ezBoundingSphere bounds = ezBoundingSphere::MakeFromCenterAndRadius(t.m_vPosition, fBoundingRadius);
  const float fScreenSpaceSize = CalculateScreenSpaceSize(bounds, *msg.m_pView->GetCullingCamera());
  float fShadowScreenSize = 0.0f;
  const float fShadowFadeOut = CalculateShadowFadeOut(bounds, m_fShadowFadeOutRange, *msg.m_pView->GetCullingCamera(), fShadowScreenSize);

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  VisualizeScreenSpaceSize(msg.m_pView->GetHandle(), bounds, fScreenSpaceSize, fShadowScreenSize, fShadowFadeOut);
#endif

  auto pRenderData = msg.m_pRenderDataManager->CreateRenderDataForThisFrame<ezPointLightRenderData>(GetOwner());

  pRenderData->m_LightColor = GetEffectiveColor();
  pRenderData->m_fIntensity = m_fIntensity;
  pRenderData->m_fSpecularMultiplier = m_fSpecularMultiplier;
  pRenderData->m_fRange = m_fEffectiveRange;
  pRenderData->m_qGlobalRotation = t.m_qRotation;
  pRenderData->m_fLength = fEffectiveLength;
  pRenderData->m_fRadius = fEffectiveRadius;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  if (cvar_RenderingLightingVisScreenSpaceSize)
  {
    VisualizeScreenSpaceSize(msg.m_pView->GetHandle(), bounds, fScreenSpaceSize, fShadowScreenSize, fShadowFadeOut);

    ezMat4 capsuleMat = ezMat4::MakeTranslation(t.m_vPosition);
    ezMat3 rotXtoZ;
    rotXtoZ.SetColumn(0, ezVec3(0, 0, 1));
    rotXtoZ.SetColumn(1, ezVec3(0, 1, 0));
    rotXtoZ.SetColumn(2, ezVec3(-1, 0, 0));
    capsuleMat.SetRotationalPart(t.m_qRotation.GetAsMat3() * rotXtoZ);
    ezDebugRenderer::DrawLineCapsuleZ(msg.m_pView->GetHandle(), fEffectiveLength, fEffectiveRadius, ezColorScheme::LightUI(ezColorScheme::Yellow), capsuleMat);
  }
#endif

  if (m_bCastShadows && fShadowFadeOut > 0.0f)
  {
    pRenderData->FillShadowDataOffsetAndFadeOut(ezShadowPool::AddPointLight(this, fScreenSpaceSize, msg.m_pView), fShadowFadeOut);
  }
  else
  {
    pRenderData->m_uiShadowDataOffsetAndFadeOut = 0;
  }

  pRenderData->FillSortingKey(fScreenSpaceSize);

  ezRenderData::Caching::Enum caching = m_bCastShadows ? ezRenderData::Caching::Never : ezRenderData::Caching::IfStatic;
#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  if (cvar_RenderingLightingVisScreenSpaceSize)
    caching = ezRenderData::Caching::Never;
#endif
  msg.AddRenderData(pRenderData, ezDefaultRenderDataCategories::Light, caching);
}

void ezPointLightComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  ezStreamWriter& s = inout_stream.GetStream();

  ezTextureCubeResourceHandle m_hProjectedTexture;

  s << m_fRange;
  s << m_fShadowFadeOutRange;
  s << m_hProjectedTexture;
  s << m_fLength;
  s << m_fRadius;
}

void ezPointLightComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = inout_stream.GetStream();

  ezTextureCubeResourceHandle m_hProjectedTexture;

  s >> m_fRange;
  if (uiVersion >= 3)
  {
    s >> m_fShadowFadeOutRange;
  }
  s >> m_hProjectedTexture;
  if (uiVersion >= 4)
  {
    s >> m_fLength;
    s >> m_fRadius;
  }
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPointLightVisualizerAttribute, 1, ezRTTIDefaultAllocator<ezPointLightVisualizerAttribute>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezPointLightVisualizerAttribute::ezPointLightVisualizerAttribute()
  : ezVisualizerAttribute(nullptr)
{
}

ezPointLightVisualizerAttribute::ezPointLightVisualizerAttribute(
  const char* szLengthProperty, const char* szRadiusProperty, const char* szRangeProperty, const char* szIntensityProperty, const char* szColorProperty)
  : ezVisualizerAttribute(szLengthProperty, szRadiusProperty, szRangeProperty, szIntensityProperty, szColorProperty)
{
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>

class ezPointLightComponentPatch_1_2 : public ezGraphPatch
{
public:
  ezPointLightComponentPatch_1_2()
    : ezGraphPatch("ezPointLightComponent", 2)
  {
  }

  virtual void Patch(ezGraphPatchContext& ref_context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    ref_context.PatchBaseClass("ezLightComponent", 2, true);
  }
};

ezPointLightComponentPatch_1_2 g_ezPointLightComponentPatch_1_2;

EZ_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_PointLightComponent);
