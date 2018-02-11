#include <PCH.h>
#include <RendererCore/Messages/SetColorMessage.h>

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezSetColorMode, 1)
EZ_ENUM_CONSTANTS(ezSetColorMode::SetAll, ezSetColorMode::SetRGB, ezSetColorMode::SetAlpha, ezSetColorMode::AlphaBlend, ezSetColorMode::Additive, ezSetColorMode::Modulate)
EZ_END_STATIC_REFLECTED_ENUM();

EZ_IMPLEMENT_MESSAGE_TYPE(ezSetColorMessage);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSetColorMessage, 1, ezRTTIDefaultAllocator<ezSetColorMessage>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Color", m_Color),
    EZ_ENUM_MEMBER_PROPERTY("Mode", ezSetColorMode, m_Mode)
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

void ezSetColorMessage::ModifyColor(ezColor& color) const
{
  switch (m_Mode)
  {
  case ezSetColorMode::SetRGB:
    color.SetRGB(m_Color.r, m_Color.g, m_Color.b);
    break;

  case ezSetColorMode::SetAlpha:
    color.a = m_Color.a;
    break;

  case ezSetColorMode::AlphaBlend:
    color = ezMath::Lerp(color, m_Color, m_Color.a);
    break;

  case ezSetColorMode::Additive:
    color += m_Color;
    break;

  case ezSetColorMode::Modulate:
    color *= m_Color;
    break;

  case ezSetColorMode::SetAll:
  default:
    color = m_Color;
    break;
  }
}

void ezSetColorMessage::ModifyColor(ezColorGammaUB& color) const
{
  ezColor temp = color;
  ModifyColor(temp);
  color = temp;
}
