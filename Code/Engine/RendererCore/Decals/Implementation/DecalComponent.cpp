#include <RendererCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <Core/Messages/DeleteObjectMessage.h>
#include <Core/Messages/TriggerMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <RendererCore/Decals/DecalAtlasResource.h>
#include <RendererCore/Decals/DecalComponent.h>
#include <RendererCore/Decals/DecalResource.h>
#include <RendererCore/Messages/ApplyOnlyToMessage.h>
#include <RendererCore/Messages/SetColorMessage.h>
#include <RendererFoundation/Shader/ShaderUtils.h>

#include <RendererCore/../../../Data/Base/Shaders/Common/LightData.h>

ezDecalComponentManager::ezDecalComponentManager(ezWorld* pWorld)
  : ezComponentManager<ezDecalComponent, ezBlockStorageType::Compact>(pWorld)
{
}

void ezDecalComponentManager::Initialize()
{
  m_hDecalAtlas = ezDecalAtlasResource::GetDecalAtlasResource();
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDecalRenderData, 1, ezRTTIDefaultAllocator<ezDecalRenderData>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_COMPONENT_TYPE(ezDecalComponent, 4, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Extents", GetExtents, SetExtents)->AddAttributes(new ezDefaultValueAttribute(ezVec3(1.0f)), new ezClampValueAttribute(ezVec3(0.01), ezVariant(25.0f))),
    EZ_ACCESSOR_PROPERTY("SizeVariance", GetSizeVariance, SetSizeVariance)->AddAttributes(new ezClampValueAttribute(0.0f, 1.0f)),
    EZ_ACCESSOR_PROPERTY("Color", GetColor, SetColor)->AddAttributes(new ezExposeColorAlphaAttribute()),
    EZ_ACCESSOR_PROPERTY("Decal", GetDecalFile, SetDecalFile)->AddAttributes(new ezAssetBrowserAttribute("Decal")),
    EZ_ACCESSOR_PROPERTY("SortOrder", GetSortOrder, SetSortOrder)->AddAttributes(new ezClampValueAttribute(-64.0f, 64.0f)),
    EZ_ACCESSOR_PROPERTY("WrapAround", GetWrapAround, SetWrapAround),
    EZ_ACCESSOR_PROPERTY("MapNormalToGeometry", GetMapNormalToGeometry, SetMapNormalToGeometry),
    EZ_ACCESSOR_PROPERTY("InnerFadeAngle", GetInnerFadeAngle, SetInnerFadeAngle)->AddAttributes(new ezClampValueAttribute(ezAngle::Degree(0.0f), ezAngle::Degree(90.0f)), new ezDefaultValueAttribute(ezAngle::Degree(50.0f))),
    EZ_ACCESSOR_PROPERTY("OuterFadeAngle", GetOuterFadeAngle, SetOuterFadeAngle)->AddAttributes(new ezClampValueAttribute(ezAngle::Degree(0.0f), ezAngle::Degree(90.0f)), new ezDefaultValueAttribute(ezAngle::Degree(80.0f))),
    EZ_MEMBER_PROPERTY("FadeOutDelay", m_FadeOutDelay),
    EZ_MEMBER_PROPERTY("FadeOutDuration", m_FadeOutDuration),
    EZ_ENUM_MEMBER_PROPERTY("OnFinishedAction", ezOnComponentFinishedAction, m_OnFinishedAction),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Effects"),
    new ezDirectionVisualizerAttribute(ezBasisAxis::PositiveX, 0.5f, ezColor::LightSteelBlue),
    new ezBoxManipulatorAttribute("Extents"),
    new ezBoxVisualizerAttribute("Extents"),
  }
  EZ_END_ATTRIBUTES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_FUNCTION_PROPERTY(OnObjectCreated),
  }
  EZ_END_FUNCTIONS;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgExtractRenderData, OnMsgExtractRenderData),
    EZ_MESSAGE_HANDLER(ezMsgComponentInternalTrigger, OnTriggered),
    EZ_MESSAGE_HANDLER(ezMsgDeleteGameObject, OnMsgDeleteGameObject),
    EZ_MESSAGE_HANDLER(ezMsgOnlyApplyToObject, OnMsgOnlyApplyToObject),
    EZ_MESSAGE_HANDLER(ezMsgSetColor, OnMsgSetColor),
  }
  EZ_END_MESSAGEHANDLERS;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezUInt16 ezDecalComponent::s_uiNextSortKey = 0;

ezDecalComponent::ezDecalComponent()
  : m_uiInternalSortKey(s_uiNextSortKey++)
{
}

ezDecalComponent::~ezDecalComponent() = default;

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
  s << m_bMapNormalToGeometry;
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

  if (uiVersion >= 4)
  {
    s >> m_bMapNormalToGeometry;
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

void ezDecalComponent::SetMapNormalToGeometry(bool bMapNormal)
{
  m_bMapNormalToGeometry = bMapNormal;
}

bool ezDecalComponent::GetMapNormalToGeometry() const
{
  return m_bMapNormalToGeometry;
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

void ezDecalComponent::OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const
{
  // Don't extract decal render data for selection.
  if (msg.m_OverrideCategory != ezInvalidRenderDataCategory)
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

  const bool bNoFade = m_InnerFadeAngle == ezAngle::Radian(0.0f) && m_OuterFadeAngle == ezAngle::Radian(0.0f);
  const float fCosInner = ezMath::Cos(m_InnerFadeAngle);
  const float fCosOuter = ezMath::Cos(m_OuterFadeAngle);
  const float fFadeParamScale = bNoFade ? 0.0f : (1.0f / ezMath::Max(0.001f, (fCosInner - fCosOuter)));
  const float fFadeParamOffset = bNoFade ? 1.0f : (-fCosOuter * fFadeParamScale);

  auto hDecalAtlas = GetWorld()->GetComponentManager<ezDecalComponentManager>()->m_hDecalAtlas;
  ezVec4 baseAtlasScaleOffset = ezVec4(0.5f);
  ezVec4 normalAtlasScaleOffset = ezVec4(0.5f);
  ezVec4 ormAtlasScaleOffset = ezVec4(0.5f);
  ezUInt32 uiDecalFlags = 0;

  {
    ezResourceLock<ezDecalAtlasResource> pDecalAtlas(hDecalAtlas, ezResourceAcquireMode::BlockTillLoaded);

    const auto& atlas = pDecalAtlas->GetAtlas();
    const ezUInt32 decalIdx = atlas.m_Items.Find(m_hDecal.GetResourceIDHash());

    if (decalIdx != ezInvalidIndex)
    {
      const auto& item = atlas.m_Items.GetValue(decalIdx);
      uiDecalFlags = item.m_uiFlags;

      auto layerRectToScaleOffset = [](ezRectU32 layerRect, ezVec2U32 textureSize) {
        ezVec4 result;
        result.x = (float)layerRect.width / textureSize.x * 0.5f;
        result.y = (float)layerRect.height / textureSize.y * 0.5f;
        result.z = (float)layerRect.x / textureSize.x + result.x;
        result.w = (float)layerRect.y / textureSize.y + result.y;
        return result;
      };

      baseAtlasScaleOffset = layerRectToScaleOffset(item.m_LayerRects[0], pDecalAtlas->GetBaseColorTextureSize());
      normalAtlasScaleOffset = layerRectToScaleOffset(item.m_LayerRects[1], pDecalAtlas->GetNormalTextureSize());
      ormAtlasScaleOffset = layerRectToScaleOffset(item.m_LayerRects[2], pDecalAtlas->GetORMTextureSize());
    }
  }

  auto pRenderData = ezCreateRenderDataForThisFrame<ezDecalRenderData>(GetOwner());

  ezUInt32 uiSortingId = (ezUInt32)(ezMath::Min(m_fSortOrder * 512.0f, 32767.0f) + 32768.0f);
  pRenderData->m_uiSortingKey = (uiSortingId << 16) | (m_uiInternalSortKey & 0xFFFF);

  pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
  pRenderData->m_vHalfExtents = m_vExtents * 0.5f;
  pRenderData->m_uiApplyOnlyToId = m_uiApplyOnlyToId;
  pRenderData->m_uiFlags = uiDecalFlags;
  pRenderData->m_uiFlags |= (m_bWrapAround ? DECAL_WRAP_AROUND : 0);
  pRenderData->m_uiFlags |= (m_bMapNormalToGeometry ? DECAL_MAP_NORMAL_TO_GEOMETRY : 0);
  pRenderData->m_uiAngleFadeParams = ezShaderUtils::Float2ToRG16F(ezVec2(fFadeParamScale, fFadeParamOffset));
  pRenderData->m_Color = finalColor;
  ezShaderUtils::Float4ToRGBA16F(baseAtlasScaleOffset, pRenderData->m_uiBaseColorAtlasScale, pRenderData->m_uiBaseColorAtlasOffset);
  ezShaderUtils::Float4ToRGBA16F(normalAtlasScaleOffset, pRenderData->m_uiNormalAtlasScale, pRenderData->m_uiNormalAtlasOffset);
  ezShaderUtils::Float4ToRGBA16F(ormAtlasScaleOffset, pRenderData->m_uiORMAtlasScale, pRenderData->m_uiORMAtlasOffset);

  ezRenderData::Caching::Enum caching = (m_FadeOutDelay.m_Value.GetSeconds() > 0.0 || m_FadeOutDuration.GetSeconds() > 0.0)
                                          ? ezRenderData::Caching::Never
                                          : ezRenderData::Caching::IfStatic;
  msg.AddRenderData(pRenderData, ezDefaultRenderDataCategories::Decal, caching);
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
  m_StartFadeOutTime =
    ezTime::Seconds(60.0 * 60.0 * 24.0 * 365.0 * 100.0); // 100 years should be enough for everybody (ignoring leap years)

  if (m_FadeOutDelay.m_Value.GetSeconds() > 0.0 || m_FadeOutDuration.GetSeconds() > 0.0)
  {
    const ezTime tFadeOutDelay =
      ezTime::Seconds(pWorld->GetRandomNumberGenerator().DoubleVariance(m_FadeOutDelay.m_Value.GetSeconds(), m_FadeOutDelay.m_fVariance));
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

  ezOnComponentFinishedAction::HandleFinishedAction(this, m_OnFinishedAction);
}

void ezDecalComponent::OnMsgDeleteGameObject(ezMsgDeleteGameObject& msg)
{
  ezOnComponentFinishedAction::HandleDeleteObjectMsg(msg, m_OnFinishedAction);
}

void ezDecalComponent::OnMsgOnlyApplyToObject(ezMsgOnlyApplyToObject& msg)
{
  SetApplyOnlyTo(msg.m_hObject);
}

void ezDecalComponent::OnMsgSetColor(ezMsgSetColor& msg)
{
  msg.ModifyColor(m_Color);
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_Decals_Implementation_DecalComponent);
