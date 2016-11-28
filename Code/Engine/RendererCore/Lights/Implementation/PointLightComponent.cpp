#include <RendererCore/PCH.h>
#include <RendererCore/Lights/PointLightComponent.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPointLightRenderData, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_COMPONENT_TYPE(ezPointLightComponent, 2)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Range", GetRange, SetRange)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant()), new ezDefaultValueAttribute(10.0f), new ezSuffixAttribute(" m")),
    //EZ_ACCESSOR_PROPERTY("Projected Texture", GetProjectedTextureFile, SetProjectedTextureFile)->AddAttributes(new ezAssetBrowserAttribute("Texture Cube")),
  }
  EZ_END_PROPERTIES
    EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezExtractRenderDataMessage, OnExtractRenderData),
  }
  EZ_END_MESSAGEHANDLERS
    EZ_BEGIN_ATTRIBUTES
  {
    new ezSphereManipulatorAttribute("Range"),
    new ezSphereVisualizerAttribute("Range", "LightColor"),
  }
  EZ_END_ATTRIBUTES
}
EZ_END_COMPONENT_TYPE

ezPointLightComponent::ezPointLightComponent()
  : m_fRange(10.0f)
{
}

ezPointLightComponent::~ezPointLightComponent()
{

}

ezResult ezPointLightComponent::GetLocalBounds(ezBoundingBoxSphere& bounds)
{
  bounds = ezBoundingSphere(ezVec3::ZeroVector(), m_fRange);
  return EZ_SUCCESS;
}

void ezPointLightComponent::SetRange(float fRange)
{
  m_fRange = fRange;

  if (IsActive())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

float ezPointLightComponent::GetRange() const
{
  return m_fRange;
}

void ezPointLightComponent::SetProjectedTexture(const ezTextureResourceHandle& hProjectedTexture)
{
  m_hProjectedTexture = hProjectedTexture;
}

const ezTextureResourceHandle& ezPointLightComponent::GetProjectedTexture() const
{
  return m_hProjectedTexture;
}

void ezPointLightComponent::SetProjectedTextureFile(const char* szFile)
{
  ezTextureResourceHandle hProjectedTexture;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hProjectedTexture = ezResourceManager::LoadResource<ezTextureResource>(szFile);
  }

  SetProjectedTexture(hProjectedTexture);
}

const char* ezPointLightComponent::GetProjectedTextureFile() const
{
  if (!m_hProjectedTexture.IsValid())
    return "";

  return m_hProjectedTexture.GetResourceID();
}

void ezPointLightComponent::OnExtractRenderData( ezExtractRenderDataMessage& msg ) const
{
  if (msg.m_OverrideCategory != ezInvalidIndex)
    return;

  ezUInt32 uiBatchId = m_bCastShadows ? 0 : 1;

  auto pRenderData = ezCreateRenderDataForThisFrame<ezPointLightRenderData>(GetOwner(), uiBatchId);

  pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
  pRenderData->m_LightColor = m_LightColor;
  pRenderData->m_fIntensity = m_fIntensity;
  pRenderData->m_fRange = m_fRange;
  pRenderData->m_hProjectedTexture = m_hProjectedTexture;
  pRenderData->m_bCastShadows = m_bCastShadows;

  msg.m_pExtractedRenderData->AddRenderData(pRenderData, ezDefaultRenderDataCategories::Light, uiBatchId);
}

void ezPointLightComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  ezStreamWriter& s = stream.GetStream();

  s << m_fRange;
  s << m_hProjectedTexture;
}

void ezPointLightComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  //const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = stream.GetStream();

  s >> m_fRange;
  s >> m_hProjectedTexture;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>

class ezPointLightComponentPatch_1_2 : public ezGraphPatch
{
public:
  ezPointLightComponentPatch_1_2()
    : ezGraphPatch(ezGetStaticRTTI<ezPointLightComponent>(), 2) {}

  virtual void Patch(ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    PatchBaseClass(pGraph, pNode, ezGetStaticRTTI<ezLightComponent>(), 2);
  }
};

ezPointLightComponentPatch_1_2 g_ezPointLightComponentPatch_1_2;

EZ_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_PointLightComponent);

