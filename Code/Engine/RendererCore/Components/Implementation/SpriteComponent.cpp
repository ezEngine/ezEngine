#include <RendererCore/RendererCorePCH.h>

#include <Core/Messages/SetColorMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Components/SpriteComponent.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/Textures/Texture2DResource.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezSpriteBlendMode, 1)
  EZ_ENUM_CONSTANTS(ezSpriteBlendMode::Masked, ezSpriteBlendMode::Transparent, ezSpriteBlendMode::Additive)
EZ_END_STATIC_REFLECTED_ENUM;
// clang-format on

// static
ezTempHashedString ezSpriteBlendMode::GetPermutationValue(Enum blendMode)
{
  switch (blendMode)
  {
    case ezSpriteBlendMode::Masked:
    case ezSpriteBlendMode::ShapeIcon:
      return "BLEND_MODE_MASKED";
    case ezSpriteBlendMode::Transparent:
      return "BLEND_MODE_TRANSPARENT";
    case ezSpriteBlendMode::Additive:
      return "BLEND_MODE_ADDITIVE";
  }

  return "";
}

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSpriteRenderData, 1, ezRTTIDefaultAllocator<ezSpriteRenderData>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void ezSpriteRenderData::FillSortingKey()
{
  // ignore upper 32 bit of the resource ID hash
  const ezUInt32 uiTextureIDHash = static_cast<ezUInt32>(m_hTexture.GetResourceIDHash());

  // Sort by mode and then by texture
  m_uiSortingKey = (m_BlendMode << 30) | (uiTextureIDHash & 0x3FFFFFFF);
}

bool ezSpriteRenderData::CanBatch(const ezRenderData& other0) const
{
  const auto& other = ezStaticCast<const ezSpriteRenderData&>(other0);

  return m_BlendMode == other.m_BlendMode && m_hTexture == other.m_hTexture;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezSpriteComponent, 3, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_RESOURCE_ACCESSOR_PROPERTY("Texture", GetTexture, SetTexture)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Texture_2D")),
    EZ_ENUM_MEMBER_PROPERTY("BlendMode", ezSpriteBlendMode, m_BlendMode),
    EZ_ACCESSOR_PROPERTY("Color", GetColor, SetColor)->AddAttributes(new ezExposeColorAlphaAttribute()),
    EZ_ACCESSOR_PROPERTY("Size", GetSize, SetSize)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant()), new ezDefaultValueAttribute(1.0f), new ezSuffixAttribute(" m")),
    EZ_ACCESSOR_PROPERTY("MaxScreenSize", GetMaxScreenSize, SetMaxScreenSize)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant()), new ezDefaultValueAttribute(64.0f), new ezSuffixAttribute(" px")),
    EZ_MEMBER_PROPERTY("AspectRatio", m_fAspectRatio)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant()), new ezDefaultValueAttribute(1.0f)),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Rendering"),
  }
  EZ_END_ATTRIBUTES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgExtractRenderData, OnMsgExtractRenderData),
    EZ_MESSAGE_HANDLER(ezMsgSetColor, OnMsgSetColor),
  }
  EZ_END_MESSAGEHANDLERS;
}
EZ_END_COMPONENT_TYPE;
// clang-format on

ezSpriteComponent::ezSpriteComponent() = default;
ezSpriteComponent::~ezSpriteComponent() = default;

ezResult ezSpriteComponent::GetLocalBounds(ezBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, ezMsgUpdateLocalBounds& ref_msg)
{
  ref_bounds = ezBoundingSphere::MakeFromCenterAndRadius(ezVec3::MakeZero(), m_fSize * 0.5f);
  return EZ_SUCCESS;
}

void ezSpriteComponent::OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const
{
  // Don't render in shadow views
  if (msg.m_pView->GetCameraUsageHint() == ezCameraUsageHint::Shadow)
    return;

  if (!m_hTexture.IsValid())
    return;

  ezSpriteRenderData* pRenderData = ezCreateRenderDataForThisFrame<ezSpriteRenderData>(GetOwner());
  {
    pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
    pRenderData->m_GlobalBounds = GetOwner()->GetGlobalBounds();
    pRenderData->m_hTexture = m_hTexture;
    pRenderData->m_fSize = m_fSize;
    pRenderData->m_fMaxScreenSize = m_fMaxScreenSize;
    pRenderData->m_fAspectRatio = m_fAspectRatio;
    pRenderData->m_BlendMode = m_BlendMode;
    pRenderData->m_color = m_Color;
    pRenderData->m_texCoordScale = ezVec2(1.0f);
    pRenderData->m_texCoordOffset = ezVec2(0.0f);
    pRenderData->m_uiUniqueID = GetUniqueIdForRendering();

    pRenderData->FillSortingKey();
  }

  // Determine render data category.
  ezRenderData::Category category = ezDefaultRenderDataCategories::LitTransparent;
  if (m_BlendMode == ezSpriteBlendMode::Masked)
  {
    category = ezDefaultRenderDataCategories::LitMasked;
  }

  msg.AddRenderData(pRenderData, category, ezRenderData::Caching::IfStatic);
}

void ezSpriteComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  ezStreamWriter& s = inout_stream.GetStream();

  s << m_hTexture;
  s << m_fSize;
  s << m_fMaxScreenSize;

  // Version 3
  s << m_Color; // HDR now
  s << m_fAspectRatio;
  s << m_BlendMode;
}

void ezSpriteComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  ezStreamReader& s = inout_stream.GetStream();

  s >> m_hTexture;

  if (uiVersion < 3)
  {
    ezColorGammaUB color;
    s >> color;
    m_Color = color;
  }

  s >> m_fSize;
  s >> m_fMaxScreenSize;

  if (uiVersion >= 3)
  {
    s >> m_Color;
    s >> m_fAspectRatio;
    s >> m_BlendMode;
  }
}

void ezSpriteComponent::SetTexture(const ezTexture2DResourceHandle& hTexture)
{
  m_hTexture = hTexture;
}

const ezTexture2DResourceHandle& ezSpriteComponent::GetTexture() const
{
  return m_hTexture;
}

void ezSpriteComponent::SetColor(ezColor color)
{
  m_Color = color;
}

ezColor ezSpriteComponent::GetColor() const
{
  return m_Color;
}

void ezSpriteComponent::SetSize(float fSize)
{
  m_fSize = fSize;

  TriggerLocalBoundsUpdate();
}

float ezSpriteComponent::GetSize() const
{
  return m_fSize;
}

void ezSpriteComponent::SetMaxScreenSize(float fSize)
{
  m_fMaxScreenSize = fSize;
}

float ezSpriteComponent::GetMaxScreenSize() const
{
  return m_fMaxScreenSize;
}

void ezSpriteComponent::OnMsgSetColor(ezMsgSetColor& ref_msg)
{
  ref_msg.ModifyColor(m_Color);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>

class ezSpriteComponentPatch_1_2 : public ezGraphPatch
{
public:
  ezSpriteComponentPatch_1_2()
    : ezGraphPatch("ezSpriteComponent", 2)
  {
  }

  virtual void Patch(ezGraphPatchContext& ref_context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override { pNode->RenameProperty("Max Screen Size", "MaxScreenSize"); }
};

ezSpriteComponentPatch_1_2 g_ezSpriteComponentPatch_1_2;



EZ_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_SpriteComponent);
