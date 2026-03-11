#pragma once

#include <Foundation/Reflection/Reflection.h>

/// Pin for connecting procedural generation nodes.
///
/// Pins represent input or output connections on ProcGen nodes. They are used to pass
/// values like density, position, color, or other data between nodes in the procedural generation graph.
struct ezProcGenNodePin
{
  EZ_DECLARE_POD_TYPE();

  struct Type
  {
    using StorageType = ezUInt8;

    enum Enum
    {
      Input = EZ_BIT(0),  ///< Pin accepts input from other nodes.
      Output = EZ_BIT(1), ///< Pin provides output to other nodes.

      Default = 0
    };

    struct Bits
    {
      StorageType Input : 1;
      StorageType Output : 1;
    };
  };

  ezBitflags<Type> m_Type;
  ezUInt8 m_uiInputIndex = 0xFF;
  ezUInt8 m_uiOutputIndex = 0xFF;
  class ezProcGenNodeBase* m_pParent = nullptr;
};
EZ_DECLARE_FLAGS_OPERATORS(ezProcGenNodePin::Type);

/// Input pin for receiving data from other procedural generation nodes.
struct ezProcGenNodeInputPin : public ezProcGenNodePin
{
  EZ_DECLARE_POD_TYPE();

  EZ_ALWAYS_INLINE ezProcGenNodeInputPin() { m_Type = Type::Input; }
};

/// Output pin for sending data to other procedural generation nodes.
struct ezProcGenNodeOutputPin : public ezProcGenNodePin
{
  EZ_DECLARE_POD_TYPE();

  EZ_ALWAYS_INLINE ezProcGenNodeOutputPin() { m_Type = Type::Output; }
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezProcGenNodePin);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezProcGenNodeInputPin);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezProcGenNodeOutputPin);
