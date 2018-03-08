#include <PCH.h>
#include <RendererCore/Debug/DebugTextComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/View.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>

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
  EZ_END_PROPERTIES
    EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgExtractRenderData, OnExtractRenderData),
  }
  EZ_END_MESSAGEHANDLERS
    EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Gameplay"),
  }
  EZ_END_ATTRIBUTES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezDebugTextComponent::ezDebugTextComponent()
  : m_sText("Value0: {0}, Value1: {1}, Value2: {2}, Value3: {3}")
  , m_fValue0(0.0f)
  , m_fValue1(0.0f)
  , m_fValue2(0.0f)
  , m_fValue3(0.0f)
  , m_Color(ezColor::White)
{
}

ezDebugTextComponent::~ezDebugTextComponent()
{
}

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
  //const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

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
  if (msg.m_OverrideCategory != ezInvalidIndex || msg.m_pView->GetCameraUsageHint() == ezCameraUsageHint::Shadow)
    return;

  if (!m_sText.IsEmpty())
  {
    ezVec3 worldPos = GetOwner()->GetGlobalPosition();

    ezVec3 screenPos;
    if (msg.m_pView->ComputeScreenSpacePos(worldPos, screenPos).Succeeded())
    {
      ezStringBuilder sb;
      sb.Format(m_sText, m_fValue0, m_fValue1, m_fValue2, m_fValue3);

      ezHybridArray<ezStringView, 8> lines;
      sb.Split(false, lines, "\\n");

      int maxLineLength = 0;
      for (auto& line : lines)
      {
        maxLineLength = ezMath::Max<int>(maxLineLength, line.GetElementCount());
      }

      int screenPosX = (int)screenPos.x - maxLineLength * 4; // one character is 8 pixels wide and we want to center the text
      int screenPosY = (int)screenPos.y - lines.GetCount() * 20;

      for (auto& line : lines)
      {
        ezDebugRenderer::DrawText(GetWorld(), line, ezVec2I32(screenPosX, screenPosY), m_Color);

        screenPosY -= 20;
      }
    }
  }
}

