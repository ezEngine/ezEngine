#pragma once

#include <Foundation/Communication/Message.h>
#include <GameEngine/GameEngineDLL.h>

/// \brief Message that is sent to all game objects when a scene or prefab is being exported.
/// This message can be handled in scripts or custom components to e.g. remove editor only objects/components or save custom data.
struct EZ_GAMEENGINE_DLL ezMsgExport : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgExport, ezMessage);

  ezString m_sDocumentType; ///< The type of document that is being exported, e.g. "Prefab", "Scene", etc.
  ezString m_sDocumentGuid; ///< The GUID (as string) of the document that is being exported.
};
