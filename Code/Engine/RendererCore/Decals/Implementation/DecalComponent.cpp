#include <PCH.h>
#include <RendererCore/Decals/DecalComponent.h>
#include <RendererCore/Decals/DecalAtlasResource.h>
#include <RendererCore/Decals/DecalResource.h>
#include <RendererCore/Messages/ApplyOnlyToMessage.h>
#include <RendererCore/Messages/SetColorMessage.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/Graphics/Camera.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Core/Messages/TriggerMessage.h>


ezDecalComponentManager::ezDecalComponentManager(ezWorld* pWorld)
  : ezComponentManager<ezDecalComponent, ezBlockStorageType::Compact>(pWorld)
{

}

void ezDecalComponentManager::Initialize()
{
  m_hDecalAtlas = ezDecalAtlasResource::GetDecalAtlasResource();
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDecalRenderData, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_COMPONENT_TYPE(ezDecalComponent, 3, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Extents", GetExtents, SetExtents)->AddAttributes(new ezDefaultValueAttribute(ezVec3(1.0f)), new ezClampValueAttribute(ezVec3(0.01), ezVariant(25.0f))),
    EZ_ACCESSOR_PROPERTY("SizeVariance", GetSizeVariance, SetSizeVariance)->AddAttributes(new ezClampValueAttribute(0.0f, 1.0f)),
    EZ_ACCESSOR_PROPERTY("Color", GetColor, SetColor)->AddAttributes(new ezExposeColorAlphaAttribute()),
    EZ_ACCESSOR_PROPERTY("Decal", GetDecalFile, SetDecalFile)->AddAttributes(new ezAssetBrowserAttribute("Decal")),
    EZ_ACCESSOR_PROPERTY("SortOrder", GetSortOrder, SetSortOrder)->AddAttributes(new ezClampValueAttribute(-64.0f, 64.0f)),
    EZ_ACCESSOR_PROPERTY("WrapAround", GetWrapAround, SetWrapAround),
    EZ_ACCESSOR_PROPERTY("InnerFadeAngle", GetInnerFadeAngle, SetInnerFadeAngle)->AddAttributes(new ezClampValueAttribute(ezAngle::Degree(0.0f), ezAngle::Degree(90.0f)), new ezDefaultValueAttribute(ezAngle::Degree(50.0f))),
    EZ_ACCESSOR_PROPERTY("OuterFadeAngle", GetOuterFadeAngle, SetOuterFadeAngle)->AddAttributes(new ezClampValueAttribute(ezAngle::Degree(0.0f), ezAngle::Degree(90.0f)), new ezDefaultValueAttribute(ezAngle::Degree(80.0f))),
    EZ_MEMBER_PROPERTY("FadeOutDelay", m_FadeOutDelay),
    EZ_MEMBER_PROPERTY("FadeOutDuration", m_FadeOutDuration),
    EZ_ENUM_MEMBER_PROPERTY("OnFinishedAction", ezOnComponentFinishedAction, m_OnFinishedAction),
  }
  EZ_END_PROPERTIES
    EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Effects"),
    new ezDirectionVisualizerAttribute(ezBasisAxis::PositiveX, 0.5f, ezColor::LightSteelBlue),
    new ezBoxManipulatorAttribute("Extents"),
    new ezBoxVisualizerAttribute("Extents"),
  }
  EZ_END_ATTRIBUTES
    EZ_BEGIN_FUNCTIONS
  {
    EZ_FUNCTION_PROPERTY(OnObjectCreated),
  }
  EZ_END_FUNCTIONS
    EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgExtractRenderData, OnExtractRenderData),
    EZ_MESSAGE_HANDLER(ezMsgComponentInternalTrigger, OnTriggered),
    EZ_MESSAGE_HANDLER(ezMsgOnlyApplyToObject, OnApplyOnlyTo),
    EZ_MESSAGE_HANDLER(ezMsgSetColor, OnSetColor),
  }
  EZ_END_MESSAGEHANDLERS
}
EZ_END_COMPONENT_TYPE

ezUInt16 ezDecalComponent::s_uiNextSortKey = 0;

ezDecalComponent::ezDecalComponent()
  : m_vExtents(1.0f)
  , m_fSizeVariance(0.0f)
  , m_Color(ezColor::White)
  , m_InnerFadeAngle(ezAngle::Degree(50.0f))
  , m_OuterFadeAngle(ezAngle::Degree(80.0f))
  , m_fSortOrder(0.0f)
  , m_bWrapAround(false)
  , m_uiApplyOnlyToId(0)
  , m_uiInternalSortKey(s_uiNextSortKey++)
{
}

ezDecalComponent::~ezDecalComponent()
{
}

void ezDecalComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  ezStreamWriter& s = stream.GetStream();

  s << m_vExtents;
  s << m_Color;
  s << m_InnerFadeAngle;
  s << m_OuterFadeAngle;
  s << m_fSortOrder;
  s << m_uiInternalSortKey;
  s << m_hDecal;
  s << m_FadeOutDelay.m_Value;
  s << m_FadeOutDelay.m_fVariance;
  s << m_FadeOutDuration;
  s << m_StartFadeOutTime;
  s << m_fSizeVariance;
  s << m_OnFinishedAction;
  s << m_bWrapAround;

}

void ezDecalComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  ezStreamReader& s = stream.GetStream();

  s >> m_vExtents;
  s >> m_Color;
  s >> m_InnerFadeAngle;
  s >> m_OuterFadeAngle;
  s >> m_fSortOrder;
  s >> m_uiInternalSortKey;
  s >> m_hDecal;
  s >> m_FadeOutDelay.m_Value;
  s >> m_FadeOutDelay.m_fVariance;
  s >> m_FadeOutDuration;
  s >> m_StartFadeOutTime;
  s >> m_fSizeVariance;
  s >> m_OnFinishedAction;

  if (uiVersion >= 3)
  {
    s >> m_bWrapAround;
  }
}

ezResult ezDecalComponent::GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible)
{
  bounds = ezBoundingBox(m_vExtents * -0.5f, m_vExtents * 0.5f);
  return EZ_SUCCESS;
}

void ezDecalComponent::SetExtents(const ezVec3& value)
{
  m_vExtents = value.CompMax(ezVec3::ZeroVector());

  TriggerLocalBoundsUpdate();
}

const ezVec3& ezDecalComponent::GetExtents() const
{
  return m_vExtents;
}

void ezDecalComponent::SetSizeVariance(float fVariance)
{
  m_fSizeVariance = ezMath::Clamp(fVariance, 0.0f, 1.0f);
}

float ezDecalComponent::GetSizeVariance() const
{
  return m_fSizeVariance;
}

void ezDecalComponent::SetColor(ezColor color)
{
  m_Color = color;
}

ezColor ezDecalComponent::GetColor() const
{
  return m_Color;
}

void ezDecalComponent::SetInnerFadeAngle(ezAngle spotAngle)
{
  m_InnerFadeAngle = ezMath::Clamp(spotAngle, ezAngle::Degree(0.0f), m_OuterFadeAngle);
}

ezAngle ezDecalComponent::GetInnerFadeAngle() const
{
  return m_InnerFadeAngle;
}

void ezDecalComponent::SetOuterFadeAngle(ezAngle spotAngle)
{
  m_OuterFadeAngle = ezMath::Clamp(spotAngle, m_InnerFadeAngle, ezAngle::Degree(90.0f));
}

ezAngle ezDecalComponent::GetOuterFadeAngle() const
{
  return m_OuterFadeAngle;
}

void ezDecalComponent::SetSortOrder(float fOrder)
{
  m_fSortOrder = fOrder;
}

float ezDecalComponent::GetSortOrder() const
{
  return m_fSortOrder;
}

void ezDecalComponent::SetWrapAround(bool bWrapAround)
{
  m_bWrapAround = bWrapAround;
}

bool ezDecalComponent::GetWrapAround() const
{
  return m_bWrapAround;
}

void ezDecalComponent::SetDecal(const ezDecalResourceHandle& hDecal)
{
  m_hDecal = hDecal;
}

const ezDecalResourceHandle& ezDecalComponent::GetDecal() const
{
  return m_hDecal;
}

void ezDecalComponent::SetDecalFile(const char* szFile)
{
  ezDecalResourceHandle hResource;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = ezResourceManager::LoadResource<ezDecalResource>(szFile);
  }

  SetDecal(hResource);
}

const char* ezDecalComponent::GetDecalFile() const
{
  if (!m_hDecal.IsValid())
    return "";

  return m_hDecal.GetResourceID();
}

void ezDecalComponent::SetApplyOnlyTo(ezGameObjectHandle hObject)
{
  if (m_hApplyOnlyToObject != hObject)
  {
    m_hApplyOnlyToObject = hObject;
    m_uiApplyOnlyToId = 0;

    ezGameObject* pObject = nullptr;
    if (!GetWorld()->TryGetObject(hObject, pObject))
      return;

    ezRenderComponent* pRenderComponent = nullptr;
    if (pObject->TryGetComponentOfBaseType(pRenderComponent))
    {
      m_uiApplyOnlyToId = pRenderComponent->GetUniqueIdForRendering();
    }
  }
}

ezGameObjectHandle ezDecalComponent::GetApplyOnlyTo() const
{
  return m_hApplyOnlyToObject;
}

void ezDecalComponent::OnExtractRenderData(ezMsgExtractRenderData& msg) const
{
  // Don't extract decal render data for selection.
  if (msg.m_OverrideCategory != ezInvalidIndex)
    return;

  if (!m_hDecal.IsValid() || m_vExtents.IsZero() || GetOwner()->GetLocalScaling().IsZero())
    return;

  float fFade = 1.0f;

  const ezTime tNow = GetWorld()->GetClock().GetAccumulatedTime();
  if (tNow > m_StartFadeOutTime)
  {
    fFade -= ezMath::Min<float>(1.0f, (float)((tNow - m_StartFadeOutTime).GetSeconds() / m_FadeOutDuration.GetSeconds()));
  }

  ezColor finalColor = m_Color;
  finalColor.a *= fFade;

  if (finalColor.a <= 0.0f)
    return;

  auto hDecalAtlas = GetWorld()->GetComponentManager<ezDecalComponentManager>()->m_hDecalAtlas;
  ezVec2 baseAtlasScale = ezVec2(0.5f);
  ezVec2 baseAtlasOffset = ezVec2(0.5f);

  {
    ezResourceLock<ezDecalAtlasResource> pDecalAtlas(hDecalAtlas, ezResourceAcquireMode::NoFallback);
    ezVec2U32 baseTextureSize = pDecalAtlas->GetBaseColorTextureSize();

    if (auto pDecalInfo = pDecalAtlas->GetAllDecals().GetValue(m_hDecal.GetResourceIDHash()))
    {
      baseAtlasScale.x = (float)pDecalInfo->m_BaseColorRect.width / baseTextureSize.x * 0.5f;
      baseAtlasScale.y = (float)pDecalInfo->m_BaseColorRect.height / baseTextureSize.y * 0.5f;
      baseAtlasOffset.x = (float)pDecalInfo->m_BaseColorRect.x / baseTextureSize.x + baseAtlasScale.x;
      baseAtlasOffset.y = (float)pDecalInfo->m_BaseColorRect.y / baseTextureSize.y + baseAtlasScale.y;
    }
  }

  ezUInt32 uiBatchId = 0;
  auto pRenderData = ezCreateRenderDataForThisFrame<ezDecalRenderData>(GetOwner(), uiBatchId);

  pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
  pRenderData->m_vHalfExtents = m_vExtents * 0.5f;
  pRenderData->m_uiApplyOnlyToId = m_uiApplyOnlyToId;
  pRenderData->m_uiDecalMode = 0;
  pRenderData->m_bWrapAround = m_bWrapAround;
  pRenderData->m_Color = finalColor;
  pRenderData->m_InnerFadeAngle = m_InnerFadeAngle;
  pRenderData->m_OuterFadeAngle = m_OuterFadeAngle;
  pRenderData->m_vBaseAtlasScale = baseAtlasScale;
  pRenderData->m_vBaseAtlasOffset = baseAtlasOffset;

  ezUInt32 uiSortingId = (ezUInt32)(ezMath::Min(m_fSortOrder * 512.0f, 32767.0f) + 32768.0f);
  uiSortingId = (uiSortingId << 16) | (m_uiInternalSortKey & 0xFFFF);
  msg.m_pExtractedRenderData->AddRenderData(pRenderData, ezDefaultRenderDataCategories::Decal, uiSortingId);
}

void ezDecalComponent::OnObjectCreated(const ezAbstractObjectNode& node)
{
  m_uiInternalSortKey = ezHashHelper<ezUuid>::Hash(node.GetGuid());
  m_uiInternalSortKey = (m_uiInternalSortKey >> 16) ^ (m_uiInternalSortKey & 0xFFFF);
}

void ezDecalComponent::OnSimulationStarted()
{
  ezWorld* pWorld = GetWorld();

  // no fade out -> fade out pretty late
  m_StartFadeOutTime = ezTime::Seconds(60.0 * 60.0 * 24.0 * 365.0 * 100.0); // 100 years should be enough for everybody (ignoring leap years)

  if (m_FadeOutDelay.m_Value.GetSeconds() > 0.0 || m_FadeOutDuration.GetSeconds() > 0.0)
  {
    const ezTime tFadeOutDelay = ezTime::Seconds(pWorld->GetRandomNumberGenerator().DoubleVariance(m_FadeOutDelay.m_Value.GetSeconds(), m_FadeOutDelay.m_fVariance));
    m_StartFadeOutTime = pWorld->GetClock().GetAccumulatedTime() + tFadeOutDelay;

    if (m_OnFinishedAction != ezOnComponentFinishedAction::None)
    {
      ezMsgComponentInternalTrigger msg;
      msg.m_uiUsageStringHash = ezTempHashedString::ComputeHash("Suicide");

      const ezTime tKill = tFadeOutDelay + m_FadeOutDuration;

      PostMessage(msg, ezObjectMsgQueueType::NextFrame, tKill);
    }
  }

  if (m_fSizeVariance > 0)
  {
    const float scale = (float)pWorld->GetRandomNumberGenerator().DoubleVariance(1.0, m_fSizeVariance);
    m_vExtents *= scale;

    TriggerLocalBoundsUpdate();
  }
}

void ezDecalComponent::OnTriggered(ezMsgComponentInternalTrigger& msg)
{
  if (msg.m_uiUsageStringHash != ezTempHashedString::ComputeHash("Suicide"))
    return;

  if (m_OnFinishedAction == ezOnComponentFinishedAction::DeleteGameObject)
  {
    GetWorld()->DeleteObjectDelayed(GetOwner()->GetHandle());
  }
  else if (m_OnFinishedAction == ezOnComponentFinishedAction::DeleteComponent)
  {
    GetOwningManager()->DeleteComponent(GetHandle());
  }
}

void ezDecalComponent::OnApplyOnlyTo(ezMsgOnlyApplyToObject& msg)
{
  SetApplyOnlyTo(msg.m_hObject);
}

void ezDecalComponent::OnSetColor(ezMsgSetColor& msg)
{
  msg.ModifyColor(m_Color);
}
