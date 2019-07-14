#pragma once

#include <Core/World/Declarations.h>
#include <Foundation/Communication/Message.h>

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

struct EZ_CORE_DLL ezMsgComponentsChanged : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgComponentsChanged, ezMessage);

  enum class Type
  {
    ComponentAdded,
    ComponentRemoved
  };

  Type m_Type;
  ezGameObjectHandle m_hOwner;
  ezComponentHandle m_hComponent;
};
