#include <RendererCore/RendererCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Debug/DebugTextComponent.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/View.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezDebugTextComponent, 2, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Text", m_sText)->AddAttributes(new ezDefaultValueAttribute("Value0: {0}, Value1: {1}, Value2: {2}, Value3: {3}")),
    EZ_MEMBER_PROPERTY("Value0", m_fValue0),
    EZ_MEMBER_PROPERTY("Value1", m_fValue1),
    EZ_MEMBER_PROPERTY("Value2", m_fValue2),
    EZ_MEMBER_PROPERTY("Value3", m_fValue3),
    EZ_MEMBER_PROPERTY("Color", m_Color),
    EZ_MEMBER_PROPERTY("MaxDistance", m_fMaxDistance)->AddAttributes(new ezDefaultValueAttribute(10.0f), new ezClampValueAttribute(0.0f, ezVariant())),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgExtractRenderData, OnMsgExtractRenderData),
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Utilities/Debug"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezDebugTextComponent::ezDebugTextComponent()
  : m_sText("Value0: {0}, Value1: {1}, Value2: {2}, Value3: {3}")
  , m_Color(ezColor::White)
{
}

ezDebugTextComponent::~ezDebugTextComponent() = default;

void ezDebugTextComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_sText;
  s << m_fValue0;
  s << m_fValue1;
  s << m_fValue2;
  s << m_fValue3;
  s << m_Color;
  s << m_fMaxDistance;
}

void ezDebugTextComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_sText;
  s >> m_fValue0;
  s >> m_fValue1;
  s >> m_fValue2;
  s >> m_fValue3;
  s >> m_Color;

  if (uiVersion >= 2)
  {
    s >> m_fMaxDistance;
  }
}

void ezDebugTextComponent::OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const
{
  if (msg.m_OverrideCategory != ezInvalidRenderDataCategory || msg.m_pView->GetCameraUsageHint() == ezCameraUsageHint::Shadow)
    return;

  if (m_sText.IsEmpty())
    return;

  const float fSquaredDistance = msg.m_pView->GetCullingCamera()->GetCenterPosition().GetSquaredDistanceTo(GetOwner()->GetGlobalPosition());
  if (fSquaredDistance > m_fMaxDistance * m_fMaxDistance)
    return;

  const float fFade = ezMath::Saturate(ezMath::Sqrt(fSquaredDistance) / ezMath::Max(m_fMaxDistance, 0.0001f) * -5.0f + 5.0f);

  ezColor c = m_Color;
  c.a *= fFade;

  ezStringBuilder sb;
  sb.SetFormat(m_sText, m_fValue0, m_fValue1, m_fValue2, m_fValue3);

  ezDebugRenderer::Draw3DText(msg.m_pView->GetHandle(), sb, GetOwner()->GetGlobalPosition(), c);
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_Debug_Implementation_DebugTextComponent);
