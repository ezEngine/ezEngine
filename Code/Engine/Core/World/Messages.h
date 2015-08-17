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
