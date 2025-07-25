#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <Core/Messages/SetColorMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Configuration/CVar.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Lights/LightComponent.h>

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
ezCVarBool cvar_RenderingLightingVisScreenSpaceSize("Rendering.Lighting.VisScreenSpaceSize", false, ezCVarFlags::Default, "Enables debug visualization of light screen space size calculation");
#endif

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLightRenderData, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void ezLightRenderData::FillBatchIdAndSortingKey(float fScreenSpaceSize)
{
  m_uiSortingKey = (m_uiShadowDataOffsetAndFadeOut != 0) ? 0 : 1;
}

void ezLightRenderData::FillShadowDataOffsetAndFadeOut(ezUInt32 uiDataOffset, float fFadeOut)
{
  ezUInt32 uiFadeOut = ezMath::ColorFloatToUnsignedInt<12>(fFadeOut);
  m_uiShadowDataOffsetAndFadeOut = uiDataOffset | (uiFadeOut << 20);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_ABSTRACT_COMPONENT_TYPE(ezLightComponent, 6)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY_READ_ONLY("EffectiveColor", GetEffectiveColor)->AddAttributes(new ezHiddenAttribute),
    EZ_ACCESSOR_PROPERTY("UseColorTemperature", GetUsingColorTemperature, SetUsingColorTemperature),    
    EZ_ACCESSOR_PROPERTY("LightColor", GetLightColor, SetLightColor),
    EZ_ACCESSOR_PROPERTY("Temperature", GetTemperature, SetTemperature)->AddAttributes(new ezImageSliderUiAttribute("LightTemperature"), new ezDefaultValueAttribute(6550), new ezClampValueAttribute(1000, 15000)),
    EZ_ACCESSOR_PROPERTY("Intensity", GetIntensity, SetIntensity)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant()), new ezDefaultValueAttribute(10.0f)),
    EZ_ACCESSOR_PROPERTY("SpecularMultiplier", GetSpecularMultiplier, SetSpecularMultiplier)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant()), new ezDefaultValueAttribute(1.0f)),
    EZ_ACCESSOR_PROPERTY("CastShadows", GetCastShadows, SetCastShadows),
    EZ_ACCESSOR_PROPERTY("TransparentShadows", GetTransparentShadows, SetTransparentShadows),
    EZ_ACCESSOR_PROPERTY("PenumbraSize", GetPenumbraSize, SetPenumbraSize)->AddAttributes(new ezClampValueAttribute(0.0f, 0.5f), new ezDefaultValueAttribute(0.05f), new ezSuffixAttribute(" m")),
    EZ_ACCESSOR_PROPERTY("SlopeBias", GetSlopeBias, SetSlopeBias)->AddAttributes(new ezClampValueAttribute(0.0f, 10.0f), new ezDefaultValueAttribute(0.25f)),
    EZ_ACCESSOR_PROPERTY("ConstantBias", GetConstantBias, SetConstantBias)->AddAttributes(new ezClampValueAttribute(0.0f, 10.0f), new ezDefaultValueAttribute(0.1f))
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Lighting"),
  }
  EZ_END_ATTRIBUTES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgSetColor, OnMsgSetColor),
  }
  EZ_END_MESSAGEHANDLERS;
}
EZ_END_ABSTRACT_COMPONENT_TYPE
// clang-format on

ezLightComponent::ezLightComponent() = default;
ezLightComponent::~ezLightComponent() = default;

void ezLightComponent::SetUsingColorTemperature(bool bUseColorTemperature)
{
  m_bUseColorTemperature = bUseColorTemperature;

  InvalidateCachedRenderData();
}

bool ezLightComponent::GetUsingColorTemperature() const
{
  return m_bUseColorTemperature;
}

void ezLightComponent::SetTemperature(ezUInt32 uiTemperature)
{
  m_uiTemperature = ezMath::Clamp(uiTemperature, 1500u, 40000u);

  InvalidateCachedRenderData();
}

ezUInt32 ezLightComponent::GetTemperature() const
{
  return m_uiTemperature;
}

void ezLightComponent::SetLightColor(ezColorGammaUB lightColor)
{
  m_LightColor = lightColor;

  InvalidateCachedRenderData();
}

ezColorGammaUB ezLightComponent::GetLightColor() const
{
  return m_LightColor;
}

ezColorGammaUB ezLightComponent::GetEffectiveColor() const
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

void ezLightComponent::SetIntensity(float fIntensity)
{
  m_fIntensity = ezMath::Max(fIntensity, 0.0f);

  TriggerLocalBoundsUpdate();
}

float ezLightComponent::GetIntensity() const
{
  return m_fIntensity;
}

void ezLightComponent::SetSpecularMultiplier(float fSpecularMultiplier)
{
  m_fSpecularMultiplier = ezMath::Max(fSpecularMultiplier, 0.0f);

  InvalidateCachedRenderData();
}

float ezLightComponent::GetSpecularMultiplier() const
{
  return m_fSpecularMultiplier;
}

void ezLightComponent::SetCastShadows(bool bCastShadows)
{
  m_bCastShadows = bCastShadows;

  InvalidateCachedRenderData();
}

bool ezLightComponent::GetCastShadows() const
{
  return m_bCastShadows;
}

void ezLightComponent::SetTransparentShadows(bool bShadows)
{
  m_bTransparentShadows = bShadows;

  InvalidateCachedRenderData();
}

bool ezLightComponent::GetTransparentShadows() const
{
  return m_bTransparentShadows;
}

void ezLightComponent::SetPenumbraSize(float fPenumbraSize)
{
  m_fPenumbraSize = fPenumbraSize;

  InvalidateCachedRenderData();
}

float ezLightComponent::GetPenumbraSize() const
{
  return m_fPenumbraSize;
}

void ezLightComponent::SetSlopeBias(float fBias)
{
  m_fSlopeBias = fBias;

  InvalidateCachedRenderData();
}

float ezLightComponent::GetSlopeBias() const
{
  return m_fSlopeBias;
}

void ezLightComponent::SetConstantBias(float fBias)
{
  m_fConstantBias = fBias;

  InvalidateCachedRenderData();
}

float ezLightComponent::GetConstantBias() const
{
  return m_fConstantBias;
}

void ezLightComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  ezStreamWriter& s = inout_stream.GetStream();

  s << m_LightColor;
  s << m_fIntensity;
  s << m_fPenumbraSize;
  s << m_fSlopeBias;
  s << m_fConstantBias;
  s << m_bCastShadows;
  s << m_bTransparentShadows;
  s << m_bUseColorTemperature;
  s << m_uiTemperature;
  s << m_fSpecularMultiplier;
}

void ezLightComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  ezStreamReader& s = inout_stream.GetStream();

  s >> m_LightColor;
  s >> m_fIntensity;

  if (uiVersion >= 3)
  {
    s >> m_fPenumbraSize;
  }

  if (uiVersion >= 4)
  {
    s >> m_fSlopeBias;
    s >> m_fConstantBias;
  }

  s >> m_bCastShadows;

  if (uiVersion >= 6)
  {
    s >> m_bTransparentShadows;
  }

  if (uiVersion >= 5)
  {
    s >> m_bUseColorTemperature;
    s >> m_uiTemperature;
    s >> m_fSpecularMultiplier;
  }
}

void ezLightComponent::OnMsgSetColor(ezMsgSetColor& ref_msg)
{
  ref_msg.ModifyColor(m_LightColor);

  InvalidateCachedRenderData();
}

// static
float ezLightComponent::CalculateEffectiveRange(float fRange, float fIntensity)
{
  const float fThreshold = 0.10f; // aggressive threshold to prevent large lights
  const float fEffectiveRange = ezMath::Sqrt(ezMath::Max(0.0f, fIntensity)) / ezMath::Sqrt(fThreshold);

  EZ_ASSERT_DEBUG(!ezMath::IsNaN(fEffectiveRange), "Light range is NaN");

  if (fRange <= 0.0f)
  {
    return fEffectiveRange;
  }

  return ezMath::Min(fRange, fEffectiveRange);
}

// static
float ezLightComponent::CalculateScreenSpaceSize(const ezBoundingSphere& sphere, const ezCamera& camera)
{
  if (camera.IsPerspective())
  {
    float dist = (sphere.m_vCenter - camera.GetPosition()).GetLength();
    float fHalfHeight = ezMath::Tan(camera.GetFovY(1.0f) * 0.5f) * dist;
    return sphere.m_fRadius / fHalfHeight;
  }
  else
  {
    float fHalfHeight = camera.GetDimensionY(1.0f) * 0.5f;
    return sphere.m_fRadius / fHalfHeight;
  }
}

float ezLightComponent::CalculateShadowFadeOut(const ezBoundingSphere& sphere, float fShadowFadeOutRange, const ezCamera& camera, float& out_fShadowScreenSize) const
{
  if (!m_bCastShadows)
    return 0.0f;

  ezBoundingSphere shadowBounds = sphere;
  if (fShadowFadeOutRange > 0.0f)
  {
    shadowBounds.m_fRadius = fShadowFadeOutRange;
  }
  out_fShadowScreenSize = CalculateScreenSpaceSize(shadowBounds, camera);
  return ezMath::Saturate(ezMath::Unlerp(0.8f, 1.0f, out_fShadowScreenSize));
}

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
void ezLightComponent::VisualizeScreenSpaceSize(ezViewHandle hView, const ezBoundingSphere& sphere, float fScreenSize, float fShadowScreenSize, float fShadowFadeOut) const
{
  if (cvar_RenderingLightingVisScreenSpaceSize)
  {
    ezColor c = ezColorScheme::LightUI(ezColorScheme::Cyan);
    if (m_bCastShadows)
    {
      ezDebugRenderer::Draw3DText(hView,
        ezFmt("ScreenSize: {}\nShadowScreenSize: {}\n ShadowFadeOut: {}", ezArgF(fScreenSize, 3), ezArgF(fShadowScreenSize, 3), ezArgF(fShadowFadeOut, 3)), sphere.m_vCenter, c);
    }
    else
    {
      ezDebugRenderer::Draw3DText(hView, ezFmt("ScreenSize: {}", ezArgF(fScreenSize, 3)), sphere.m_vCenter, c);
    }
    ezDebugRenderer::DrawLineSphere(hView, sphere, c);
  }
}
#endif

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>

class ezLightComponentPatch_1_2 : public ezGraphPatch
{
public:
  ezLightComponentPatch_1_2()
    : ezGraphPatch("ezLightComponent", 2)
  {
  }

  virtual void Patch(ezGraphPatchContext& ref_context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override { pNode->RenameProperty("Light Color", "LightColor"); }
};

ezLightComponentPatch_1_2 g_ezLightComponentPatch_1_2;



EZ_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_LightComponent);
