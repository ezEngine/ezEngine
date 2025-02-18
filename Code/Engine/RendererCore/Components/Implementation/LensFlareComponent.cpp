#include <RendererCore/RendererCorePCH.h>

#include <Core/Messages/SetColorMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Components/LensFlareComponent.h>
#include <RendererCore/Lights/DirectionalLightComponent.h>
#include <RendererCore/Lights/SpotLightComponent.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/Textures/Texture2DResource.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLensFlareRenderData, 1, ezRTTIDefaultAllocator<ezLensFlareRenderData>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void ezLensFlareRenderData::FillSortingKey()
{
  // ignore upper 32 bit of the resource ID hash
  const ezUInt32 uiTextureIDHash = static_cast<ezUInt32>(m_hTexture.GetResourceIDHash());

  // Sort by texture
  m_uiSortingKey = uiTextureIDHash;
}

bool ezLensFlareRenderData::CanBatch(const ezRenderData& other0) const
{
  const auto& other = ezStaticCast<const ezLensFlareRenderData&>(other0);

  return m_hTexture == other.m_hTexture;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezLensFlareElement, ezNoBase, 1, ezRTTIDefaultAllocator<ezLensFlareElement>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_RESOURCE_MEMBER_PROPERTY("Texture", m_hTexture)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Texture_2D")),
    EZ_MEMBER_PROPERTY("GreyscaleTexture", m_bGreyscaleTexture),
    EZ_MEMBER_PROPERTY("Color", m_Color)->AddAttributes(new ezExposeColorAlphaAttribute()),
    EZ_MEMBER_PROPERTY("ModulateByLightColor", m_bModulateByLightColor)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("Size", m_fSize)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant()), new ezDefaultValueAttribute(10000.0f), new ezSuffixAttribute(" m")),
    EZ_MEMBER_PROPERTY("MaxScreenSize", m_fMaxScreenSize)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant()), new ezDefaultValueAttribute(1.0f)),
    EZ_MEMBER_PROPERTY("AspectRatio", m_fAspectRatio)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant()), new ezDefaultValueAttribute(1.0f)),
    EZ_MEMBER_PROPERTY("ShiftToCenter", m_fShiftToCenter),
    EZ_MEMBER_PROPERTY("InverseTonemap", m_bInverseTonemap),
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;
// clang-format on

ezResult ezLensFlareElement::Serialize(ezStreamWriter& inout_stream) const
{
  inout_stream << m_hTexture;
  inout_stream << m_Color;
  inout_stream << m_fSize;
  inout_stream << m_fMaxScreenSize;
  inout_stream << m_fAspectRatio;
  inout_stream << m_fShiftToCenter;
  inout_stream << m_bInverseTonemap;
  inout_stream << m_bModulateByLightColor;
  inout_stream << m_bGreyscaleTexture;

  return EZ_SUCCESS;
}

ezResult ezLensFlareElement::Deserialize(ezStreamReader& inout_stream)
{
  inout_stream >> m_hTexture;
  inout_stream >> m_Color;
  inout_stream >> m_fSize;
  inout_stream >> m_fMaxScreenSize;
  inout_stream >> m_fAspectRatio;
  inout_stream >> m_fShiftToCenter;
  inout_stream >> m_bInverseTonemap;
  inout_stream >> m_bModulateByLightColor;
  inout_stream >> m_bGreyscaleTexture;

  return EZ_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezLensFlareComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("LinkToLightShape", GetLinkToLightShape, SetLinkToLightShape)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("Intensity", m_fIntensity)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant()), new ezDefaultValueAttribute(1.0f)),
    EZ_ACCESSOR_PROPERTY("OcclusionSampleRadius", GetOcclusionSampleRadius, SetOcclusionSampleRadius)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant()), new ezDefaultValueAttribute(0.1f), new ezSuffixAttribute(" m")),
    EZ_MEMBER_PROPERTY("OcclusionSampleSpread", m_fOcclusionSampleSpread)->AddAttributes(new ezClampValueAttribute(0.0f, 1.0f), new ezDefaultValueAttribute(0.5f)),
    EZ_MEMBER_PROPERTY("OcclusionDepthOffset", m_fOcclusionDepthOffset)->AddAttributes(new ezSuffixAttribute(" m")),
    EZ_MEMBER_PROPERTY("ApplyFog", m_bApplyFog)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_ARRAY_MEMBER_PROPERTY("Elements", m_Elements)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant()), new ezDefaultValueAttribute(1.0f)),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Rendering"),
    new ezSphereManipulatorAttribute("OcclusionSampleRadius"),
    new ezSphereVisualizerAttribute("OcclusionSampleRadius", ezColor::White)
  }
  EZ_END_ATTRIBUTES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgExtractRenderData, OnMsgExtractRenderData),
  }
  EZ_END_MESSAGEHANDLERS;
}
EZ_END_COMPONENT_TYPE;
// clang-format on

ezLensFlareComponent::ezLensFlareComponent() = default;
ezLensFlareComponent::~ezLensFlareComponent() = default;

void ezLensFlareComponent::OnActivated()
{
  SUPER::OnActivated();

  FindLightComponent();
}

void ezLensFlareComponent::OnDeactivated()
{
  SUPER::OnDeactivated();

  m_bDirectionalLight = false;
  m_hLightComponent.Invalidate();
}

void ezLensFlareComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  ezStreamWriter& s = inout_stream.GetStream();

  s.WriteArray(m_Elements).IgnoreResult();
  s << m_fIntensity;
  s << m_fOcclusionSampleRadius;
  s << m_fOcclusionSampleSpread;
  s << m_fOcclusionDepthOffset;
  s << m_bLinkToLightShape;
  s << m_bApplyFog;
}

void ezLensFlareComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  EZ_IGNORE_UNUSED(uiVersion);

  ezStreamReader& s = inout_stream.GetStream();

  s.ReadArray(m_Elements).IgnoreResult();
  s >> m_fIntensity;
  s >> m_fOcclusionSampleRadius;
  s >> m_fOcclusionSampleSpread;
  s >> m_fOcclusionDepthOffset;
  s >> m_bLinkToLightShape;
  s >> m_bApplyFog;
}

ezResult ezLensFlareComponent::GetLocalBounds(ezBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, ezMsgUpdateLocalBounds& ref_msg)
{
  if (m_bDirectionalLight)
  {
    ref_bAlwaysVisible = true;
  }
  else
  {
    ref_bounds = ezBoundingSphere::MakeFromCenterAndRadius(ezVec3::MakeZero(), m_fOcclusionSampleRadius);
  }
  return EZ_SUCCESS;
}

void ezLensFlareComponent::SetLinkToLightShape(bool bLink)
{
  if (m_bLinkToLightShape == bLink)
    return;

  m_bLinkToLightShape = bLink;
  if (IsActiveAndInitialized())
  {
    FindLightComponent();
  }

  TriggerLocalBoundsUpdate();
}

void ezLensFlareComponent::SetOcclusionSampleRadius(float fRadius)
{
  m_fOcclusionSampleRadius = fRadius;

  TriggerLocalBoundsUpdate();
}

void ezLensFlareComponent::FindLightComponent()
{
  ezLightComponent* pLightComponent = nullptr;

  if (m_bLinkToLightShape)
  {
    ezGameObject* pObject = GetOwner();
    while (pObject != nullptr)
    {
      if (pObject->TryGetComponentOfBaseType(pLightComponent))
        break;

      pObject = pObject->GetParent();
    }
  }

  if (pLightComponent != nullptr)
  {
    m_bDirectionalLight = pLightComponent->IsInstanceOf<ezDirectionalLightComponent>();
    m_hLightComponent = pLightComponent->GetHandle();
  }
  else
  {
    m_bDirectionalLight = false;
    m_hLightComponent.Invalidate();
  }
}

void ezLensFlareComponent::OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const
{
  // Don't render in shadow views
  if (msg.m_pView->GetCameraUsageHint() == ezCameraUsageHint::Shadow)
    return;

  // Don't extract render data for selection.
  if (msg.m_OverrideCategory != ezInvalidRenderDataCategory)
    return;

  if (m_fIntensity <= 0.0f)
    return;

  const ezCamera* pCamera = msg.m_pView->GetCamera();
  ezTransform globalTransform = GetOwner()->GetGlobalTransform();
  ezBoundingBoxSphere globalBounds = GetOwner()->GetGlobalBounds();
  float fScale = globalTransform.GetMaxScale();
  ezColor lightColor = ezColor::White;

  const ezLightComponent* pLightComponent = nullptr;
  if (GetWorld()->TryGetComponent(m_hLightComponent, pLightComponent))
  {
    lightColor = pLightComponent->GetLightColor();
    lightColor *= pLightComponent->GetIntensity() * 0.1f;
  }

  float fFade = 1.0f;
  if (auto pDirectionalLight = ezDynamicCast<const ezDirectionalLightComponent*>(pLightComponent))
  {
    ezTransform localOffset = ezTransform::MakeIdentity();
    localOffset.m_vPosition = ezVec3(pCamera->GetFarPlane() * -0.999f, 0, 0);

    globalTransform = ezTransform::MakeGlobalTransform(globalTransform, localOffset);
    globalTransform.m_vPosition += pCamera->GetCenterPosition();

    if (pCamera->IsPerspective())
    {
      float fHalfHeight = ezMath::Tan(pCamera->GetFovY(1.0f) * 0.5f) * pCamera->GetFarPlane();
      fScale *= fHalfHeight;
    }

    lightColor *= 10.0f;
  }
  else if (auto pSpotLight = ezDynamicCast<const ezSpotLightComponent*>(pLightComponent))
  {
    const ezVec3 lightDir = globalTransform.TransformDirection(ezVec3::MakeAxisX());
    const ezVec3 cameraDir = (pCamera->GetCenterPosition() - globalTransform.m_vPosition).GetNormalized();

    const float cosAngle = lightDir.Dot(cameraDir);
    const float fCosInner = ezMath::Cos(pSpotLight->GetInnerSpotAngle() * 0.5f);
    const float fCosOuter = ezMath::Cos(pSpotLight->GetOuterSpotAngle() * 0.5f);
    fFade = ezMath::Saturate((cosAngle - fCosOuter) / ezMath::Max(0.001f, (fCosInner - fCosOuter)));
    fFade *= fFade;
  }

  for (auto& element : m_Elements)
  {
    if (element.m_hTexture.IsValid() == false)
      continue;

    ezColor color = element.m_Color * m_fIntensity;
    if (element.m_bModulateByLightColor)
    {
      color *= lightColor;
    }
    color.a = element.m_Color.a * fFade;

    if (color.GetLuminance() <= 0.0f || color.a <= 0.0f)
      continue;

    ezLensFlareRenderData* pRenderData = ezCreateRenderDataForThisFrame<ezLensFlareRenderData>(GetOwner());
    {
      pRenderData->m_GlobalTransform = globalTransform;
      pRenderData->m_GlobalBounds = globalBounds;
      pRenderData->m_hTexture = element.m_hTexture;
      pRenderData->m_Color = color.GetAsVec4();
      pRenderData->m_fSize = element.m_fSize * fScale;
      pRenderData->m_fMaxScreenSize = element.m_fMaxScreenSize * 2.0f;
      pRenderData->m_fAspectRatio = 1.0f / element.m_fAspectRatio;
      pRenderData->m_fShiftToCenter = element.m_fShiftToCenter;
      pRenderData->m_fOcclusionSampleRadius = m_fOcclusionSampleRadius * fScale;
      pRenderData->m_fOcclusionSampleSpread = m_fOcclusionSampleSpread;
      pRenderData->m_fOcclusionDepthOffset = m_fOcclusionDepthOffset * fScale;
      pRenderData->m_bInverseTonemap = element.m_bInverseTonemap;
      pRenderData->m_bGreyscaleTexture = element.m_bGreyscaleTexture;
      pRenderData->m_bApplyFog = m_bApplyFog;

      pRenderData->FillSortingKey();
    }

    msg.AddRenderData(pRenderData, ezDefaultRenderDataCategories::LitTransparent,
      pLightComponent != nullptr ? ezRenderData::Caching::Never : ezRenderData::Caching::IfStatic);
  }
}


EZ_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_LensFlareComponent);
