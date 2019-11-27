#include <RendererCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Lights/LightComponent.h>
#include <RendererCore/Messages/SetColorMessage.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLightRenderData, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void ezLightRenderData::FillBatchIdAndSortingKey(float fScreenSpaceSize)
{
  m_uiSortingKey = (m_uiShadowDataOffset != ezInvalidIndex) ? 0 : 1;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_ABSTRACT_COMPONENT_TYPE(ezLightComponent, 4)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("LightColor", GetLightColor, SetLightColor),
    EZ_ACCESSOR_PROPERTY("Intensity", GetIntensity, SetIntensity)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant()), new ezDefaultValueAttribute(10.0f)),
    EZ_ACCESSOR_PROPERTY("CastShadows", GetCastShadows, SetCastShadows),
    EZ_ACCESSOR_PROPERTY("PenumbraSize", GetPenumbraSize, SetPenumbraSize)->AddAttributes(new ezClampValueAttribute(0.0f, 0.5f), new ezDefaultValueAttribute(0.1f), new ezSuffixAttribute(" m")),
    EZ_ACCESSOR_PROPERTY("SlopeBias", GetSlopeBias, SetSlopeBias)->AddAttributes(new ezClampValueAttribute(0.0f, 10.0f), new ezDefaultValueAttribute(0.25f)),
    EZ_ACCESSOR_PROPERTY("ConstantBias", GetConstantBias, SetConstantBias)->AddAttributes(new ezClampValueAttribute(0.0f, 10.0f), new ezDefaultValueAttribute(0.1f))
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Rendering/Lighting"),
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

void ezLightComponent::SetLightColor(ezColorGammaUB LightColor)
{
  m_LightColor = LightColor;
}

ezColorGammaUB ezLightComponent::GetLightColor() const
{
  return m_LightColor;
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

void ezLightComponent::SetCastShadows(bool bCastShadows)
{
  m_bCastShadows = bCastShadows;
}

bool ezLightComponent::GetCastShadows() const
{
  return m_bCastShadows;
}

void ezLightComponent::SetPenumbraSize(float fPenumbraSize)
{
  m_fPenumbraSize = fPenumbraSize;
}

float ezLightComponent::GetPenumbraSize() const
{
  return m_fPenumbraSize;
}

void ezLightComponent::SetSlopeBias(float fBias)
{
  m_fSlopeBias = fBias;
}

float ezLightComponent::GetSlopeBias() const
{
  return m_fSlopeBias;
}

void ezLightComponent::SetConstantBias(float fBias)
{
  m_fConstantBias = fBias;
}

float ezLightComponent::GetConstantBias() const
{
  return m_fConstantBias;
}

void ezLightComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  ezStreamWriter& s = stream.GetStream();

  s << m_LightColor;
  s << m_fIntensity;
  s << m_fPenumbraSize;
  s << m_fSlopeBias;
  s << m_fConstantBias;
  s << m_bCastShadows;
}

void ezLightComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  ezStreamReader& s = stream.GetStream();

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
}

void ezLightComponent::OnMsgSetColor(ezMsgSetColor& msg)
{
  msg.ModifyColor(m_LightColor);
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
    return ezMath::Pow(sphere.m_fRadius / fHalfHeight, 0.8f); // tweak factor to make transitions more linear.
  }
  else
  {
    float fHalfHeight = camera.GetDimensionY(1.0f) * 0.5f;
    return sphere.m_fRadius / fHalfHeight;
  }
}

  //////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>

class ezLightComponentPatch_1_2 : public ezGraphPatch
{
public:
  ezLightComponentPatch_1_2()
      : ezGraphPatch("ezLightComponent", 2)
  {
  }

  virtual void Patch(ezGraphPatchContext& context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Light Color", "LightColor");
  }
};

ezLightComponentPatch_1_2 g_ezLightComponentPatch_1_2;



EZ_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_LightComponent);

