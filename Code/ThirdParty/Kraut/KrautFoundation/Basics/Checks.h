#ifndef AE_FOUNDATION_BASICS_CHECKS_H
#define AE_FOUNDATION_BASICS_CHECKS_H

#include "../Defines.h"

#include <cassert>

// Set Warnings as Errors: Too few/many parameters given for Macro
#pragma warning(error : 4002 4003)

#define AE_CHECK_ALWAYS(bCondition, szErrorMsg, ...) \
  {                                                  \
    bool cond = bCondition;                          \
    assert(cond);                                    \
  }

#define AE_CHECK_DEV(bCondition, szErrorMsg, ...) \
  {                                               \
    bool cond = bCondition;                       \
    assert(cond);                                 \
  }


#endif
