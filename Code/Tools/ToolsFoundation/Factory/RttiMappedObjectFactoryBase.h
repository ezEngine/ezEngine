#pragma once

#include <ToolsFoundation/Basics.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Communication/Event.h>

/// \brief A factory that creates the closest matching objects according to the passed type.
///
/// Creators can be registered at the factory for a specific type.
/// When the create function is called for a type, the parent type hierarchy is traversed until
/// the first type is found for which a creator is registered.
template <typename RttiType, typename Object, typename TypeTraverser>
struct ezRttiMappedObjectFactoryBase
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezRttiMappedObjectFactoryBase);
public:
  ezRttiMappedObjectFactoryBase();
  ~ezRttiMappedObjectFactoryBase();

  typedef Object* (*CreateObjectFunc)(const RttiType type);
  //struct Creator
  //{
  //  EZ_DECLARE_POD_TYPE();
  //  CreateObjectFunc m_CreateObject;
  //};

  static ezResult RegisterCreator(RttiType type, CreateObjectFunc creator);
  static ezResult UnregisterCreator(RttiType type);
  static Object* CreateObject(RttiType type);

  struct Event
  {
    enum class Type
    {
      CreatorAdded,
      CreatorRemoved
    };

    Type m_Type;
    RttiType m_RttiType;
  };

  static ezEvent<const Event&> s_Events;

private:
  static ezHashTable<RttiType, CreateObjectFunc> s_Creators;
};

#include <ToolsFoundation/Factory/Implementation/RttiMappedObjectFactoryBase_inl.h>
