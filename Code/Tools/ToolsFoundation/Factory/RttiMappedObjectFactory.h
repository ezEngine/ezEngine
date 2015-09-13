#pragma once

#include <ToolsFoundation/Basics.h>
#include <Foundation/Reflection/Reflection.h>

/// \brief A factory that creates the closest matching objects according to the passed type.
///
/// Creators can be registered at the factory for a specific type.
/// When the create function is called for a type, the parent type hierarchy is traversed until
/// the first type is found for which a creator is registered.
template <typename Object>
class ezRttiMappedObjectFactory
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezRttiMappedObjectFactory);
public:
  ezRttiMappedObjectFactory();
  ~ezRttiMappedObjectFactory();

  typedef Object* (*CreateObjectFunc)(const ezRTTI* pType);

  ezResult RegisterCreator(const ezRTTI* pType, CreateObjectFunc creator);
  ezResult UnregisterCreator(const ezRTTI* pType);
  Object* CreateObject(const ezRTTI* pType);

  struct Event
  {
    enum class Type
    {
      CreatorAdded,
      CreatorRemoved
    };

    Type m_Type;
    const ezRTTI* m_pRttiType;
  };

  ezEvent<const Event&> m_Events;

private:
  ezHashTable<const ezRTTI*, CreateObjectFunc> m_Creators;
};

#include <ToolsFoundation/Factory/Implementation/RttiMappedObjectFactory_inl.h>
