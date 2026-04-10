#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <ToolsFoundation/VisualGraph/VisualGraphCommentNode.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualGraphComment, 1, ezRTTIDefaultAllocator<ezVisualGraphComment>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Comment", m_sComment)->AddAttributes(new ezDefaultValueAttribute("Comment")),
    EZ_MEMBER_PROPERTY("Size", m_vSize),
    EZ_MEMBER_PROPERTY("Color", m_Color)->AddAttributes(new ezDefaultValueAttribute(ezColorScheme::DarkUI(ezColorScheme::Gray))),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Misc"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
