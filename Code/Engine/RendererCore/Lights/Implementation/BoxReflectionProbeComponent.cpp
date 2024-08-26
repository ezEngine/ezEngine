#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Lights/BoxReflectionProbeComponent.h>

#include <../../Data/Base/Shaders/Common/LightData.h>
#include <Core/Messages/TransformChangedMessage.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <RendererCore/Lights/Implementation/ReflectionPool.h>
#include <RendererCore/Pipeline/View.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezBoxReflectionProbeComponent, 2, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Extents", GetExtents, SetExtents)->AddAttributes(new ezClampValueAttribute(ezVec3(0.0f), {}), new ezDefaultValueAttribute(ezVec3(5.0f))),
    EZ_ACCESSOR_PROPERTY("InfluenceScale", GetInfluenceScale, SetInfluenceScale)->AddAttributes(new ezClampValueAttribute(ezVec3(0.0f), ezVec3(1.0f)), new ezDefaultValueAttribute(ezVec3(1.0f))),
    EZ_ACCESSOR_PROPERTY("InfluenceShift", GetInfluenceShift, SetInfluenceShift)->AddAttributes(new ezClampValueAttribute(ezVec3(-1.0f), ezVec3(1.0f)), new ezDefaultValueAttribute(ezVec3(0.0f))),
    EZ_ACCESSOR_PROPERTY("PositiveFalloff", GetPositiveFalloff, SetPositiveFalloff)->AddAttributes(new ezClampValueAttribute(ezVec3(0.0f), ezVec3(1.0f)), new ezDefaultValueAttribute(ezVec3(0.1f, 0.1f, 0.0f))),
    EZ_ACCESSOR_PROPERTY("NegativeFalloff", GetNegativeFalloff, SetNegativeFalloff)->AddAttributes(new ezClampValueAttribute(ezVec3(0.0f), ezVec3(1.0f)), new ezDefaultValueAttribute(ezVec3(0.1f, 0.1f, 0.0f))),
    EZ_ACCESSOR_PROPERTY("BoxProjection", GetBoxProjection, SetBoxProjection)->AddAttributes(new ezDefaultValueAttribute(true)),
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
    new ezBoxVisualizerAttribute("Extents", 1.0f, ezColorScheme::LightUI(ezColorScheme::Blue)),
    new ezBoxManipulatorAttribute("Extents", 1.0f, true),
    new ezBoxReflectionProbeVisualizerAttribute("Extents", "InfluenceScale", "InfluenceShift"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezBoxReflectionProbeVisualizerAttribute, 1, ezRTTIDefaultAllocator<ezBoxReflectionProbeVisualizerAttribute>)
{
  EZ_BEGIN_FUNCTIONS
  {
    EZ_CONSTRUCTOR_PROPERTY(const char*, const char*, const char*),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezBoxReflectionProbeComponentManager::ezBoxReflectionProbeComponentManager(ezWorld* pWorld)
  : ezComponentManager<ezBoxReflectionProbeComponent, ezBlockStorageType::Compact>(pWorld)
{
}

//////////////////////////////////////////////////////////////////////////

ezBoxReflectionProbeComponent::ezBoxReflectionProbeComponent() = default;
ezBoxReflectionProbeComponent::~ezBoxReflectionProbeComponent() = default;

void ezBoxReflectionProbeComponent::SetExtents(const ezVec3& vExtents)
{
  m_vExtents = vExtents;
}

const ezVec3& ezBoxReflectionProbeComponent::GetInfluenceScale() const
{
  return m_vInfluenceScale;
}

void ezBoxReflectionProbeComponent::SetInfluenceScale(const ezVec3& vInfluenceScale)
{
  m_vInfluenceScale = vInfluenceScale;
}

const ezVec3& ezBoxReflectionProbeComponent::GetInfluenceShift() const
{
  return m_vInfluenceShift;
}

void ezBoxReflectionProbeComponent::SetInfluenceShift(const ezVec3& vInfluenceShift)
{
  m_vInfluenceShift = vInfluenceShift;
}

void ezBoxReflectionProbeComponent::SetPositiveFalloff(const ezVec3& vFalloff)
{
  // Does not affect cube generation so m_bStatesDirty is not set.
  m_vPositiveFalloff = vFalloff.CompClamp(ezVec3(ezMath::DefaultEpsilon<float>()), ezVec3(1.0f));
}

void ezBoxReflectionProbeComponent::SetNegativeFalloff(const ezVec3& vFalloff)
{
  // Does not affect cube generation so m_bStatesDirty is not set.
  m_vNegativeFalloff = vFalloff.CompClamp(ezVec3(ezMath::DefaultEpsilon<float>()), ezVec3(1.0f));
}

void ezBoxReflectionProbeComponent::SetBoxProjection(bool bBoxProjection)
{
  m_bBoxProjection = bBoxProjection;
}

const ezVec3& ezBoxReflectionProbeComponent::GetExtents() const
{
  return m_vExtents;
}

void ezBoxReflectionProbeComponent::OnActivated()
{
  GetOwner()->EnableStaticTransformChangesNotifications();
  m_Id = ezReflectionPool::RegisterReflectionProbe(GetWorld(), m_Desc, this);
  GetOwner()->UpdateLocalBounds();
}

void ezBoxReflectionProbeComponent::OnDeactivated()
{
  ezReflectionPool::DeregisterReflectionProbe(GetWorld(), m_Id);
  m_Id.Invalidate();

  GetOwner()->UpdateLocalBounds();
}

void ezBoxReflectionProbeComponent::OnObjectCreated(const ezAbstractObjectNode& node)
{
  m_Desc.m_uniqueID = node.GetGuid();
}

void ezBoxReflectionProbeComponent::OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg)
{
  msg.SetAlwaysVisible(ezDefaultSpatialDataCategories::RenderDynamic);
}

void ezBoxReflectionProbeComponent::OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const
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
  pRenderData->m_vHalfExtents = m_vExtents / 2.0f;
  pRenderData->m_vInfluenceScale = m_vInfluenceScale;
  pRenderData->m_vInfluenceShift = m_vInfluenceShift;
  pRenderData->m_vPositiveFalloff = m_vPositiveFalloff;
  pRenderData->m_vNegativeFalloff = m_vNegativeFalloff;
  pRenderData->m_Id = m_Id;
  pRenderData->m_uiIndex = 0;
  if (m_bBoxProjection)
    pRenderData->m_uiIndex |= REFLECTION_PROBE_IS_PROJECTED;

  const ezVec3 vScale = pRenderData->m_GlobalTransform.m_vScale.CompMul(m_vExtents);
  const float fVolume = ezMath::Abs(vScale.x * vScale.y * vScale.z);

  float fPriority = ComputePriority(msg, pRenderData, fVolume, vScale);
  ezReflectionPool::ExtractReflectionProbe(this, msg, pRenderData, GetWorld(), m_Id, fPriority);
}

void ezBoxReflectionProbeComponent::OnTransformChanged(ezMsgTransformChanged& msg)
{
  m_bStatesDirty = true;
}

void ezBoxReflectionProbeComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  ezStreamWriter& s = inout_stream.GetStream();

  s << m_vExtents;
  s << m_vInfluenceScale;
  s << m_vInfluenceShift;
  s << m_vPositiveFalloff;
  s << m_vNegativeFalloff;
  s << m_bBoxProjection;
}

void ezBoxReflectionProbeComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = inout_stream.GetStream();

  s >> m_vExtents;
  s >> m_vInfluenceScale;
  s >> m_vInfluenceShift;
  s >> m_vPositiveFalloff;
  s >> m_vNegativeFalloff;
  if (uiVersion >= 2)
  {
    s >> m_bBoxProjection;
  }
}

//////////////////////////////////////////////////////////////////////////

ezBoxReflectionProbeVisualizerAttribute::ezBoxReflectionProbeVisualizerAttribute()
  : ezVisualizerAttribute(nullptr)
{
}

ezBoxReflectionProbeVisualizerAttribute::ezBoxReflectionProbeVisualizerAttribute(const char* szExtentsProperty, const char* szInfluenceScaleProperty, const char* szInfluenceShiftProperty)
  : ezVisualizerAttribute(szExtentsProperty, szInfluenceScaleProperty, szInfluenceShiftProperty)
{
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_BoxReflectionProbeComponent);
