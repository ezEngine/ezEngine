#include <RendererCore/RendererCorePCH.h>

#include <Core/Messages/SetColorMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Lights/FillLightComponent.h>
#include <RendererCore/Lights/LightComponent.h>
#include <RendererCore/Pipeline/View.h>

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
extern ezCVarBool cvar_RenderingLightingVisScreenSpaceSize;
#endif

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezFillLightMode, 1)
  EZ_ENUM_CONSTANTS(ezFillLightMode::Additive, ezFillLightMode::Subtractive, ezFillLightMode::ModulateIndirect)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezFillLightRenderData, 1, ezRTTIDefaultAllocator<ezFillLightRenderData>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

void ezFillLightRenderData::FillBatchIdAndSortingKey(float fScreenSpaceSize)
{
  m_uiSortingKey = 1;
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_COMPONENT_TYPE(ezFillLightComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY_READ_ONLY("EffectiveColor", GetEffectiveColor)->AddAttributes(new ezHiddenAttribute),
    EZ_ENUM_ACCESSOR_PROPERTY("LightMode", ezFillLightMode, GetLightMode, SetLightMode),
    EZ_ACCESSOR_PROPERTY("UseColorTemperature", GetUsingColorTemperature, SetUsingColorTemperature),
    EZ_ACCESSOR_PROPERTY("LightColor", GetLightColor, SetLightColor),
    EZ_ACCESSOR_PROPERTY("Temperature", GetTemperature, SetTemperature)->AddAttributes(new ezImageSliderUiAttribute("LightTemperature"), new ezDefaultValueAttribute(6550), new ezClampValueAttribute(1000, 15000)),
    EZ_ACCESSOR_PROPERTY("Intensity", GetIntensity, SetIntensity)->AddAttributes(new ezDefaultValueAttribute(10.0f), new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_ACCESSOR_PROPERTY("Range", GetRange, SetRange)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant()), new ezDefaultValueAttribute(5.0f), new ezSuffixAttribute(" m")),
    EZ_ACCESSOR_PROPERTY("FalloffExponent", GetFalloffExponent, SetFalloffExponent)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant()), new ezDefaultValueAttribute(1.0f)),
    EZ_ACCESSOR_PROPERTY("Directionality", GetDirectionality, SetDirectionality)->AddAttributes(new ezClampValueAttribute(0.0f, 1.0f), new ezDefaultValueAttribute(1.0f)),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgSetColor, OnMsgSetColor),
    EZ_MESSAGE_HANDLER(ezMsgExtractRenderData, OnMsgExtractRenderData),
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Lighting"),
    new ezSphereManipulatorAttribute("Range"),
    new ezSphereVisualizerAttribute("Range", ezColor::White, "LightColor"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezFillLightComponent::ezFillLightComponent() = default;
ezFillLightComponent::~ezFillLightComponent() = default;

void ezFillLightComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  ezStreamWriter& s = inout_stream.GetStream();

  s << m_LightColor;
  s << m_uiTemperature;
  s << m_fIntensity;
  s << m_fRange;
  s << m_fFalloffExponent;
  s << m_fDirectionality;
  s << m_LightMode;
  s << m_bUseColorTemperature;
}

void ezFillLightComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = inout_stream.GetStream();

  s >> m_LightColor;
  s >> m_uiTemperature;
  s >> m_fIntensity;
  s >> m_fRange;
  s >> m_fFalloffExponent;
  s >> m_fDirectionality;
  s >> m_LightMode;
  s >> m_bUseColorTemperature;
}

ezResult ezFillLightComponent::GetLocalBounds(ezBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, ezMsgUpdateLocalBounds& ref_msg)
{
  ref_bounds = ezBoundingSphere::MakeFromCenterAndRadius(ezVec3::MakeZero(), m_fRange);
  return EZ_SUCCESS;
}

void ezFillLightComponent::SetLightMode(ezEnum<ezFillLightMode> mode)
{
  m_LightMode = mode;

  InvalidateCachedRenderData();
}

void ezFillLightComponent::SetUsingColorTemperature(bool bUseColorTemperature)
{
  m_bUseColorTemperature = bUseColorTemperature;

  InvalidateCachedRenderData();
}

void ezFillLightComponent::SetTemperature(ezUInt32 uiTemperature)
{
  m_uiTemperature = ezMath::Clamp(uiTemperature, 1500u, 40000u);

  InvalidateCachedRenderData();
}

void ezFillLightComponent::SetLightColor(ezColorGammaUB lightColor)
{
  m_LightColor = lightColor;

  InvalidateCachedRenderData();
}

ezColorGammaUB ezFillLightComponent::GetEffectiveColor() const
{
  if (m_bUseColorTemperature)
  {
    return ezColor::MakeFromKelvin(m_uiTemperature);
  }
  else
  {
    return m_LightColor;
  }
}

void ezFillLightComponent::SetIntensity(float fIntensity)
{
  m_fIntensity = fIntensity;

  InvalidateCachedRenderData();
}

void ezFillLightComponent::SetRange(float fRange)
{
  m_fRange = ezMath::Max(fRange, 0.0f);

  TriggerLocalBoundsUpdate();
}

void ezFillLightComponent::SetFalloffExponent(float fFalloffExponent)
{
  m_fFalloffExponent = ezMath::Max(fFalloffExponent, 0.0f);

  InvalidateCachedRenderData();
}

void ezFillLightComponent::SetDirectionality(float fDirectionality)
{
  m_fDirectionality = ezMath::Saturate(fDirectionality);

  InvalidateCachedRenderData();
}

void ezFillLightComponent::OnMsgSetColor(ezMsgSetColor& ref_msg)
{
  ref_msg.ModifyColor(m_LightColor);

  InvalidateCachedRenderData();
}

void ezFillLightComponent::OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const
{
  // Don't extract light render data for selection or in shadow views.
  if (msg.m_OverrideCategory != ezInvalidRenderDataCategory || msg.m_pView->GetCameraUsageHint() == ezCameraUsageHint::Shadow)
    return;

  if ((m_LightMode == ezFillLightMode::Additive && ezMath::IsZero(m_fIntensity, ezMath::DefaultEpsilon<float>())) || m_fRange <= 0.0f)
    return;

  const ezTransform t = GetOwner()->GetGlobalTransform();
  const ezBoundingSphere bs = ezBoundingSphere::MakeFromCenterAndRadius(t.m_vPosition, m_fRange);

  const float fScreenSpaceSize = ezLightComponent::CalculateScreenSpaceSize(bs, *msg.m_pView->GetCullingCamera());

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  if (cvar_RenderingLightingVisScreenSpaceSize)
  {
    ezColor c = ezColorScheme::LightUI(ezColorScheme::Cyan);
    ezDebugRenderer::Draw3DText(msg.m_pView->GetHandle(), ezFmt("{0}", fScreenSpaceSize), t.m_vPosition, c);
    ezDebugRenderer::DrawLineSphere(msg.m_pView->GetHandle(), bs, c);
  }
#endif

  auto pRenderData = ezCreateRenderDataForThisFrame<ezFillLightRenderData>(GetOwner());

  pRenderData->m_GlobalTransform = t;
  pRenderData->m_LightColor = GetEffectiveColor();
  pRenderData->m_LightMode = m_LightMode;
  pRenderData->m_fIntensity = m_fIntensity;
  pRenderData->m_fRange = m_fRange;
  pRenderData->m_fFalloffExponent = m_fFalloffExponent;
  pRenderData->m_fDirectionality = m_fDirectionality;

  pRenderData->FillBatchIdAndSortingKey(fScreenSpaceSize);

  msg.AddRenderData(pRenderData, ezDefaultRenderDataCategories::Light, ezRenderData::Caching::IfStatic);
}


EZ_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_FillLightComponent);
