#pragma once

#include "RTTI.h"
#include "Helpers.h"

// ****************************************************
// ***** Templates for accessing static RTTI data *****

/// \brief [internal] Helper struct for accessing static RTTI data.
template<typename T>
struct ezStaticRTTI
{
  static ezRTTI* GetRTTI();
};

/// \brief Use this function, specialized with the type that you are interested in, to get the static RTTI data for some type.
template<typename T>
const ezRTTI* ezGetStaticRTTI()
{
  // Since this is pure C++ and no preprocessor macro, calling it with types such as 'int' and 'ezInt32' will 
  // actually return the same RTTI object, which would not be possible with a purely macro based solution

  return ezStaticRTTI<T>::GetRTTI();
}

// **************************************************
// ***** Macros for declaring types reflectable *****

#define EZ_NO_LINKAGE

/// \brief Declares a type to be reflectable. Insert this into the header of a type to enable reflection on it.
#define EZ_DECLARE_REFLECTABLE_TYPE(Linkage, TYPE)                    \
  /* The function that stores the RTTI object.*/                      \
  Linkage ezRTTI* ezReflectableTypeRTTI_##TYPE();                     \
                                                                      \
  /* This specialization calls the function to get the RTTI data */   \
  /* This code might get duplicated in different DLLs, but all   */   \
  /* will call the same function, so the RTTI object is unique   */   \
  template<>                                                          \
  struct ezStaticRTTI<TYPE>                                           \
  {                                                                   \
    static ezRTTI* GetRTTI()                                          \
    {                                                                 \
      static ezRTTI* s_pRTTI = ezReflectableTypeRTTI_##TYPE();        \
      return s_pRTTI;                                                 \
    }                                                                 \
  };                                                                  \


/// \brief Implements the necessary functionality for a type to be generally reflectable.
#define EZ_IMPLEMENT_REFLECTABLE_TYPE(Type)                           \
  ezRTTI* ezReflectableTypeRTTI_##Type()                              \
  {                                                                   \
    static ezRTTI rtti(#Type, NULL);                                  \
    return &rtti;                                                     \
  }                                                                   \
  static ezExecuteAtStartup s_AutoRegister_##Type                     \
    ([] { ezReflectableTypeRTTI_##Type(); });                         \

/// \brief Implements the necessary functionality for a type to be generally reflectable.
#define EZ_IMPLEMENT_REFLECTABLE_TYPE_WITH_BASE(Type, Base)           \
  ezRTTI* ezReflectableTypeRTTI_##Type()                              \
  {                                                                   \
    static ezRTTI rtti(#Type, ezGetStaticRTTI<Base>());               \
    return &rtti;                                                     \
  }                                                                   \
  static ezExecuteAtStartup s_AutoRegister_##Type                     \
    ([] { ezReflectableTypeRTTI_##Type(); });                         \



