#include <PCH.h>

#include <RendererCore/Messages/SetColorMessage.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezSetColorMode, 1)
EZ_ENUM_CONSTANTS(ezSetColorMode::SetRGBA, ezSetColorMode::SetRGB, ezSetColorMode::SetAlpha, ezSetColorMode::AlphaBlend, ezSetColorMode::Additive, ezSetColorMode::Modulate)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgSetColor);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgSetColor, 1, ezRTTIDefaultAllocator<ezMsgSetColor>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Color", m_Color),
    EZ_ENUM_MEMBER_PROPERTY("Mode", ezSetColorMode, m_Mode)
  }
  EZ_END_PROPERTIES;

  EZ_BEGIN_ATTRIBUTES
  {
    new ezAutoGenVisScriptMsgSender,
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void ezMsgSetColor::ModifyColor(ezColor& color) const
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

    case ezSetColorMode::SetRGBA:
    default:
      color = m_Color;
      break;
  }
}

void ezMsgSetColor::ModifyColor(ezColorGammaUB& color) const
{
  ezColor temp = color;
  ModifyColor(temp);
  color = temp;
}

void ezMsgSetColor::Serialize(ezStreamWriter& stream) const
{
  stream << m_Color;
  stream << m_Mode;
}

void ezMsgSetColor::Deserialize(ezStreamReader& stream, ezUInt8 uiTypeVersion)
{
  stream >> m_Color;
  stream >> m_Mode;
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_Messages_Implementation_SetColorMessage);
