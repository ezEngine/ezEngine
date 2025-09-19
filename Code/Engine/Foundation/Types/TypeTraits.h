#pragma once

#ifndef EZ_INCLUDING_BASICS_H
#  error "Please don't include TypeTraits.h directly, but instead include Foundation/Basics.h"
#endif

/// \file

/// \brief Compile-time type classification system for optimizing container operations.
///
/// The ezEngine type trait system classifies types into three categories to enable
/// different optimization strategies for containers and memory operations:
///
/// - Class (0): Standard types requiring constructor/destructor calls and careful copying
/// - POD (1): Plain Old Data types that can be memcpy'd and don't need destructor calls
/// - MemRelocatable (2): Types that can be moved with memcpy but may need destructor calls
///
/// This classification allows containers to choose the most efficient implementation
/// for construction, destruction, copying, and moving operations.
template <int v>
struct ezTraitInt
{
  static constexpr int value = v;
};

using ezTypeIsMemRelocatable = ezTraitInt<2>; ///< Types that can be moved with memcpy
using ezTypeIsPod = ezTraitInt<1>;            ///< Plain Old Data types
using ezTypeIsClass = ezTraitInt<0>;          ///< Standard class types

using ezCompileTimeTrueType = char;
using ezCompileTimeFalseType = int;

/// \brief Converts a bool condition to CompileTimeTrue/FalseType
template <bool cond>
struct ezConditionToCompileTimeBool
{
  using type = ezCompileTimeFalseType;
};

template <>
struct ezConditionToCompileTimeBool<true>
{
  using type = ezCompileTimeTrueType;
};

/// \brief Default % operator for T and TypeIsPod which returns a CompileTimeFalseType.
template <typename T>
ezCompileTimeFalseType operator%(const T&, const ezTypeIsPod&);

/// \brief If there is an % operator which takes a TypeIsPod and returns a CompileTimeTrueType T is Pod. Default % operator return false.
template <typename T>
struct ezIsPodType : public ezTraitInt<(sizeof(*((T*)0) % *((const ezTypeIsPod*)0)) == sizeof(ezCompileTimeTrueType)) ? 1 : 0>
{
};

/// \brief Pointers are POD types.
template <typename T>
struct ezIsPodType<T*> : public ezTypeIsPod
{
};

/// \brief arrays are POD types
template <typename T, int N>
struct ezIsPodType<T[N]> : public ezTypeIsPod
{
};

/// \brief Default % operator for T and ezTypeIsMemRelocatable which returns a CompileTimeFalseType.
template <typename T>
ezCompileTimeFalseType operator%(const T&, const ezTypeIsMemRelocatable&);

/// \brief If there is an % operator which takes a ezTypeIsMemRelocatable and returns a CompileTimeTrueType T is Pod. Default % operator
/// return false.
template <typename T>
struct ezGetTypeClass
  : public ezTraitInt<(sizeof(*((T*)0) % *((const ezTypeIsMemRelocatable*)0)) == sizeof(ezCompileTimeTrueType)) ? 2 : ezIsPodType<T>::value>
{
};

/// \brief Static Conversion Test
template <typename From, typename To>
struct ezConversionTest
{
  static ezCompileTimeTrueType Test(const To&);
  static ezCompileTimeFalseType Test(...);
  static From MakeFrom();

  enum
  {
    exists = sizeof(Test(MakeFrom())) == sizeof(ezCompileTimeTrueType),
    sameType = 0
  };
};

/// \brief Specialization for above Type.
template <typename T>
struct ezConversionTest<T, T>
{
  enum
  {
    exists = 1,
    sameType = 1
  };
};

// remapping of the 0 (not special) type to 3
template <typename T1, typename T2>
struct ezGetStrongestTypeClass : public ezTraitInt<(T1::value == 0 || T2::value == 0) ? 0 : EZ_COMPILE_TIME_MAX(T1::value, T2::value)>
{
};


#ifdef __INTELLISENSE__

/// \brief Embed this into a class to mark it as a POD type.
/// POD types will get special treatment from allocators and container classes, such that they are faster to construct and copy.
#  define EZ_DECLARE_POD_TYPE()

/// \brief Embed this into a class to mark it as memory relocatable.
/// Memory relocatable types will get special treatment from allocators and container classes, such that they are faster to construct and
/// copy. A type is memory relocatable if it does not have any internal references. e.g: struct example { char[16] buffer; char* pCur;
/// example() pCur(buffer) {} }; A memory relocatable type also must not give out any pointers to its own location. If these two conditions
/// are met, a type is memory relocatable.
#  define EZ_DECLARE_MEM_RELOCATABLE_TYPE()

/// \brief mark a class as memory relocatable if the passed type is relocatable or pod.
#  define EZ_DECLARE_MEM_RELOCATABLE_TYPE_CONDITIONAL(T)

// \brief embed this into a class to automatically detect which type class it belongs to
// This macro is only guaranteed to work for classes / structs which don't have any constructor / destructor / assignment operator!
// As arguments you have to list the types of all the members of the class / struct.
#  define EZ_DETECT_TYPE_CLASS(...)

#else

/// \brief Embed this into a class to mark it as a POD type.
/// POD types will get special treatment from allocators and container classes, such that they are faster to construct and copy.
#  define EZ_DECLARE_POD_TYPE()                               \
    ezCompileTimeTrueType operator%(const ezTypeIsPod&) const \
    {                                                         \
      return {};                                              \
    }

/// \brief Embed this into a class to mark it as memory relocatable.
/// Memory relocatable types will get special treatment from allocators and container classes, such that they are faster to construct and
/// copy. A type is memory relocatable if it does not have any internal references. e.g: struct example { char[16] buffer; char* pCur;
/// example() pCur(buffer) {} }; A memory relocatable type also must not give out any pointers to its own location. If these two conditions
/// are met, a type is memory relocatable.
#  define EZ_DECLARE_MEM_RELOCATABLE_TYPE()                              \
    ezCompileTimeTrueType operator%(const ezTypeIsMemRelocatable&) const \
    {                                                                    \
      return {};                                                         \
    }

/// \brief mark a class as memory relocatable if the passed type is relocatable or pod.
#  define EZ_DECLARE_MEM_RELOCATABLE_TYPE_CONDITIONAL(T)                                                                                       \
    typename ezConditionToCompileTimeBool<ezGetTypeClass<T>::value == ezTypeIsMemRelocatable::value || ezIsPodType<T>::value>::type operator%( \
      const ezTypeIsMemRelocatable&) const                                                                                                     \
    {                                                                                                                                          \
      return {};                                                                                                                               \
    }

#  define EZ_DETECT_TYPE_CLASS_1(T1) ezGetTypeClass<T1>
#  define EZ_DETECT_TYPE_CLASS_2(T1, T2) ezGetStrongestTypeClass<EZ_DETECT_TYPE_CLASS_1(T1), EZ_DETECT_TYPE_CLASS_1(T2)>
#  define EZ_DETECT_TYPE_CLASS_3(T1, T2, T3) ezGetStrongestTypeClass<EZ_DETECT_TYPE_CLASS_2(T1, T2), EZ_DETECT_TYPE_CLASS_1(T3)>
#  define EZ_DETECT_TYPE_CLASS_4(T1, T2, T3, T4) ezGetStrongestTypeClass<EZ_DETECT_TYPE_CLASS_2(T1, T2), EZ_DETECT_TYPE_CLASS_2(T3, T4)>
#  define EZ_DETECT_TYPE_CLASS_5(T1, T2, T3, T4, T5) ezGetStrongestTypeClass<EZ_DETECT_TYPE_CLASS_4(T1, T2, T3, T4), EZ_DETECT_TYPE_CLASS_1(T5)>
#  define EZ_DETECT_TYPE_CLASS_6(T1, T2, T3, T4, T5, T6) \
    ezGetStrongestTypeClass<EZ_DETECT_TYPE_CLASS_4(T1, T2, T3, T4), EZ_DETECT_TYPE_CLASS_2(T5, T6)>

// \brief embed this into a class to automatically detect which type class it belongs to
// This macro is only guaranteed to work for classes / structs which don't have any constructor / destructor / assignment operator!
// As arguments you have to list the types of all the members of the class / struct.
#  define EZ_DETECT_TYPE_CLASS(...)                                                                                                   \
    ezCompileTimeTrueType operator%(                                                                                                  \
      const ezTraitInt<EZ_CALL_MACRO(EZ_PP_CONCAT(EZ_DETECT_TYPE_CLASS_, EZ_VA_NUM_ARGS(__VA_ARGS__)), (__VA_ARGS__))::value>&) const \
    {                                                                                                                                 \
      return {};                                                                                                                      \
    }
#endif

/// \brief Defines a type T as Pod.
/// POD types will get special treatment from allocators and container classes, such that they are faster to construct and copy.
#define EZ_DEFINE_AS_POD_TYPE(T)             \
  template <>                                \
  struct ezIsPodType<T> : public ezTypeIsPod \
  {                                          \
  }

EZ_DEFINE_AS_POD_TYPE(bool);
EZ_DEFINE_AS_POD_TYPE(float);
EZ_DEFINE_AS_POD_TYPE(double);

EZ_DEFINE_AS_POD_TYPE(char);
EZ_DEFINE_AS_POD_TYPE(ezInt8);
EZ_DEFINE_AS_POD_TYPE(ezInt16);
EZ_DEFINE_AS_POD_TYPE(ezInt32);
EZ_DEFINE_AS_POD_TYPE(ezInt64);
EZ_DEFINE_AS_POD_TYPE(ezUInt8);
EZ_DEFINE_AS_POD_TYPE(ezUInt16);
EZ_DEFINE_AS_POD_TYPE(ezUInt32);
EZ_DEFINE_AS_POD_TYPE(ezUInt64);
EZ_DEFINE_AS_POD_TYPE(wchar_t);
EZ_DEFINE_AS_POD_TYPE(unsigned long);
EZ_DEFINE_AS_POD_TYPE(long);
EZ_DEFINE_AS_POD_TYPE(std::byte);

/// \brief Checks inheritance at compile time.
#define EZ_IS_DERIVED_FROM_STATIC(BaseClass, DerivedClass) \
  (ezConversionTest<const DerivedClass*, const BaseClass*>::exists && !ezConversionTest<const BaseClass*, const void*>::sameType)

/// \brief Checks whether A and B are the same type
#define EZ_IS_SAME_TYPE(TypeA, TypeB) ezConversionTest<TypeA, TypeB>::sameType

/// \brief Utility template for extracting clean types from decorated types.
template <typename T>
struct ezTypeTraits
{
  /// \brief Removes const qualifier: const int -> int
  using NonConstType = typename std::remove_const<T>::type;

  /// \brief Removes reference qualifier: int& -> int, int&& -> int
  using NonReferenceType = typename std::remove_reference<T>::type;

  /// \brief Removes pointer qualifier: int* -> int
  using NonPointerType = typename std::remove_pointer<T>::type;

  /// \brief Removes both reference and const qualifiers: const int& -> int
  using NonConstReferenceType = typename std::remove_const<typename std::remove_reference<T>::type>::type;

  /// \brief Removes both reference and pointer qualifiers: int*& -> int
  using NonReferencePointerType = typename std::remove_pointer<typename std::remove_reference<T>::type>::type;

  /// \brief Removes reference, const, and pointer qualifiers from the pointed-to type.
  ///
  /// Note: This operates on the pointed-to type, not the pointer itself.
  /// Example: const int*& -> int (not int*)
  using NonConstReferencePointerType = typename std::remove_const<typename std::remove_reference<typename std::remove_pointer<T>::type>::type>::type;
};

/// generates a template named 'checkerName' which checks for the existence of a member function with
/// the name 'functionName' and the signature 'Signature'
#define EZ_MAKE_MEMBERFUNCTION_CHECKER(functionName, checkerName)                \
  template <typename T, typename Signature>                                      \
  struct checkerName                                                             \
  {                                                                              \
    template <typename U, U>                                                     \
    struct type_check;                                                           \
    template <typename O>                                                        \
    static ezCompileTimeTrueType& chk(type_check<Signature, &O::functionName>*); \
    template <typename>                                                          \
    static ezCompileTimeFalseType& chk(...);                                     \
    enum                                                                         \
    {                                                                            \
      value = (sizeof(chk<T>(0)) == sizeof(ezCompileTimeTrueType)) ? 1 : 0       \
    };                                                                           \
  }
