#include <RmlUiPlugin/RmlUiPluginPCH.h>

#include <RmlUiPlugin/Components/RmlUiMessages.h>

// clang-format off
EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgRmlUiReload);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgRmlUiReload, 1, ezRTTIDefaultAllocator<ezMsgRmlUiReload>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezExcludeFromScript(),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgRmlUiEvent);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgRmlUiEvent, 1, ezRTTIDefaultAllocator<ezMsgRmlUiEvent>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Identifier", m_sIdentifier),
    EZ_MEMBER_PROPERTY("Type", m_sType),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

EZ_STATICLINK_FILE(RmlUiPlugin, RmlUiPlugin_Components_Implementation_RmlUiMessages);
