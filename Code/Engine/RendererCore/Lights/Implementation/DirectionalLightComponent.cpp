#include <RendererCore/PCH.h>
#include <RendererCore/Lights/DirectionalLightComponent.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDirectionalLightRenderData, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_COMPONENT_TYPE(ezDirectionalLightComponent, 2)
{
  /*  EZ_BEGIN_PROPERTIES
    EZ_END_PROPERTIES*/
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezExtractRenderDataMessage, OnExtractRenderData),
  }
  EZ_END_MESSAGEHANDLERS
    EZ_BEGIN_ATTRIBUTES
  {
    new ezDirectionVisualizerAttribute(ezBasisAxis::PositiveX, 0.75f, "LightColor"),
  }
  EZ_END_ATTRIBUTES
}
EZ_END_COMPONENT_TYPE

ezDirectionalLightComponent::ezDirectionalLightComponent()
{

}

ezDirectionalLightComponent::~ezDirectionalLightComponent()
{

}

ezResult ezDirectionalLightComponent::GetLocalBounds(ezBoundingBoxSphere& bounds)
{
  ///\todo
  bounds = ezBoundingSphere(ezVec3::ZeroVector(), 1.0f);
  return EZ_SUCCESS;
}

void ezDirectionalLightComponent::OnExtractRenderData(ezExtractRenderDataMessage& msg) const
{
  if (msg.m_OverrideCategory != ezInvalidIndex)
    return;

  ezUInt32 uiBatchId = m_bCastShadows ? 0 : 1;

  auto pRenderData = ezCreateRenderDataForThisFrame<ezDirectionalLightRenderData>(GetOwner(), uiBatchId);

  pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
  pRenderData->m_LightColor = m_LightColor;
  pRenderData->m_fIntensity = m_fIntensity;
  pRenderData->m_bCastShadows = m_bCastShadows;

  msg.m_pExtractedRenderData->AddRenderData(pRenderData, ezDefaultRenderDataCategories::Light, uiBatchId);
}

void ezDirectionalLightComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  //ezStreamWriter& s = stream.GetStream();

}

void ezDirectionalLightComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  //const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  //ezStreamReader& s = stream.GetStream();

}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>

class ezDirectionalLightComponentPatch_1_2 : public ezGraphPatch
{
public:
  ezDirectionalLightComponentPatch_1_2()
    : ezGraphPatch(ezGetStaticRTTI<ezDirectionalLightComponent>(), 2) {}

  virtual void Patch(ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    PatchBaseClass(pGraph, pNode, ezGetStaticRTTI<ezLightComponent>(), 2);
  }
};

ezDirectionalLightComponentPatch_1_2 g_ezDirectionalLightComponentPatch_1_2;



EZ_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_DirectionalLightComponent);

