#pragma once

/// \file

#include <Foundation/Basics.h>

/// Type traits
template <int v>
struct ezTraitInt
{
  enum { value = v };
};

typedef ezTraitInt<1> ezTypeIsPod;
typedef ezTraitInt<0> ezTypeIsClass;

typedef char ezCompileTimeTrueType;
typedef int ezCompileTimeFalseType;

/// \brief Default % operator for T and TypeIsPod which returns a CompileTimeFalseType.
template <typename T>
ezCompileTimeFalseType operator%(const T&, const ezTypeIsPod&);

/// \brief If there is an % operator which takes a TypeIsPod and returns a CompileTimeTrueType T is Pod. Default % operator return false.
template <typename T>
struct ezIsPodType : public ezTraitInt<(sizeof(*((T*)0) % *((const ezTypeIsPod*)0)) == 
  sizeof(ezCompileTimeTrueType)) ? 1 : 0> { };

/// \brief Pointers are POD types.
template <typename T> 
struct ezIsPodType<T*> : public ezTypeIsPod { };

/// \brief arrays are POD types
template <typename T, int N>
struct ezIsPodType<T[N]> : public ezTypeIsPod { };

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


/// \brief Embed this into a class to mark it as a POD type.
/// POD types will get special treatment from allocators and container classes, such that they are faster to construct and copy.
#define EZ_DECLARE_POD_TYPE() \
  ezCompileTimeTrueType operator%(const ezTypeIsPod&) const

/// \brief Defines a type T as Pod.
/// POD types will get special treatment from allocators and container classes, such that they are faster to construct and copy.
#define EZ_DEFINE_AS_POD_TYPE(T) \
  template<> struct ezIsPodType<T> : public ezTypeIsPod { };

EZ_DEFINE_AS_POD_TYPE(bool);
EZ_DEFINE_AS_POD_TYPE(float);
EZ_DEFINE_AS_POD_TYPE(double);

EZ_DEFINE_AS_POD_TYPE(ezInt8);
EZ_DEFINE_AS_POD_TYPE(ezInt16);
EZ_DEFINE_AS_POD_TYPE(ezInt32);
EZ_DEFINE_AS_POD_TYPE(ezInt64);
EZ_DEFINE_AS_POD_TYPE(ezUInt8);
EZ_DEFINE_AS_POD_TYPE(ezUInt16);
EZ_DEFINE_AS_POD_TYPE(ezUInt32);
EZ_DEFINE_AS_POD_TYPE(ezUInt64);
EZ_DEFINE_AS_POD_TYPE(wchar_t);

/// \brief Checks inheritance at compile time.
#define EZ_IS_DERIVED_FROM_STATIC(BaseClass, DerivedClass) \
  (ezConversionTest<const DerivedClass*, const BaseClass*>::exists && \
  !ezConversionTest<const BaseClass*, const void*>::sameType)

/// \brief Checks whether A and B are the same type
#define EZ_IS_SAME_TYPE(TypeA, TypeB) \
  ezConversionTest<TypeA, TypeB>::sameType

template <typename T>
struct ezTypeTraits
{
private:
  template<typename U>
  struct RemoveConst
  {
    typedef U type;
  };

  template<typename U>
  struct RemoveConst<const U>
  {
    typedef U type;
  };

public:
  /// \brief removes const qualifier
  typedef typename RemoveConst<T>::type NonConstType;
};

