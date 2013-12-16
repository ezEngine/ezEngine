#pragma once

/// \file

#include "StaticRTTI.h"

/// \brief All classes that should be dynamically reflectable, need to be derived from this base class.
///
/// The only functionality that this class provides is the virtual GetDynamicRTTI() function, which then needs to be
/// overridden in each derived class. Use the macro EZ_ADD_DYNAMIC_REFLECTION for this.
class ezReflectedClass
{
public:
  virtual const ezRTTI* GetDynamicRTTI()
  {
    return ezGetStaticRTTI<ezReflectedClass>();
  }
};

/// \brief This needs to be put into the class declaration of EVERY dynamically reflectable class.
///
/// This macro extends a class, such that it is now able to return its own type information via GetDynamicRTTI(),
/// which is a virtual function, that is reimplemented on each type. A class needs to be derived from ezReflectedClass
/// (at least indirectly) for this.
#define EZ_ADD_DYNAMIC_REFLECTION(SELF)                               \
  EZ_ALLOW_PRIVATE_PROPERTIES(SELF);                                  \
  public:                                                             \
    virtual const ezRTTI* GetDynamicRTTI() EZ_OVERRIDE                \
    {                                                                 \
      return ezGetStaticRTTI<SELF>();                                 \
    }                                                                 \
  private:                                                            \

