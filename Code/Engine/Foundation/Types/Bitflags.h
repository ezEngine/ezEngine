#pragma once

/// \file

#include <Foundation/Basics.h>
#include <Foundation/Containers/Implementation/BitIterator.h>
#include <Foundation/Types/Enum.h>

/// \brief The ezBitflags class allows you to work with type-safe bitflags.
///
/// ezBitflags takes a struct as its template parameter, which contains an enum for the available flag values.
/// ezBitflags wraps this type in a way which enables the compiler to do type-checks. This makes it very easy
/// to document and enforce what flags are to be used in an interface.
/// For example, in traditional C++ code, you usually need to have an integer as a function parameter type,
/// when that parameter is supposed to take flags. However, WHICH flags (e.g. from which enum) cannot be enforced
/// through compile time checks. Thus it is difficult for the user to see whether he used the correct type, and
/// it is impossible for the compiler to help find such bugs.
/// ezBitflags solves this problem. However the flag type used to instantiate ezBitflags must fulfill some requirements.
///
/// There are two ways to define your bitflags type, that can be used with ezBitflags.
///
/// The easier, less powerful way: Use the EZ_DECLARE_FLAGS() macro.\n
/// Example:\n
/// \code{.cpp}
///   EZ_DECLARE_FLAGS(ezUInt8, SimpleRenderFlags, EnableEffects, EnableLighting, EnableShadows);
/// \endcode
/// This will declare a type 'SimpleRenderFlags' which contains three different flags.
/// You can then create a function which takes flags like this:
/// \code{.cpp}
///   void RenderScene(ezBitflags<SimpleRenderFlags> Flags);
/// \endcode
/// And this function can be called like this:\n
/// \code{.cpp}
///   RenderScene(EnableEffects | EnableLighting | EnableShadows);
/// \endcode
/// However it will refuse to compile with anything else, for example this will not work:\n
/// \code{.cpp}
///   RenderScene(1);
/// \endcode
///
/// The second way to declare your bitflags type allows even more flexibility. Here you need to declare your bitflag type manually:
/// \code{.cpp}
///   struct SimpleRenderFlags
///   {
///     using StorageType = ezUInt32;
///
///     enum Enum
///     {
///       EnableEffects   = EZ_BIT(0),
///       EnableLighting  = EZ_BIT(1),
///       EnableShadows   = EZ_BIT(2),
///       FullLighting    = EnableLighting | EnableShadows,
///       AllFeatures     = FullLighting | EnableEffects,
///       Default = AllFeatures
///     };
///
///     struct Bits
///     {
///       StorageType EnableEffects   : 1;
///       StorageType EnableLighting  : 1;
///       StorageType EnableShadows   : 1;
///     };
///   };
///
///   EZ_DECLARE_FLAGS_OPERATORS(SimpleRenderFlags);
/// \endcode
///
/// Here we declare a struct which contains our enum that contains all the flags that we want to have. This enum can contain
/// flags that are combinations of other flags. Note also the 'Default' flag, which is mandatory.\n
/// The 'Bits' struct enables debuggers to show exactly which flags are enabled (with nice names) when you inspect an ezBitflags
/// instance. You could leave this struct empty, but then your debugger can not show helpful information about the flags anymore.
/// The Bits struct should contain one named entry for each individual bit. E.g. here only the flags 'EnableEffects', 'EnableLighting'
/// and 'EnableShadows' actually map to single bits, the other flags are combinations of those. Therefore the Bits struct only
/// specifies names for those first three Bits.\n
/// The typedef 'StorageType' is also mandatory, such that ezBitflags can access it.\n
/// Finally the macro EZ_DECLARE_FLAGS_OPERATORS will define the required operator to be able to combine bitflags of your type.
/// I.e. it enables to write ezBitflags<SimpleRenderFlags> f = EnableEffects | EnableLighting;\n
///
/// For a real world usage example, see ezCVarFlags.
template <typename T>
struct ezBitflags
{
private:
  using Enum = typename T::Enum;
  using Bits = typename T::Bits;
  using StorageType = typename T::StorageType;

public:
  using ConstIterator = ezBitIterator<Enum, false>;

  /// \brief Constructor. Initializes the flags to the default value.
  EZ_ALWAYS_INLINE ezBitflags()
    : m_Value((StorageType)T::Default) // [tested]
  {
  }

  /// \brief Converts the incoming type to ezBitflags<T>
  EZ_ALWAYS_INLINE ezBitflags(Enum flag1) // [tested]
  {
    m_Value = (StorageType)flag1;
  }

  EZ_ALWAYS_INLINE void operator=(Enum flag1) { m_Value = (StorageType)flag1; }

  /// \brief Comparison operator.
  EZ_ALWAYS_INLINE bool operator==(const StorageType rhs) const // [tested]
  {
    return m_Value == rhs;
  }

  /// \brief Comparison operator.
  EZ_ALWAYS_INLINE bool operator!=(const StorageType rhs) const { return m_Value != rhs; }

  /// \brief Comparison operator.
  EZ_ALWAYS_INLINE bool operator==(const ezBitflags<T>& rhs) const { return m_Value == rhs.m_Value; }

  /// \brief Comparison operator.
  EZ_ALWAYS_INLINE bool operator!=(const ezBitflags<T>& rhs) const { return m_Value != rhs.m_Value; }

  /// \brief Clears all flags
  EZ_ALWAYS_INLINE void Clear() // [tested]
  {
    m_Value = 0;
  }

  /// \brief Checks if certain flags are set within the bitfield.
  EZ_ALWAYS_INLINE bool IsSet(Enum flag) const // [tested]
  {
    return (m_Value & flag) != 0;
  }

  /// \brief Returns whether all the given flags are set.
  EZ_ALWAYS_INLINE bool AreAllSet(const ezBitflags<T>& rhs) const // [tested]
  {
    return (m_Value & rhs.m_Value) == rhs.m_Value;
  }

  /// \brief Returns whether none of the given flags is set.
  EZ_ALWAYS_INLINE bool AreNoneSet(const ezBitflags<T>& rhs) const // [tested]
  {
    return (m_Value & rhs.m_Value) == 0;
  }

  /// \brief  Returns whether any of the given flags is set.
  EZ_ALWAYS_INLINE bool IsAnySet(const ezBitflags<T>& rhs) const // [tested]
  {
    return (m_Value & rhs.m_Value) != 0;
  }

  /// \brief Sets the given flag.
  EZ_ALWAYS_INLINE void Add(const ezBitflags<T>& rhs) // [tested]
  {
    m_Value |= rhs.m_Value;
  }

  /// \brief Removes the given flag.
  EZ_ALWAYS_INLINE void Remove(const ezBitflags<T>& rhs) // [tested]
  {
    m_Value &= (~rhs.m_Value);
  }

  /// \brief Toggles the state of the given flag.
  EZ_ALWAYS_INLINE void Toggle(const ezBitflags<T>& rhs) // [tested]
  {
    m_Value ^= rhs.m_Value;
  }

  /// \brief Sets or clears the given flag.
  EZ_ALWAYS_INLINE void AddOrRemove(const ezBitflags<T>& rhs, bool bState) // [tested]
  {
    m_Value = (bState) ? m_Value | rhs.m_Value : m_Value & (~rhs.m_Value);
  }

  /// \brief Returns an object that has the flags of \a this and \a rhs combined.
  EZ_ALWAYS_INLINE ezBitflags<T> operator|(const ezBitflags<T>& rhs) const // [tested]
  {
    return ezBitflags<T>(m_Value | rhs.m_Value);
  }

  /// \brief Returns an object that has the flags that were set both in \a this and \a rhs.
  EZ_ALWAYS_INLINE ezBitflags<T> operator&(const ezBitflags<T>& rhs) const // [tested]
  {
    return ezBitflags<T>(m_Value & rhs.m_Value);
  }

  /// \brief Modifies \a this to also contain the bits from \a rhs.
  EZ_ALWAYS_INLINE void operator|=(const ezBitflags<T>& rhs) // [tested]
  {
    m_Value |= rhs.m_Value;
  }

  /// \brief Modifies \a this to only contain the bits that were set in \a this and \a rhs.
  EZ_ALWAYS_INLINE void operator&=(const ezBitflags<T>& rhs) // [tested]
  {
    m_Value &= rhs.m_Value;
  }

  /// \brief Returns the stored value as the underlying integer type.
  EZ_ALWAYS_INLINE StorageType GetValue() const // [tested]
  {
    return m_Value;
  }

  /// \brief Overwrites the flags with a new value.
  EZ_ALWAYS_INLINE void SetValue(StorageType value) // [tested]
  {
    m_Value = value;
  }

  /// \brief Returns true if not a single bit is set.
  EZ_ALWAYS_INLINE bool IsNoFlagSet() const // [tested]
  {
    return m_Value == 0;
  }

  /// \brief Returns true if any bitflag is set.
  EZ_ALWAYS_INLINE bool IsAnyFlagSet() const // [tested]
  {
    return m_Value != 0;
  }

  /// \brief Returns a constant iterator to the very first set bit.
  /// Note that due to the way iterating through bits is accelerated, changes to the bitflags will not affect the iterator after creation.
  EZ_ALWAYS_INLINE ConstIterator GetIterator() const // [tested]
  {
    return ConstIterator((Enum)m_Value);
  }

  /// \brief Returns an invalid iterator. Needed to support range based for loops.
  EZ_ALWAYS_INLINE ConstIterator GetEndIterator() const // [tested]
  {
    return ConstIterator();
  }

private:
  EZ_ALWAYS_INLINE explicit ezBitflags(StorageType flags)
    : m_Value(flags)
  {
  }

  union
  {
    StorageType m_Value;
    Bits m_bits;
  };
};

//////////////////////////////////////////////////////////////////////////
// begin() /end() for range-based for-loop support
template <typename T>
typename ezBitflags<T>::ConstIterator begin(const ezBitflags<T>& container)
{
  return container.GetIterator();
}

template <typename T>
typename ezBitflags<T>::ConstIterator cbegin(const ezBitflags<T>& container)
{
  return container.GetIterator();
}

template <typename T>
typename ezBitflags<T>::ConstIterator end(const ezBitflags<T>& container)
{
  return container.GetEndIterator();
}

template <typename T>
typename ezBitflags<T>::ConstIterator cend(const ezBitflags<T>& container)
{
  return container.GetEndIterator();
}

/// \brief This macro will define the operator| and operator& function that is required for class \a FlagsType to work with ezBitflags.
/// See class ezBitflags for more information.
#define EZ_DECLARE_FLAGS_OPERATORS(FlagsType)                                      \
  inline ezBitflags<FlagsType> operator|(FlagsType::Enum lhs, FlagsType::Enum rhs) \
  {                                                                                \
    return (ezBitflags<FlagsType>(lhs) | ezBitflags<FlagsType>(rhs));              \
  }                                                                                \
                                                                                   \
  inline ezBitflags<FlagsType> operator&(FlagsType::Enum lhs, FlagsType::Enum rhs) \
  {                                                                                \
    return (ezBitflags<FlagsType>(lhs) & ezBitflags<FlagsType>(rhs));              \
  }



/// \brief This macro allows to conveniently declare a bitflag type that can be used with the ezBitflags class.
///
/// Usage: EZ_DECLARE_FLAGS(ezUInt32, FlagsTypeName, Flag1Name, Flag2Name, Flag3Name, Flag4Name, ...)
///
/// This macro will define a simple type of with the name that is given as the second parameter,
/// which can be used as type-safe bitflags. Everything that is necessary to work with the ezBitflags
/// class, will be set up automatically.
/// The bitflag type will use the integer type that is given as the first parameter for its internal
/// storage. So if you pass ezUInt32 as the first parameter, your bitflag type will take up 4 bytes
/// and will support up to 32 flags. You can also pass any other integer type to adjust the required
/// storage space, if you don't need that many flags.
///
/// The third parameter and onwards declare the names of the flags that the type should contain.
/// Each flag will use a different bit. If you need to define flags that are combinations of several
/// other flags, you need to declare the bitflag struct manually. See the ezBitflags class for more
/// information on how to do that.
#define EZ_DECLARE_FLAGS_WITH_DEFAULT(InternalStorageType, BitflagsTypeName, DefaultValue, ...) \
  struct BitflagsTypeName                                                                       \
  {                                                                                             \
    static constexpr ezUInt32 Count = EZ_VA_NUM_ARGS(__VA_ARGS__);                              \
    using StorageType = InternalStorageType;                                                    \
    enum Enum                                                                                   \
    {                                                                                           \
      EZ_EXPAND_ARGS_WITH_INDEX(EZ_DECLARE_FLAGS_ENUM, ##__VA_ARGS__) Default = DefaultValue    \
    };                                                                                          \
    struct Bits                                                                                 \
    {                                                                                           \
      EZ_EXPAND_ARGS(EZ_DECLARE_FLAGS_BITS, ##__VA_ARGS__)                                      \
    };                                                                                          \
    EZ_ENUM_TO_STRING(__VA_ARGS__)                                                              \
  };                                                                                            \
  EZ_DECLARE_FLAGS_OPERATORS(BitflagsTypeName)

#define EZ_DECLARE_FLAGS(InternalStorageType, BitflagsTypeName, ...) \
  EZ_DECLARE_FLAGS_WITH_DEFAULT(InternalStorageType, BitflagsTypeName, 0, ##__VA_ARGS__)
/// \cond

/// Internal Do not use.
#define EZ_DECLARE_FLAGS_ENUM(name, n) name = EZ_BIT(n),

/// Internal Do not use.
#define EZ_DECLARE_FLAGS_BITS(name) StorageType name : 1;

/// \endcond
