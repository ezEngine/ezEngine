#pragma once


#include <Core/Basics.h>
#include <Foundation/Communication/Message.h>
#include <Foundation/Math/BoundingBoxSphere.h>

struct EZ_CORE_DLL ezUpdateLocalBoundsMessage : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezUpdateLocalBoundsMessage);

  ezBoundingBoxSphere m_ResultingLocalBounds;
};
