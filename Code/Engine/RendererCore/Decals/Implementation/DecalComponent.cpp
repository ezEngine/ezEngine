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
#include <RendererCore/RenderWorld/RenderWorld.h>
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

EZ_BEGIN_COMPONENT_TYPE(ezDecalComponent, 7, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ARRAY_ACCESSOR_PROPERTY("Decals", DecalFile_GetCount, DecalFile_Get, DecalFile_Set, DecalFile_Insert, DecalFile_Remove)->AddAttributes(new ezAssetBrowserAttribute("Decal")),
    EZ_ENUM_ACCESSOR_PROPERTY("ProjectionAxis", ezBasisAxis, GetProjectionAxis, SetProjectionAxis),
    EZ_ACCESSOR_PROPERTY("Extents", GetExtents, SetExtents)->AddAttributes(new ezDefaultValueAttribute(ezVec3(1.0f)), new ezClampValueAttribute(ezVec3(0.01), ezVariant(25.0f))),
    EZ_ACCESSOR_PROPERTY("SizeVariance", GetSizeVariance, SetSizeVariance)->AddAttributes(new ezClampValueAttribute(0.0f, 1.0f)),
    EZ_ACCESSOR_PROPERTY("Color", GetColor, SetColor)->AddAttributes(new ezExposeColorAlphaAttribute()),
    EZ_ACCESSOR_PROPERTY("EmissiveColor", GetEmissiveColor, SetEmissiveColor)->AddAttributes(new ezDefaultValueAttribute(ezColor::Black)),
    EZ_ACCESSOR_PROPERTY("SortOrder", GetSortOrder, SetSortOrder)->AddAttributes(new ezClampValueAttribute(-64.0f, 64.0f)),
    EZ_ACCESSOR_PROPERTY("WrapAround", GetWrapAround, SetWrapAround),
    EZ_ACCESSOR_PROPERTY("MapNormalToGeometry", GetMapNormalToGeometry, SetMapNormalToGeometry)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_ACCESSOR_PROPERTY("InnerFadeAngle", GetInnerFadeAngle, SetInnerFadeAngle)->AddAttributes(new ezClampValueAttribute(ezAngle::Degree(0.0f), ezAngle::Degree(89.0f)), new ezDefaultValueAttribute(ezAngle::Degree(50.0f))),
    EZ_ACCESSOR_PROPERTY("OuterFadeAngle", GetOuterFadeAngle, SetOuterFadeAngle)->AddAttributes(new ezClampValueAttribute(ezAngle::Degree(0.0f), ezAngle::Degree(89.0f)), new ezDefaultValueAttribute(ezAngle::Degree(80.0f))),
    EZ_MEMBER_PROPERTY("FadeOutDelay", m_FadeOutDelay),
    EZ_MEMBER_PROPERTY("FadeOutDuration", m_FadeOutDuration),
    EZ_ENUM_MEMBER_PROPERTY("OnFinishedAction", ezOnComponentFinishedAction, m_OnFinishedAction),
    EZ_ACCESSOR_PROPERTY("ApplyToDynamic", DummyGetter, SetApplyToRef)->AddAttributes(new ezGameObjectReferenceAttribute()),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Effects"),
    new ezDirectionVisualizerAttribute("ProjectionAxis", 0.5f, ezColor::LightSteelBlue),
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
  s << m_EmissiveColor;
  s << m_InnerFadeAngle;
  s << m_OuterFadeAngle;
  s << m_fSortOrder;
  s << m_uiInternalSortKey;
  s << m_FadeOutDelay.m_Value;
  s << m_FadeOutDelay.m_fVariance;
  s << m_FadeOutDuration;
  s << m_StartFadeOutTime;
  s << m_fSizeVariance;
  s << m_OnFinishedAction;
  s << m_bWrapAround;
  s << m_bMapNormalToGeometry;

  // version 5
  s << m_ProjectionAxis;

  // version 6
  stream.WriteGameObjectHandle(m_hApplyOnlyToObject);

  // version 7
  s << m_uiRandomDecalIdx;
  s.WriteArray(m_hDecals);
}

void ezDecalComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  ezStreamReader& s = stream.GetStream();

  s >> m_vExtents;

  if (uiVersion >= 4)
  {
    s >> m_Color;
    s >> m_EmissiveColor;
  }
  else
  {
    ezColor tmp;
    s >> tmp;
    m_Color = tmp;
  }

  s >> m_InnerFadeAngle;
  s >> m_OuterFadeAngle;
  s >> m_fSortOrder;
  s >> m_uiInternalSortKey;

  if (uiVersion < 7)
  {
    m_hDecals.SetCount(1);
    s >> m_hDecals[0];
  }

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

  if (uiVersion >= 5)
  {
    s >> m_ProjectionAxis;
  }

  if (uiVersion >= 6)
  {
    SetApplyOnlyTo(stream.ReadGameObjectHandle());
  }

  if (uiVersion >= 7)
  {
    s >> m_uiRandomDecalIdx;
    s.ReadArray(m_hDecals);
  }
}

ezResult ezDecalComponent::GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible)
{
  if (m_hDecals.IsEmpty())
    return EZ_FAILURE;

  const ezUInt32 uiDecalIndex = ezMath::Min<ezUInt32>(m_uiRandomDecalIdx, m_hDecals.GetCount() - 1);

  if (!m_hDecals[uiDecalIndex].IsValid() || m_vExtents.IsZero())
    return EZ_FAILURE;

  float fAspectRatio = 1.0f;

  {
    auto hDecalAtlas = GetWorld()->GetComponentManager<ezDecalComponentManager>()->m_hDecalAtlas;
    ezResourceLock<ezDecalAtlasResource> pDecalAtlas(hDecalAtlas, ezResourceAcquireMode::BlockTillLoaded);

    const auto& atlas = pDecalAtlas->GetAtlas();
    const ezUInt32 decalIdx = atlas.m_Items.Find(m_hDecals[uiDecalIndex].GetResourceIDHash());

    if (decalIdx != ezInvalidIndex)
    {
      const auto& item = atlas.m_Items.GetValue(decalIdx);
      fAspectRatio = item.m_LayerRects[0].width / item.m_LayerRects[0].height;
    }
  }

  ezVec3 vAspectCorrection = ezVec3(1.0f);
  if (!ezMath::IsEqual(fAspectRatio, 1.0f, 0.001f))
  {
    if (fAspectRatio > 1.0f)
    {
      vAspectCorrection.z /= fAspectRatio;
    }
    else
    {
      vAspectCorrection.y *= fAspectRatio;
    }
  }

  const ezQuat axisRotation = ezBasisAxis::GetBasisRotation_PosX(m_ProjectionAxis);
  ezVec3 vHalfExtents = (axisRotation * vAspectCorrection).Abs().CompMul(m_vExtents * 0.5f);

  bounds = ezBoundingBox(-vHalfExtents, vHalfExtents);
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

void ezDecalComponent::SetColor(ezColorGammaUB color)
{
  m_Color = color;
}

ezColorGammaUB ezDecalComponent::GetColor() const
{
  return m_Color;
}

void ezDecalComponent::SetEmissiveColor(ezColor color)
{
  m_EmissiveColor = color;
}

ezColor ezDecalComponent::GetEmissiveColor() const
{
  return m_EmissiveColor;
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

void ezDecalComponent::SetDecal(ezUInt32 uiIndex, const ezDecalResourceHandle& hDecal)
{
  m_hDecals[uiIndex] = hDecal;

  TriggerLocalBoundsUpdate();
}

const ezDecalResourceHandle& ezDecalComponent::GetDecal(ezUInt32 uiIndex) const
{
  return m_hDecals[uiIndex];
}

void ezDecalComponent::SetProjectionAxis(ezEnum<ezBasisAxis> ProjectionAxis)
{
  m_ProjectionAxis = ProjectionAxis;

  TriggerLocalBoundsUpdate();
}

ezEnum<ezBasisAxis> ezDecalComponent::GetProjectionAxis() const
{
  return m_ProjectionAxis;
}

void ezDecalComponent::SetApplyOnlyTo(ezGameObjectHandle hObject)
{
  if (m_hApplyOnlyToObject != hObject)
  {
    m_hApplyOnlyToObject = hObject;
    UpdateApplyTo();
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

  if (m_hDecals.IsEmpty())
    return;

  const ezUInt32 uiDecalIndex = ezMath::Min<ezUInt32>(m_uiRandomDecalIdx, m_hDecals.GetCount() - 1);

  if (!m_hDecals[uiDecalIndex].IsValid() || m_vExtents.IsZero() || GetOwner()->GetLocalScaling().IsZero())
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

  float fAspectRatio = 1.0f;

  {
    ezResourceLock<ezDecalAtlasResource> pDecalAtlas(hDecalAtlas, ezResourceAcquireMode::BlockTillLoaded);

    const auto& atlas = pDecalAtlas->GetAtlas();
    const ezUInt32 decalIdx = atlas.m_Items.Find(m_hDecals[uiDecalIndex].GetResourceIDHash());

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

      fAspectRatio = item.m_LayerRects[0].width / item.m_LayerRects[0].height;
    }
  }

  auto pRenderData = ezCreateRenderDataForThisFrame<ezDecalRenderData>(GetOwner());

  ezUInt32 uiSortingId = (ezUInt32)(ezMath::Min(m_fSortOrder * 512.0f, 32767.0f) + 32768.0f);
  pRenderData->m_uiSortingKey = (uiSortingId << 16) | (m_uiInternalSortKey & 0xFFFF);

  const ezQuat axisRotation = ezBasisAxis::GetBasisRotation_PosX(m_ProjectionAxis);

  pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
  pRenderData->m_GlobalTransform.m_vScale = (axisRotation * (pRenderData->m_GlobalTransform.m_vScale.CompMul(m_vExtents * 0.5f))).Abs();
  pRenderData->m_GlobalTransform.m_qRotation = pRenderData->m_GlobalTransform.m_qRotation * axisRotation;

  if (!ezMath::IsEqual(fAspectRatio, 1.0f, 0.001f))
  {
    if (fAspectRatio > 1.0f)
    {
      pRenderData->m_GlobalTransform.m_vScale.z /= fAspectRatio;
    }
    else
    {
      pRenderData->m_GlobalTransform.m_vScale.y *= fAspectRatio;
    }
  }

  pRenderData->m_uiApplyOnlyToId = m_uiApplyOnlyToId;
  pRenderData->m_uiFlags = uiDecalFlags;
  pRenderData->m_uiFlags |= (m_bWrapAround ? DECAL_WRAP_AROUND : 0);
  pRenderData->m_uiFlags |= (m_bMapNormalToGeometry ? DECAL_MAP_NORMAL_TO_GEOMETRY : 0);
  pRenderData->m_uiAngleFadeParams = ezShaderUtils::Float2ToRG16F(ezVec2(fFadeParamScale, fFadeParamOffset));
  pRenderData->m_BaseColor = finalColor;
  pRenderData->m_EmissiveColor = m_EmissiveColor;
  ezShaderUtils::Float4ToRGBA16F(baseAtlasScaleOffset, pRenderData->m_uiBaseColorAtlasScale, pRenderData->m_uiBaseColorAtlasOffset);
  ezShaderUtils::Float4ToRGBA16F(normalAtlasScaleOffset, pRenderData->m_uiNormalAtlasScale, pRenderData->m_uiNormalAtlasOffset);
  ezShaderUtils::Float4ToRGBA16F(ormAtlasScaleOffset, pRenderData->m_uiORMAtlasScale, pRenderData->m_uiORMAtlasOffset);

  ezRenderData::Caching::Enum caching = (m_FadeOutDelay.m_Value.GetSeconds() > 0.0 || m_FadeOutDuration.GetSeconds() > 0.0)
                                          ? ezRenderData::Caching::Never
                                          : ezRenderData::Caching::IfStatic;
  msg.AddRenderData(pRenderData, ezDefaultRenderDataCategories::Decal, caching);
}

void ezDecalComponent::SetApplyToRef(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  ezGameObjectHandle hTarget = resolver(szReference, GetHandle(), "ApplyTo");

  if (m_hApplyOnlyToObject == hTarget)
    return;

  m_hApplyOnlyToObject = hTarget;

  if (IsActiveAndInitialized())
  {
    UpdateApplyTo();
  }
}

void ezDecalComponent::UpdateApplyTo()
{
  ezUInt32 uiPrevId = m_uiApplyOnlyToId;

  m_uiApplyOnlyToId = 0;

  if (!m_hApplyOnlyToObject.IsInvalidated())
  {
    m_uiApplyOnlyToId = ezInvalidIndex;

    ezGameObject* pObject = nullptr;
    if (GetWorld()->TryGetObject(m_hApplyOnlyToObject, pObject))
    {
      ezRenderComponent* pRenderComponent = nullptr;
      if (pObject->TryGetComponentOfBaseType(pRenderComponent))
      {
        // this only works for dynamic objects, for static ones we must use ID 0
        if (pRenderComponent->GetOwner()->IsDynamic())
        {
          m_uiApplyOnlyToId = pRenderComponent->GetUniqueIdForRendering();
        }
      }
    }
  }

  if (uiPrevId != m_uiApplyOnlyToId && GetOwner()->IsStatic())
  {
    ezRenderWorld::DeleteCachedRenderData(GetOwner()->GetHandle(), GetHandle());
  }
}

void ezDecalComponent::OnObjectCreated(const ezAbstractObjectNode& node)
{
  m_uiInternalSortKey = ezHashHelper<ezUuid>::Hash(node.GetGuid());
  m_uiInternalSortKey = (m_uiInternalSortKey >> 16) ^ (m_uiInternalSortKey & 0xFFFF);
}

void ezDecalComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  ezWorld* pWorld = GetWorld();

  // no fade out -> fade out pretty late
  m_StartFadeOutTime = ezTime::Seconds(60.0 * 60.0 * 24.0 * 365.0 * 100.0); // 100 years should be enough for everybody (ignoring leap years)

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

      PostMessage(msg, tKill);
    }
  }

  if (m_fSizeVariance > 0)
  {
    const float scale = (float)pWorld->GetRandomNumberGenerator().DoubleVariance(1.0, m_fSizeVariance);
    m_vExtents *= scale;

    TriggerLocalBoundsUpdate();
  }

  if (m_uiRandomDecalIdx == 0xFF && !m_hDecals.IsEmpty())
  {
    m_uiRandomDecalIdx = GetWorld()->GetRandomNumberGenerator().UIntInRange(m_hDecals.GetCount());
  }
}

void ezDecalComponent::OnActivated()
{
  SUPER::OnActivated();

  UpdateApplyTo();
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

ezUInt32 ezDecalComponent::DecalFile_GetCount() const
{
  return m_hDecals.GetCount();
}

const char* ezDecalComponent::DecalFile_Get(ezUInt32 uiIndex) const
{
  if (!m_hDecals[uiIndex].IsValid())
    return "";

  return m_hDecals[uiIndex].GetResourceID();
}

void ezDecalComponent::DecalFile_Set(ezUInt32 uiIndex, const char* szFile)
{
  ezDecalResourceHandle hResource;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = ezResourceManager::LoadResource<ezDecalResource>(szFile);
  }

  SetDecal(uiIndex, hResource);
}

void ezDecalComponent::DecalFile_Insert(ezUInt32 uiIndex, const char* szFile)
{
  m_hDecals.Insert(ezDecalResourceHandle(), uiIndex);
  DecalFile_Set(uiIndex, szFile);
}

void ezDecalComponent::DecalFile_Remove(ezUInt32 uiIndex)
{
  m_hDecals.RemoveAtAndCopy(uiIndex);
}

//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class ezDecalComponent_6_7 : public ezGraphPatch
{
public:
  ezDecalComponent_6_7()
    : ezGraphPatch("ezDecalComponent", 7)
  {
  }

  virtual void Patch(ezGraphPatchContext& context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    auto* pDecal = pNode->FindProperty("Decal");
    if (pDecal && pDecal->m_Value.IsA<ezString>())
    {
      ezVariantArray ar;
      ar.PushBack(pDecal->m_Value.Get<ezString>());
      pNode->AddProperty("Decals", ar);
    }
  }
};

ezDecalComponent_6_7 g_ezDecalComponent_6_7;

EZ_STATICLINK_FILE(RendererCore, RendererCore_Decals_Implementation_DecalComponent);
