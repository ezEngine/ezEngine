#include <RendererCore/PCH.h>
#include <RendererCore/Components/SpriteComponent.h>
#include <RendererCore/Textures/TextureResource.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSpriteRenderData, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_COMPONENT_TYPE(ezSpriteComponent, 1)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Texture", GetTextureFile, SetTextureFile)->AddAttributes(new ezAssetBrowserAttribute("Texture 2D")),
    EZ_ACCESSOR_PROPERTY("Color", GetColor, SetColor),
    EZ_ACCESSOR_PROPERTY("Size", GetSize, SetSize)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant()), new ezDefaultValueAttribute(1.0f), new ezSuffixAttribute(" m")),
    EZ_ACCESSOR_PROPERTY("Max Screen Size", GetMaxScreenSize, SetMaxScreenSize)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant()), new ezDefaultValueAttribute(64.0f), new ezSuffixAttribute(" px")),
  }
  EZ_END_PROPERTIES
    EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Rendering"),
  }
  EZ_END_ATTRIBUTES
    EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezExtractRenderDataMessage, OnExtractRenderData),
  }
  EZ_END_MESSAGEHANDLERS
}
EZ_END_COMPONENT_TYPE

ezSpriteComponent::ezSpriteComponent()
  : m_Color(ezColor::White)
  , m_fSize(1.0f)
  , m_fMaxScreenSize(64.0f)
{
}

ezSpriteComponent::~ezSpriteComponent()
{
}

ezResult ezSpriteComponent::GetLocalBounds(ezBoundingBoxSphere& bounds)
{
  bounds = ezBoundingSphere(ezVec3::ZeroVector(), m_fSize * 0.5f);
  return EZ_SUCCESS;
}

void ezSpriteComponent::OnExtractRenderData(ezExtractRenderDataMessage& msg) const
{
  if (!m_hTexture.IsValid())
    return;

  const ezUInt32 uiTextureIDHash = m_hTexture.GetResourceIDHash();

  // Generate batch id from mode and texture
  ezUInt32 data[] = { 0, uiTextureIDHash };
  ezUInt32 uiBatchId = ezHashing::MurmurHash(data, sizeof(data));

  ezSpriteRenderData* pRenderData = ezCreateRenderDataForThisFrame<ezSpriteRenderData>(GetOwner(), uiBatchId);
  {
    pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
    pRenderData->m_GlobalBounds = GetOwner()->GetGlobalBounds();
    pRenderData->m_hTexture = m_hTexture;
    pRenderData->m_fSize = m_fSize;
    pRenderData->m_fMaxScreenSize = m_fMaxScreenSize;
    pRenderData->m_color = m_Color;
    pRenderData->m_texCoordScale = ezVec2(1.0f);
    pRenderData->m_texCoordOffset = ezVec2(0.0f);
    pRenderData->m_uiEditorPickingID = GetEditorPickingID();
  }

  // Determine render data category.
  ezRenderData::Category category;
  if (msg.m_OverrideCategory != ezInvalidIndex)
  {
    category = msg.m_OverrideCategory;
  }
  else
  {
    category = ezDefaultRenderDataCategories::LitOpaque;
  }

  // Sort by mode and then by texture
  ezUInt32 uiSortingKey = (0u << 31) | (uiTextureIDHash & 0x7FFFFFFF);
  msg.m_pExtractedRenderData->AddRenderData(pRenderData, category, uiSortingKey); 
}

void ezSpriteComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  ezStreamWriter& s = stream.GetStream();

  s << m_hTexture;
  s << m_Color;
  s << m_fSize;
  s << m_fMaxScreenSize;
}

void ezSpriteComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  ezStreamReader& s = stream.GetStream();

  s >> m_hTexture;
  s >> m_Color;
  s >> m_fSize;
  s >> m_fMaxScreenSize;
}

void ezSpriteComponent::SetTexture(const ezTextureResourceHandle& hTexture)
{
  m_hTexture = hTexture;
}

const ezTextureResourceHandle& ezSpriteComponent::GetTexture() const
{
  return m_hTexture;
}

void ezSpriteComponent::SetTextureFile(const char* szFile)
{
  ezTextureResourceHandle hTexture;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hTexture = ezResourceManager::LoadResource<ezTextureResource>(szFile);
  }

  SetTexture(hTexture);
}

const char* ezSpriteComponent::GetTextureFile() const
{
  if (!m_hTexture.IsValid())
    return "";

  return m_hTexture.GetResourceID();
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

  TriggerLocalBoundsUpdate(true);
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

void ezSpriteComponent::Initialize()
{
  /// \todo Do not show in orthographic viewports
  // This is a hacky solution that will effectively hide the entire node in the ortho viewport, which is not what we want
  // it would be nicer to not draw sprites in ortho mode in general
  // However, since the same shader is also used to render the shape icons,
  // this is not possible without splitting those two use cases up a bit
  {
    const ezTag* tagNoOrtho = ezTagRegistry::GetGlobalRegistry().RegisterTag("NotInOrthoMode");

    GetOwner()->GetTags().Set(*tagNoOrtho);
  }
}

