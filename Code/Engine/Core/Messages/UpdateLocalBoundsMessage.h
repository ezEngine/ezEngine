#pragma once

#include <Core/CoreDLL.h>
#include <Core/World/SpatialData.h>
#include <Foundation/Communication/Message.h>
#include <Foundation/Math/BoundingBoxSphere.h>

struct EZ_CORE_DLL ezMsgUpdateLocalBounds : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgUpdateLocalBounds, ezMessage);

  EZ_ALWAYS_INLINE void AddBounds(const ezBoundingBoxSphere& bounds, ezSpatialData::Category category)
  {
    m_ResultingLocalBounds.ExpandToInclude(bounds);
    m_uiSpatialDataCategoryBitmask |= category.GetBitmask();
  }

  ///\brief Enforces the object to be always visible. Note that you can't set this flag to false again,
  ///  because the same message is sent to multiple components and should accumulate the bounds.
  EZ_ALWAYS_INLINE void SetAlwaysVisible(ezSpatialData::Category category)
  {
    m_bAlwaysVisible = true;
    m_uiSpatialDataCategoryBitmask |= category.GetBitmask();
  }

private:
  friend class ezGameObject;

  ezBoundingBoxSphere m_ResultingLocalBounds;
  ezUInt32 m_uiSpatialDataCategoryBitmask = 0;
  bool m_bAlwaysVisible = false;
};
