#pragma once

#include <Core/World/Declarations.h>
#include <Foundation/Communication/Message.h>

/// \brief Message sent when a game object's global transform changes.
///
/// Contains both the old and new global transforms, allowing components to respond
/// to position, rotation, or scale changes and calculate movement deltas if needed.
struct EZ_CORE_DLL ezMsgTransformChanged : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgTransformChanged, ezMessage);

  ezTransform m_OldGlobalTransform;
  ezTransform m_NewGlobalTransform;
};
