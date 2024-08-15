#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Lights/SphereReflectionProbeComponent.h>

#include <../../Data/Base/Shaders/Common/LightData.h>
#include <Core/Messages/TransformChangedMessage.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <RendererCore/Lights/Implementation/ReflectionPool.h>
#include <RendererCore/Pipeline/View.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezSphereReflectionProbeComponent, 2, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Radius", GetRadius, SetRadius)->AddAttributes(new ezClampValueAttribute(0.0f, {}), new ezDefaultValueAttribute(5.0f)),
    EZ_ACCESSOR_PROPERTY("Falloff", GetFalloff, SetFalloff)->AddAttributes(new ezClampValueAttribute(0.0f, 1.0f), new ezDefaultValueAttribute(0.1f)),
    EZ_ACCESSOR_PROPERTY("SphereProjection", GetSphereProjection, SetSphereProjection)->AddAttributes(new ezDefaultValueAttribute(true)),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_FUNCTION_PROPERTY(OnObjectCreated),
  }
  EZ_END_FUNCTIONS;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgUpdateLocalBounds, OnUpdateLocalBounds),
    EZ_MESSAGE_HANDLER(ezMsgExtractRenderData, OnMsgExtractRenderData),
    EZ_MESSAGE_HANDLER(ezMsgTransformChanged, OnTransformChanged),
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Rendering/Reflections"),
    new ezSphereVisualizerAttribute("Radius", ezColorScheme::LightUI(ezColorScheme::Blue)),
    new ezSphereManipulatorAttribute("Radius"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezSphereReflectionProbeComponentManager::ezSphereReflectionProbeComponentManager(ezWorld* pWorld)
  : ezComponentManager<ezSphereReflectionProbeComponent, ezBlockStorageType::Compact>(pWorld)
{
}

//////////////////////////////////////////////////////////////////////////

ezSphereReflectionProbeComponent::ezSphereReflectionProbeComponent() = default;
ezSphereReflectionProbeComponent::~ezSphereReflectionProbeComponent() = default;

void ezSphereReflectionProbeComponent::SetRadius(float fRadius)
{
  m_fRadius = ezMath::Max(fRadius, 0.0f);
  m_bStatesDirty = true;
}

float ezSphereReflectionProbeComponent::GetRadius() const
{
  return m_fRadius;
}

void ezSphereReflectionProbeComponent::SetFalloff(float fFalloff)
{
  m_fFalloff = ezMath::Clamp(fFalloff, ezMath::DefaultEpsilon<float>(), 1.0f);
}

void ezSphereReflectionProbeComponent::SetSphereProjection(bool bSphereProjection)
{
  m_bSphereProjection = bSphereProjection;
}

void ezSphereReflectionProbeComponent::OnActivated()
{
  GetOwner()->EnableStaticTransformChangesNotifications();
  m_Id = ezReflectionPool::RegisterReflectionProbe(GetWorld(), m_Desc, this);
  GetOwner()->UpdateLocalBounds();
}

void ezSphereReflectionProbeComponent::OnDeactivated()
{
  ezReflectionPool::DeregisterReflectionProbe(GetWorld(), m_Id);
  m_Id.Invalidate();

  GetOwner()->UpdateLocalBounds();
}

void ezSphereReflectionProbeComponent::OnObjectCreated(const ezAbstractObjectNode& node)
{
  m_Desc.m_uniqueID = node.GetGuid();
}

void ezSphereReflectionProbeComponent::OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg)
{
  msg.SetAlwaysVisible(ezDefaultSpatialDataCategories::RenderDynamic);
}

void ezSphereReflectionProbeComponent::OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const
{
  // Don't trigger reflection rendering in shadow or other reflection views.
  if (msg.m_pView->GetCameraUsageHint() == ezCameraUsageHint::Shadow || msg.m_pView->GetCameraUsageHint() == ezCameraUsageHint::Reflection)
    return;

  if (m_bStatesDirty)
  {
    m_bStatesDirty = false;
    ezReflectionPool::UpdateReflectionProbe(GetWorld(), m_Id, m_Desc, this);
  }

  auto pRenderData = ezCreateRenderDataForThisFrame<ezReflectionProbeRenderData>(GetOwner());
  pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
  pRenderData->m_vProbePosition = pRenderData->m_GlobalTransform * m_Desc.m_vCaptureOffset;
  pRenderData->m_vHalfExtents = ezVec3(m_fRadius);
  pRenderData->m_vInfluenceScale = ezVec3(1.0f);
  pRenderData->m_vInfluenceShift = ezVec3(0.0f);
  pRenderData->m_vPositiveFalloff = ezVec3(m_fFalloff);
  pRenderData->m_vNegativeFalloff = ezVec3(m_fFalloff);
  pRenderData->m_Id = m_Id;
  pRenderData->m_uiIndex = REFLECTION_PROBE_IS_SPHERE;
  if (m_bSphereProjection)
    pRenderData->m_uiIndex |= REFLECTION_PROBE_IS_PROJECTED;

  const ezVec3 vScale = pRenderData->m_GlobalTransform.m_vScale * m_fRadius;
  constexpr float fSphereConstant = (4.0f / 3.0f) * ezMath::Pi<float>();
  const float fEllipsoidVolume = fSphereConstant * ezMath::Abs(vScale.x * vScale.y * vScale.z);

  float fPriority = ComputePriority(msg, pRenderData, fEllipsoidVolume, vScale);
  ezReflectionPool::ExtractReflectionProbe(this, msg, pRenderData, GetWorld(), m_Id, fPriority);
}

void ezSphereReflectionProbeComponent::OnTransformChanged(ezMsgTransformChanged& msg)
{
  m_bStatesDirty = true;
}

void ezSphereReflectionProbeComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  ezStreamWriter& s = inout_stream.GetStream();

  s << m_fRadius;
  s << m_fFalloff;
  s << m_bSphereProjection;
}

void ezSphereReflectionProbeComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = inout_stream.GetStream();

  s >> m_fRadius;
  s >> m_fFalloff;
  if (uiVersion >= 2)
  {
    s >> m_bSphereProjection;
  }
  else
  {
    m_bSphereProjection = false;
  }
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class ezSphereReflectionProbeComponent_1_2 : public ezGraphPatch
{
public:
  ezSphereReflectionProbeComponent_1_2()
    : ezGraphPatch("ezSphereReflectionProbeComponent", 2)
  {
  }

  virtual void Patch(ezGraphPatchContext& ref_context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    pNode->AddProperty("SphereProjection", false);
  }
};

ezSphereReflectionProbeComponent_1_2 g_ezSphereReflectionProbeComponent_1_2;

EZ_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_SphereReflectionProbeComponent);
