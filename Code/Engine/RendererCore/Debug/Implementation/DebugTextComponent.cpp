#include <PCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Debug/DebugTextComponent.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/View.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezDebugTextComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Text", m_sText)->AddAttributes(new ezDefaultValueAttribute("Value0: {0}, Value1: {1}, Value2: {2}, Value3: {3}")),
    EZ_MEMBER_PROPERTY("Value0", m_fValue0),
    EZ_MEMBER_PROPERTY("Value1", m_fValue1),
    EZ_MEMBER_PROPERTY("Value2", m_fValue2),
    EZ_MEMBER_PROPERTY("Value3", m_fValue3),
    EZ_MEMBER_PROPERTY("Color", m_Color),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgExtractRenderData, OnExtractRenderData),
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Gameplay"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezDebugTextComponent::ezDebugTextComponent()
    : m_sText("Value0: {0}, Value1: {1}, Value2: {2}, Value3: {3}")
    , m_fValue0(0.0f)
    , m_fValue1(0.0f)
    , m_fValue2(0.0f)
    , m_fValue3(0.0f)
    , m_Color(ezColor::White)
{
}

ezDebugTextComponent::~ezDebugTextComponent() {}

void ezDebugTextComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_sText;
  s << m_fValue0;
  s << m_fValue1;
  s << m_fValue2;
  s << m_fValue3;
  s << m_Color;
}

void ezDebugTextComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  s >> m_sText;
  s >> m_fValue0;
  s >> m_fValue1;
  s >> m_fValue2;
  s >> m_fValue3;
  s >> m_Color;
}

void ezDebugTextComponent::OnExtractRenderData(ezMsgExtractRenderData& msg) const
{
  if (msg.m_OverrideCategory != ezInvalidRenderDataCategory || msg.m_pView->GetCameraUsageHint() == ezCameraUsageHint::Shadow)
    return;

  if (!m_sText.IsEmpty())
  {
    ezStringBuilder sb;
    sb.Format(m_sText, m_fValue0, m_fValue1, m_fValue2, m_fValue3);

    ezDebugRenderer::Draw3DText(msg.m_pView->GetHandle(), sb, GetOwner()->GetGlobalPosition(), m_Color, 16,
                                ezDebugRenderer::HorizontalAlignment::Center, ezDebugRenderer::VerticalAlignment::Bottom);
  }
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_Debug_Implementation_DebugTextComponent);
