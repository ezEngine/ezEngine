#pragma once

#include <Foundation/Communication/Message.h>
#include <Foundation/Math/BoundingBoxSphere.h>

#include <Core/World/Declarations.h>

struct EZ_CORE_DLL ezUpdateLocalBoundsMessage : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezUpdateLocalBoundsMessage);

  ezBoundingBoxSphere m_ResultingLocalBounds;
};

struct EZ_CORE_DLL ezDeleteObjectMessage : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezDeleteObjectMessage);
};

/// \brief A generic 'trigger' message to be used by different components to trigger their custom behavior.
///
/// Many components simply need to 'do their thing in x seconds'. This message can be used if only the timeout is
/// required and no further data is needed.
struct EZ_CORE_DLL ezComponentTriggerMessage : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezComponentTriggerMessage);

  /// Must be used to filter out messages meant for other components.
  ezComponentHandle m_hTargetComponent;
};
