#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Basics/Types/Enum.h>

// \brief A bitfield build from enum values
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
    
  /// \brief Constructor.
  EZ_FORCE_INLINE ezBitflags() : m_value(0) // [tested]
  {
  }

  /// \brief Implict conversion is OK here
  EZ_FORCE_INLINE ezBitflags(Enum flag1) // [tested]
  {
    m_value = flag1;
  }

  /// \brief Checks if certain flags are set within the bitfield.
  EZ_FORCE_INLINE bool IsSet(Enum flag1) const // [tested]
  {
    return (m_value & flag1) != 0;
  }
  
  /// \brief Returns whether all the given flags are set.
  EZ_FORCE_INLINE bool AreAllSet(const ezBitflags<T>& rh) const // [tested]
  {
    return (m_value & rh.m_value) == rh.m_value;
  }

  /// \brief  Returns whether any of the given flags is set.
  EZ_FORCE_INLINE bool IsAnySet(const ezBitflags<T>& rh) const // [tested]
  {
    return (m_value & rh.m_value) != 0;
  }

  /// \brief Sets the given flag.
  EZ_FORCE_INLINE void Add(Enum flag1) // [tested]
  {
    m_value |= flag1;
  }

  /// \brief Removes the given flag.
  EZ_FORCE_INLINE void Remove(Enum flag1) // [tested]
  {
    m_value &= (~flag1);
  }

  /// \brief Toggles the state of the given flag.
  EZ_FORCE_INLINE void Toggle(Enum flag) // [tested]
  {
    m_value ^= flag;
  }

  /// \brief Sets or clears the given flag.
  EZ_FORCE_INLINE void AddOrRemove(Enum flag, bool state) // [tested]
  {
    m_value = (state) ? m_value | flag : m_value & (~flag);
  }

  /// \brief Returns an object that has the flags of \a this and \a rhs combined.
  inline ezBitflags<T> operator | (const ezBitflags<T>& rhs) const // [tested]
  {
    return ezBitflags<T>(m_value | rhs.m_value);
  }

  /// \brief Returns the stored value as the underlying integer type.
  EZ_FORCE_INLINE StorageType GetValue() const // [tested]
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

/// \brief Declares bitflags that can be used with the bitflag class above.
///
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
