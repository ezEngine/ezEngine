#pragma once

#include <Foundation/Basics.h>
#include "Type.h"

// The 'type graph' can be built 'by hand', but it is easier to use macros

class ezTypeRTTI;

template<class T>
struct ezGetStaticRTTI
{
  static ezTypeRTTI* Type() { return NULL; }
};

template<class T>
ezTypeRTTI* GetStaticRTTI()
{
  return ezGetStaticRTTI<T>::Type();
}

#define EZ_DECLARE_STATIC_REFLECTION(CLASS) \
  template<>\
  struct ezGetStaticRTTI<CLASS> { static ezTypeRTTI* Type() { static ezTypeRTTI s_Type(#CLASS, NULL); return &s_Type; } };

#define EZ_DECLARE_STATIC_REFLECTION_WITH_BASE(CLASS, BASE) \
  template<>\
  struct ezGetStaticRTTI<CLASS> { static ezTypeRTTI* Type() { static ezTypeRTTI s_Type(#CLASS, ezGetStaticRTTI<BASE>::Type()); return &s_Type; } };

#define EZ_ADD_DYNAMIC_REFLECTION(CLASS)                                                        \
  template<class T>                                                                             \
  friend struct Blaaaa;                                                                         \
  virtual const ezTypeRTTI* GetDynamicRTTI() EZ_OVERRIDE   { return GetStaticRTTI<CLASS>(); }   \


class ezReflectedBase;
EZ_DECLARE_STATIC_REFLECTION(ezReflectedBase);

class ezReflectedBase
{
public:
  virtual const ezTypeRTTI* GetDynamicRTTI() { return NULL; }
};
