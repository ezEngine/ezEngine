#pragma once

#include <Core/World/Declarations.h>
#include <Foundation/Communication/Message.h>

/// \brief Message sent when a game object's parent relationship changes.
///
/// Notifies components when their object is linked to or unlinked from a parent object.
struct EZ_CORE_DLL ezMsgParentChanged : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgParentChanged, ezMessage);

  enum class Type
  {
    ParentLinked,
    ParentUnlinked,
    Invalid
  };

  Type m_Type = Type::Invalid;
  ezGameObjectHandle m_hParent; // previous or new parent, depending on m_Type
};

/// \brief Message sent when a game object's children change.
///
/// Notifies parent objects when child objects are added or removed from their hierarchy.
struct EZ_CORE_DLL ezMsgChildrenChanged : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgChildrenChanged, ezMessage);

  enum class Type
  {
    ChildAdded,
    ChildRemoved
  };

  Type m_Type;
  ezGameObjectHandle m_hParent;
  ezGameObjectHandle m_hChild;
};

/// \brief Message sent when components are added to or removed from a game object.
///
/// Notifies interested parties when the component composition of an object changes.
struct EZ_CORE_DLL ezMsgComponentsChanged : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgComponentsChanged, ezMessage);

  enum class Type
  {
    ComponentAdded,
    ComponentRemoved,
    Invalid
  };

  Type m_Type = Type::Invalid;
  ezGameObjectHandle m_hOwner;
  ezComponentHandle m_hComponent;
};
