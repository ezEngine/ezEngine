#pragma once

#include <Core/World/Declarations.h>
#include <Foundation/Communication/Message.h>

/// \brief Message used to restrict an operation to apply only to a specific game object.
///
/// This message carries a game object handle to specify which object should be affected
/// by a particular operation, allowing selective application of effects or behaviors.
struct EZ_CORE_DLL ezMsgOnlyApplyToObject : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgOnlyApplyToObject, ezMessage);

  ezGameObjectHandle m_hObject;
};
