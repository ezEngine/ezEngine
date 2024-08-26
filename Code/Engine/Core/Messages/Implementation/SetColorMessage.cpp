#include <Core/CorePCH.h>

#include <Core/Messages/SetColorMessage.h>

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
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgSetCustomData);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgSetCustomData, 1, ezRTTIDefaultAllocator<ezMsgSetCustomData>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Data", m_vData),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void ezMsgSetColor::ModifyColor(ezColor& ref_color) const
{
  switch (m_Mode)
  {
    case ezSetColorMode::SetRGB:
      ref_color.SetRGB(m_Color.r, m_Color.g, m_Color.b);
      break;

    case ezSetColorMode::SetAlpha:
      ref_color.a = m_Color.a;
      break;

    case ezSetColorMode::AlphaBlend:
      ref_color = ezMath::Lerp(ref_color, m_Color, m_Color.a);
      break;

    case ezSetColorMode::Additive:
      ref_color += m_Color;
      break;

    case ezSetColorMode::Modulate:
      ref_color *= m_Color;
      break;

    case ezSetColorMode::SetRGBA:
    default:
      ref_color = m_Color;
      break;
  }
}

void ezMsgSetColor::ModifyColor(ezColorGammaUB& ref_color) const
{
  ezColor temp = ref_color;
  ModifyColor(temp);
  ref_color = temp;
}

void ezMsgSetColor::Serialize(ezStreamWriter& inout_stream) const
{
  inout_stream << m_Color;
  inout_stream << m_Mode;
}

void ezMsgSetColor::Deserialize(ezStreamReader& inout_stream, ezUInt8 uiTypeVersion)
{
  EZ_IGNORE_UNUSED(uiTypeVersion);

  inout_stream >> m_Color;
  inout_stream >> m_Mode;
}

//////////////////////////////////////////////////////////////////////////

void ezMsgSetCustomData::Serialize(ezStreamWriter& inout_stream) const
{
  inout_stream << m_vData;
}

void ezMsgSetCustomData::Deserialize(ezStreamReader& inout_stream, ezUInt8 uiTypeVersion)
{
  EZ_IGNORE_UNUSED(uiTypeVersion);

  inout_stream >> m_vData;
}



EZ_STATICLINK_FILE(Core, Core_Messages_Implementation_SetColorMessage);
