#pragma once

/// \file

#include <Foundation/Basics.h>

/// Type traits
template <int v>
struct ezTraitInt
{
  enum { value = v };
};

typedef ezTraitInt<2> ezTypeIsMemRelocatable;
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

/// \brief Default % operator for T and ezTypeIsMemRelocatable which returns a CompileTimeFalseType.
template <typename T>
ezCompileTimeFalseType operator%(const T&, const ezTypeIsMemRelocatable&);

/// \brief If there is an % operator which takes a ezTypeIsMemRelocatable and returns a CompileTimeTrueType T is Pod. Default % operator return false.
template <typename T>
struct ezGetTypeClass : public ezTraitInt<
    (sizeof(*((T*)0) % *((const ezTypeIsMemRelocatable*)0)) == sizeof(ezCompileTimeTrueType)) ? 2 : 
    ezIsPodType<T>::value> { };

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
struct ezGetStrongestTypeClass : public ezTraitInt <
    (T1::value == 0 || T2::value == 0)
    ? 0
    : EZ_COMPILE_TIME_MAX(T1::value, T2::value) > { };


/// \brief Determines whether a type is a pointer.
template<typename T>
struct ezIsPointer 
{
  static const bool value = false;
};

template<typename T>
struct ezIsPointer<T*>
{
  static const bool value = true; 
};


#ifdef __INTELLISENSE__

  /// \brief Embed this into a class to mark it as a POD type.
  /// POD types will get special treatment from allocators and container classes, such that they are faster to construct and copy.
  #define EZ_DECLARE_POD_TYPE()

  /// \brief Embed this into a class to mark it as memory relocatable.
  /// Memory relocatable types will get special treatment from allocators and container classes, such that they are faster to construct and copy.
  /// A type is memory relocatable if it does not have any internal references. e.g: struct example { char[16] buffer; char* pCur; example() pCur(buffer) {} };
  /// A memory relocatable type also must not give out any pointers to its own location. If these two conditions are met, a type is memory relocatable.
  #define EZ_DECLARE_MEM_RELOCATABLE_TYPE()

  // \brief embed this into a class to automatically detect which type class it belongs to
  // This macro is only guaranteed to work for classes / structs which don't have any constructor / destructor / assignment operator!
  // As arguments you have to list the types of all the members of the class / struct.
  #define EZ_DETECT_TYPE_CLASS(...)

#else

  /// \brief Embed this into a class to mark it as a POD type.
  /// POD types will get special treatment from allocators and container classes, such that they are faster to construct and copy.
  #define EZ_DECLARE_POD_TYPE() \
    ezCompileTimeTrueType operator%(const ezTypeIsPod&) const

  /// \brief Embed this into a class to mark it as memory relocatable.
  /// Memory relocatable types will get special treatment from allocators and container classes, such that they are faster to construct and copy.
  /// A type is memory relocatable if it does not have any internal references. e.g: struct example { char[16] buffer; char* pCur; example() pCur(buffer) {} };
  /// A memory relocatable type also must not give out any pointers to its own location. If these two conditions are met, a type is memory relocatable.
  #define EZ_DECLARE_MEM_RELOCATABLE_TYPE() \
    ezCompileTimeTrueType operator%(const ezTypeIsMemRelocatable&) const

  #define EZ_DETECT_TYPE_CLASS_1(T1) ezGetTypeClass<T1>
  #define EZ_DETECT_TYPE_CLASS_2(T1, T2) ezGetStrongestTypeClass<EZ_DETECT_TYPE_CLASS_1(T1), EZ_DETECT_TYPE_CLASS_1(T2)>
  #define EZ_DETECT_TYPE_CLASS_3(T1, T2, T3) ezGetStrongestTypeClass<EZ_DETECT_TYPE_CLASS_2(T1, T2), EZ_DETECT_TYPE_CLASS_1(T3)>
  #define EZ_DETECT_TYPE_CLASS_4(T1, T2, T3, T4) ezGetStrongestTypeClass<EZ_DETECT_TYPE_CLASS_2(T1, T2), EZ_DETECT_TYPE_CLASS_2(T3, T4)>
  #define EZ_DETECT_TYPE_CLASS_5(T1, T2, T3, T4, T5) ezGetStrongestTypeClass<EZ_DETECT_TYPE_CLASS_4(T1, T2, T3, T4), EZ_DETECT_TYPE_CLASS_1(T5)>
  #define EZ_DETECT_TYPE_CLASS_6(T1, T2, T3, T4, T5, T6) ezGetStrongestTypeClass<EZ_DETECT_TYPE_CLASS_4(T1,T2,T3,T4), EZ_DETECT_TYPE_CLASS_2(T5, T6)>
  
  // \brief embed this into a class to automatically detect which type class it belongs to
  // This macro is only guaranteed to work for classes / structs which don't have any constructor / destructor / assignment operator!
  // As arguments you have to list the types of all the members of the class / struct.
  #define EZ_DETECT_TYPE_CLASS(...) \
     ezCompileTimeTrueType operator%(const ezTraitInt<EZ_CALL_MACRO(EZ_CONCAT(EZ_DETECT_TYPE_CLASS_, EZ_VA_NUM_ARGS(__VA_ARGS__)), (__VA_ARGS__))::value>&) const
#endif

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
  template <typename U>
  struct RemoveConst
  {
    typedef U type;
  };

  template <typename U>
  struct RemoveConst<const U>
  {
    typedef U type;
  };

  template <typename U>
  struct RemoveReference
  {
    typedef U type;
  };

  template <typename U>
  struct RemoveReference<U&>
  {
    typedef U type;
  };

  template <typename U>
  struct RemovePointer
  {
    typedef U type;
  };

  template <typename U>
  struct RemovePointer<U*>
  {
    typedef U type;
  };

public:
  /// \brief removes const qualifier
  typedef typename RemoveConst<T>::type NonConstType;

  /// \brief removes reference
  typedef typename RemoveReference<T>::type NonReferenceType;

  /// \brief removes pointer
  typedef typename RemovePointer<T>::type NonPointerType;

  /// \brief removes reference and const qualifier
  typedef typename RemoveConst<typename RemoveReference<T>::type>::type NonConstReferenceType;

  /// \brief removes reference, const and pointer qualifier
  /// Note that this removes the const and reference of the type pointed too, not of the pointer.
  typedef typename RemoveConst<typename RemoveReference<typename RemovePointer<T>::type>::type>::type NonConstReferencePointerType;

};

/// generates a template named 'checkerName' which checks for the existence of a member function with 
/// the name 'functionName' and the signature 'Signature'
#define EZ_MAKE_MEMBERFUNCTION_CHECKER(functionName, checkerName)                                        \
  template<typename T, typename Signature>                                                               \
  struct checkerName {                                                                                   \
    template <typename U, U> struct type_check;                                                          \
    template <typename O> static ezCompileTimeTrueType &chk(type_check<Signature, &O::functionName > *); \
    template <typename   > static ezCompileTimeFalseType  &chk(...);                                     \
    enum { value = (sizeof(chk<T>(0)) == sizeof(ezCompileTimeTrueType)) ? 1 : 0  };                      \
  }

