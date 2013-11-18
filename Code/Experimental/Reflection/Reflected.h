#pragma once

#include <Foundation/Basics.h>
#include "Type.h"

class ezReflectedBase
{
public:
  static  const ezTypeRTTI* GetStaticRTTI()  { return NULL; }
  virtual const ezTypeRTTI* GetDynamicRTTI() { return NULL; }

};

// The 'type graph' can be built 'by hand', but it is easier to use macros

// If we were to move the s_Type variable and the GetStaticRTTI function out of the class / struct
// we could actually reflect structs that are not part of ez, but we would need to generate lots of
// ugly concatenated function names and variables to prevent name collisions
// also instead of SomeStruct::GetStaticRTTI we would need to call something like
// ezStaticRTTI(SomeStruct) (which is a macro that will then do the name concatenation for you)


#define EZ_DECLARE_REFLECTED_CLASS(CLASS, BASECLASS)                            \
  public:                                                                       \
    typedef BASECLASS SUPER;                                                    \
    static  const ezTypeRTTI* GetStaticRTTI()              { return &s_Type; }  \
    virtual const ezTypeRTTI* GetDynamicRTTI() EZ_OVERRIDE { return &s_Type; }  \
                                                                                \
  private:                                                                      \
    static ezTypeRTTI s_Type

#define EZ_IMPLEMENT_REFLECTED_CLASS(CLASS)                                     \
  ezTypeRTTI CLASS::s_Type(#CLASS, CLASS::SUPER::GetStaticRTTI(), ezTypeRTTI::Class)


#define EZ_DECLARE_REFLECTED_STRUCT                                             \
  private:                                                                      \
    static ezTypeRTTI s_Type;                                                   \
                                                                                \
  public:                                                                       \
    static  const ezTypeRTTI* GetStaticRTTI()              { return &s_Type; }  \

#define EZ_IMPLEMENT_REFLECTED_STRUCT(STRUCT)                                   \
  ezTypeRTTI STRUCT::s_Type(#STRUCT, NULL, ezTypeRTTI::Struct)