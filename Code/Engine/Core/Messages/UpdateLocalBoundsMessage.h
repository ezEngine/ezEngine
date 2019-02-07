#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Communication/Message.h>
#include <Foundation/Math/BoundingBoxSphere.h>

struct EZ_CORE_DLL ezMsgUpdateLocalBounds : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgUpdateLocalBounds, ezMessage);

  EZ_ALWAYS_INLINE void AddBounds(const ezBoundingBoxSphere& bounds) { m_ResultingLocalBounds.ExpandToInclude(bounds); }

  ///\brief Enforces the object to be always visible. Note that you can't set this flag to false again,
  ///  because the same message is sent to multiple components and should accumulate the bounds.
  EZ_ALWAYS_INLINE void SetAlwaysVisible() { m_bAlwaysVisible = true; }

private:
  friend class ezGameObject;

  ezBoundingBoxSphere m_ResultingLocalBounds;
  bool m_bAlwaysVisible = false;
};

