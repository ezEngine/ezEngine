#pragma once

#include <Foundation/Basics/Types/Bitflags.h>
#include <Foundation/Basics/Types/Id.h>
#include <Foundation/Math/Mat4.h>
#include <Foundation/Math/Quat.h>

#include <Core/Basics.h>

class ezWorld;
class ezGameObject;
struct ezGameObjectDesc;

class ezComponentManagerBase;
class ezComponent;

struct ezGameObjectId
{
  typedef ezUInt32 StorageType;

  EZ_DECLARE_ID_TYPE(ezGameObjectId, 20);

  EZ_FORCE_INLINE ezGameObjectId(StorageType instanceIndex, StorageType generation, 
    StorageType worldIndex = 0)
  {
    m_Data = 0;
    m_InstanceIndex = instanceIndex;
    m_Generation = generation;
    m_WorldIndex = worldIndex;
  }

  union
  {
    StorageType m_Data;
    struct
    {
      StorageType m_InstanceIndex : 20;
      StorageType m_Generation : 6;
      StorageType m_WorldIndex : 6;
    };
  };
};

class ezGameObjectHandle
{
  EZ_DECLARE_HANDLE_TYPE(ezGameObjectHandle, ezGameObjectId);

  friend class ezWorld;
  friend class ezGameObject;
};

typedef ezGenericId<24, 8> ezGenericComponentId;

struct ezComponentId : public ezGenericComponentId
{
  EZ_FORCE_INLINE ezComponentId() : ezGenericComponentId()
  {
  }

  EZ_FORCE_INLINE ezComponentId(StorageType instanceIndex, StorageType generation, ezUInt16 typeId = 0) : 
    ezGenericComponentId(instanceIndex, generation)
  {
    m_TypeId = typeId;
  }

  EZ_FORCE_INLINE ezComponentId(ezGenericComponentId genericId, ezUInt16 typeId) : 
    ezGenericComponentId(genericId)
  {
    m_TypeId = typeId;
  }

  ezUInt16 m_TypeId;
};

class ezComponentHandle
{
  EZ_DECLARE_HANDLE_TYPE(ezComponentHandle, ezComponentId);

  friend class ezWorld;
  friend class ezComponentManagerBase;
  friend class ezComponent;
};
