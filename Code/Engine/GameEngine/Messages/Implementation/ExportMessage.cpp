#include <GameEngine/GameEnginePCH.h>

#include <GameEngine/Messages/ExportMessage.h>

// clang-format off
EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgExport);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgExport, 1, ezRTTIDefaultAllocator<ezMsgExport>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("DocumentType", m_sDocumentType),
    EZ_MEMBER_PROPERTY("DocumentGuid", m_sDocumentGuid),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

EZ_STATICLINK_FILE(GameEngine, GameEngine_Messages_Implementation_ExportMessage);
