#include <RmlUiPlugin/RmlUiPluginPCH.h>

#include <RmlUiPlugin/Components/RmlUiCanvas2DComponent.h>
#include <RmlUiPlugin/Implementation/BlackboardDataBinding.h>
#include <RmlUiPlugin/Implementation/RmlUiRenderData.h>
#include <RmlUiPlugin/RmlUiContext.h>
#include <RmlUiPlugin/RmlUiSingleton.h>

#include <Core/Input/InputManager.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Gameplay/BlackboardComponent.h>
#include <RendererCore/Pipeline/RenderDataManager.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Texture.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezRmlUiCanvas2DComponent, 5, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("AnchorPoint", GetAnchorPoint, SetAnchorPoint)->AddAttributes(new ezClampValueAttribute(ezVec2(0), ezVec2(1))),
    EZ_ACCESSOR_PROPERTY("Size", GetSize, SetSize)->AddAttributes(new ezSuffixAttribute("px"), new ezMinValueTextAttribute("Auto")),
    EZ_ACCESSOR_PROPERTY("Offset", GetOffset, SetOffset)->AddAttributes(new ezDefaultValueAttribute(ezVec2::MakeZero()), new ezSuffixAttribute("px")),
    EZ_ACCESSOR_PROPERTY("CustomScale", GetCustomScale, SetCustomScale)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.1f, 10.0f)),
    EZ_ACCESSOR_PROPERTY("PassInput", GetPassInput, SetPassInput)->AddAttributes(new ezDefaultValueAttribute(true)),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Input/RmlUi"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezRmlUiCanvas2DComponent::ezRmlUiCanvas2DComponent() = default;
ezRmlUiCanvas2DComponent::~ezRmlUiCanvas2DComponent() = default;
ezRmlUiCanvas2DComponent& ezRmlUiCanvas2DComponent::operator=(ezRmlUiCanvas2DComponent&& rhs) = default;

void ezRmlUiCanvas2DComponent::OnActivated()
{
  SUPER::OnActivated();

  GetOrCreateRmlContext()->ShowDocument();

  // Update once to ensure correct initial state
  Update();
}

void ezRmlUiCanvas2DComponent::Deinitialize()
{
  SUPER::Deinitialize();

  ezGALDevice::GetDefaultDevice()->DestroyTexture(m_hTexture);
}

void ezRmlUiCanvas2DComponent::Update()
{
  if (m_pContext == nullptr)
    return;

  ezVec2 viewSize = ezVec2::MakeZero();
  m_bNeedsUpdate |= UpdateSizeOffsetAndTexture(viewSize);

  if (m_bPassInput && GetWorld()->GetWorldSimulationEnabled())
  {
    ezVec2 mousePos;
    ezInputManager::GetInputSlotState(ezInputSlot_MousePositionX, &mousePos.x);
    ezInputManager::GetInputSlotState(ezInputSlot_MousePositionY, &mousePos.y);

    mousePos = mousePos.CompMul(viewSize) - m_vFinalOffset;
    ReceiveInput(mousePos, ezRmlUiInputSnapshot::MakeFromCurrentInput());
  }

  SUPER::Update();
}

void ezRmlUiCanvas2DComponent::SetOffset(const ezVec2I32& vOffset)
{
  m_vOffset = vOffset;
}

void ezRmlUiCanvas2DComponent::SetSize(const ezVec2U32& vSize)
{
  if (m_vSize != vSize)
  {
    m_vSize = vSize;

    if (m_pContext != nullptr)
    {
      m_pContext->SetSize(m_vSize);
    }
  }
}

void ezRmlUiCanvas2DComponent::SetAnchorPoint(const ezVec2& vAnchorPoint)
{
  m_vAnchorPoint = vAnchorPoint;
}

void ezRmlUiCanvas2DComponent::SetPassInput(bool bPassInput)
{
  m_bPassInput = bPassInput;
}

void ezRmlUiCanvas2DComponent::SetCustomScale(float fScale)
{
  m_fCustomScale = fScale;
}

void ezRmlUiCanvas2DComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  ezStreamWriter& s = inout_stream.GetStream();

  s << m_vOffset;
  s << m_vSize;
  s << m_vAnchorPoint;
  s << m_bPassInput;
  s << m_fCustomScale;
}

void ezRmlUiCanvas2DComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  if (uiVersion >= 5)
  {
    SUPER::DeserializeComponent(inout_stream);
    ezStreamReader& s = inout_stream.GetStream();

    s >> m_vOffset;
    s >> m_vSize;
    s >> m_vAnchorPoint;
    s >> m_bPassInput;
    s >> m_fCustomScale;
  }
  else
  {
    ezRenderComponent::DeserializeComponent(inout_stream);
    ezStreamReader& s = inout_stream.GetStream();

    s >> m_hResource;
    s >> m_vOffset;
    s >> m_vSize;
    s >> m_vAnchorPoint;
    s >> m_bPassInput;

    if (uiVersion >= 2)
    {
      s >> m_bAutobindBlackboards;
    }

    if (uiVersion >= 3)
    {
      s >> m_bOnDemandUpdate;
    }

    if (uiVersion >= 4)
    {
      s >> m_fCustomScale;
    }
  }
}

void ezRmlUiCanvas2DComponent::OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const
{
  // Don't extract render data for selection.
  if (msg.m_OverrideCategory != ezInvalidRenderDataCategory || m_hTexture.IsInvalidated())
    return;

  if (m_pContext != nullptr)
  {
    // The texture is also used in shadow maps as we don't know what the depth shader compiler is culling or what the material is using the texture for.
    ezRenderWorld::AddViewDependency(*msg.m_pView, m_hTexture, ezGALResourceState::ShaderResource, ezGALShaderStageFlags::PixelShader);
    if (msg.m_pView->GetCameraUsageHint() != ezCameraUsageHint::MainView && msg.m_pView->GetCameraUsageHint() != ezCameraUsageHint::EditorView && msg.m_pView->GetCameraUsageHint() != ezCameraUsageHint::Thumbnail)
      return;

    ezRmlUi::GetSingleton()->ExtractContext(*m_pContext, m_hTexture);

    auto pRenderData = msg.m_pRenderDataManager->CreateRenderDataForThisFrame<ezRmlUiRenderData>(GetOwner());
    pRenderData->m_hTexture = m_hTexture;
    pRenderData->m_vOffset = m_vFinalOffset;

    msg.AddRenderData(pRenderData, ezDefaultRenderDataCategories::GUI, ezRenderData::Caching::Never);
  }
}

bool ezRmlUiCanvas2DComponent::UpdateSizeOffsetAndTexture(ezVec2& out_viewSize)
{
  out_viewSize = ezVec2(1.0f);
  if (ezView* pView = ezRenderWorld::GetViewByUsageHint(ezCameraUsageHint::MainView, ezCameraUsageHint::EditorView, GetWorld()))
  {
    out_viewSize.x = pView->GetViewport().width;
    out_viewSize.y = pView->GetViewport().height;
  }

  float fScale = 1.0f;
  if (m_vReferenceResolution.x > 0 && m_vReferenceResolution.y > 0)
  {
    fScale = out_viewSize.y / m_vReferenceResolution.y;
  }

  ezVec2 size = ezVec2(static_cast<float>(m_vSize.x), static_cast<float>(m_vSize.y)) * fScale;
  if (size.x <= 0.0f)
  {
    size.x = out_viewSize.x;
  }
  if (size.y <= 0.0f)
  {
    size.y = out_viewSize.y;
  }
  const ezVec2U32 sizeU32 = ezVec2U32(static_cast<ezUInt32>(size.x), static_cast<ezUInt32>(size.y));
  if (sizeU32.IsZero())
    return false;

  m_pContext->SetSize(sizeU32);
  m_pContext->SetDpiScale(fScale * m_fCustomScale);

  ezVec2 offset = ezVec2(static_cast<float>(m_vOffset.x), static_cast<float>(m_vOffset.y)) * fScale;
  offset = (out_viewSize - size).CompMul(m_vAnchorPoint) - offset.CompMul(m_vAnchorPoint * 2.0f - ezVec2(1.0f));
  m_vFinalOffset.x = ezMath::Round(offset.x);
  m_vFinalOffset.y = ezMath::Round(offset.y);

  // Recreate texture if necessary
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  const ezGALTexture* pTexture = pDevice->GetTexture(m_hTexture);
  if (pTexture == nullptr || pTexture->GetDescription().m_uiWidth != sizeU32.x || pTexture->GetDescription().m_uiHeight != sizeU32.y)
  {
    if (pTexture != nullptr)
    {
      pDevice->DestroyTexture(m_hTexture);
    }

    ezGALTextureCreationDescription desc;
    desc.m_uiWidth = sizeU32.x;
    desc.m_uiHeight = sizeU32.y;
    desc.m_Format = ezGALResourceFormat::RGBAUByteNormalized;
    desc.m_ResourceAccess.m_bImmutable = false;

    m_hTexture = pDevice->CreateTexture(desc);

    return true;
  }

  return false;
}


EZ_STATICLINK_FILE(RmlUiPlugin, RmlUiPlugin_Components_Implementation_RmlUiCanvas2DComponent);
