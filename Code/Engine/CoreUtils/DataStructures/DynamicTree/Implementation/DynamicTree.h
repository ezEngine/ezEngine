#pragma once

#include <CoreUtils/Basics.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Math/BoundingBox.h>
#include <Foundation/Math/Frustum.h>

struct ezDynamicTree
{
  struct ezObjectData
  {
    ezInt32 m_iObjectType;
    ezInt32 m_iObjectInstance;
  };

  struct ezMultiMapKey
  {
    ezUInt32 m_uiKey;
    ezUInt32 m_uiCounter;

    ezMultiMapKey()
    {
      m_uiKey = 0;
      m_uiCounter = 0;
    }

    inline bool operator<(const ezMultiMapKey& rhs) const
    {
      if (m_uiKey == rhs.m_uiKey)
        return m_uiCounter < rhs.m_uiCounter;

      return m_uiKey < rhs.m_uiKey;
    }

    inline bool operator==(const ezMultiMapKey& rhs) const
    {
      return (m_uiCounter == rhs.m_uiCounter && m_uiKey == rhs.m_uiKey);
    }
  };
};

typedef ezMap<ezDynamicTree::ezMultiMapKey, ezDynamicTree::ezObjectData>::Iterator ezDynamicTreeObject;
typedef ezMap<ezDynamicTree::ezMultiMapKey, ezDynamicTree::ezObjectData>::ConstIterator ezDynamicTreeObjectConst;

/// \brief Callback type for object queries. Return "false" to abort a search (e.g. when the desired element has been found).
typedef bool (*EZ_VISIBLE_OBJ_CALLBACK) (void* pPassThrough, ezDynamicTreeObjectConst Object);

class ezDynamicOctree;
class ezDynamicQuadtree;



