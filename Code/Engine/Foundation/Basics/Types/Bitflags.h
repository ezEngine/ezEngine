#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Basics/Types/Enum.h>

// A bitfield build from enum values
// This has several advantages over the "normal" approach
// 1) It is typesafe
// 2) You can see from the code which enum contains the bit values
template <typename T>
struct ezBitflags
{
private:
  typedef typename T::Enum Enum;
  typedef typename T::Bits Bits;
  typedef typename T::StorageType StorageType;

public:
    
  /// constructor
  EZ_FORCE_INLINE ezBitflags() : m_value(0)
  {
  }

  /// implict conversion is OK here
  EZ_FORCE_INLINE ezBitflags(Enum flag1)
  {
    m_value = flag1;
  }

  /// Checks if certain flags are set within the bitfield
  EZ_FORCE_INLINE bool IsSet(Enum flag1) const
  {
    return (m_value & flag1) != 0;
  }

  EZ_FORCE_INLINE bool AreAllSet(const ezBitflags<T>& rh) const
  {
    return (m_value & rh.m_value) == rh.m_value;
  }

  EZ_FORCE_INLINE bool IsAnySet(const ezBitflags<T>& rh) const
  {
    return (m_value & rh.m_value) != 0;
  }

  /// Adds flags to the bitfield
  EZ_FORCE_INLINE void Add(Enum flag1)
  {
    m_value |= flag1;
  }

  /// Removes flags from the bitfield
  EZ_FORCE_INLINE void Remove(Enum flag1)
  {
    m_value &= (~flag1);
  }

  EZ_FORCE_INLINE void Toggle(Enum flag)
  {
    m_value ^= flag;
  }

  EZ_FORCE_INLINE void AddOrRemove(Enum flag, bool state)
  {
    m_value = (state) ? m_value | flag : m_value & (~flag);
  }

  inline ezBitflags<T> operator | (const ezBitflags<T>& rh) const
  {
    return ezBitflags<T>(m_value | rh.m_value);
  }

  /// Returns the stored value
  EZ_FORCE_INLINE StorageType GetValue() const
  {
    return m_value;
  }

private:
  EZ_FORCE_INLINE explicit ezBitflags(StorageType flags)
    : m_value(flags)
  {
  }

  union
  {
    StorageType m_value;
    Bits m_bits;
  };
};

/// Declares bitflags that can be used with the bitflag class above
/// Usage: EZ_DECLARE_FLAGS(ezUInt32, Name, Value1, Value2, Value3, Value4)
#define EZ_DECLARE_FLAGS(storage, name, ...)    \
struct name    \
  {    \
    static const ezUInt32 Count = EZ_VA_NUM_ARGS(__VA_ARGS__);    \
    typedef storage StorageType; \
    enum Enum    \
    {    \
      EZ_EXPAND_ARGS_WITH_INDEX EZ_PASS_VA(EZ_DECLARE_FLAGS_ENUM, __VA_ARGS__)    \
    };    \
    struct Bits    \
    {    \
      EZ_EXPAND_ARGS EZ_PASS_VA(EZ_DECLARE_FLAGS_BITS, __VA_ARGS__)    \
    };    \
    EZ_ENUM_TO_STRING(__VA_ARGS__) \
  };    \
  EZ_DECLARE_FLAGS_OR_OPERATOR(name)


#define EZ_DECLARE_FLAGS_ENUM(name, n)    name = (1 << n),
#define EZ_DECLARE_FLAGS_BITS(name)       StorageType name : 1;

#define EZ_DECLARE_FLAGS_OR_OPERATOR(name) \
  inline ezBitflags<name> operator|(name::Enum lhs, name::Enum rhs)    \
  {    \
    return (ezBitflags<name>(lhs) | ezBitflags<name>(rhs));    \
  }
