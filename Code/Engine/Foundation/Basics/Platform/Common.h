#pragma once

// On MSVC 2008 in 64 Bit <cmath> generates a lot of warnings (actually it is math.h, which is included by cmath)
#define EZ_MSVC_WARNING_NUMBER 4985
#include <Foundation/Basics/Compiler/DisableWarning.h>

// include std header
#include <new>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <cwctype>
#include <cmath>

#include <Foundation/Basics/Compiler/RestoreWarning.h>

// include c++11 specific header if the compiler supports it
#ifdef EZ_CPP11
  #include <type_traits>
#endif

/// Concatenates two strings, even when the strings are macros themselves
#define EZ_CONCAT(x,y) EZ_CONCAT_HELPER(x,y)
#define EZ_CONCAT_HELPER(x,y) EZ_CONCAT_HELPER2(x,y)
#define EZ_CONCAT_HELPER2(x,y) x##y

/// Stringizes a string, even macros
#define EZ_STRINGIZE(str) EZ_STRINGIZE_HELPER(str)
#define EZ_STRINGIZE_HELPER(x) #x

// Macros to do compile-time checks, such as to ensure sizes of types
// EZ_CHECK_AT_COMPILETIME(exp) : only checks exp
// EZ_CHECK_AT_COMPILETIME_MSG(exp, msg) : checks exp and displays msg
#ifdef EZ_CPP11
  #define EZ_CHECK_AT_COMPILETIME(exp) \
  static_assert(exp, EZ_STRINGIZE(exp)##" is false.");
  
  #define EZ_CHECK_AT_COMPILETIME_MSG(exp, msg) \
  static_assert(exp, EZ_STRINGIZE(exp)##" is false. Message: "##msg);
#else
  // Declares an array with an invalid size, if the expression is invalid -> generates a compile-time error
  #define EZ_CHECK_AT_COMPILETIME(exp) \
    typedef int EZ_CompileTimeAssert[(exp) ? 1 : -1]

  #define EZ_CHECK_AT_COMPILETIME_MSG(exp, msg) \
    typedef int EZ_CompileTimeAssert[(exp) ? 1 : -1]
#endif

/// Disallow the copy constructor and the assignment operator for this type. 
#define EZ_DISALLOW_COPY_AND_ASSIGN(type) \
  private: \
    type(const type&); \
    void operator=(const type&)

/// Macro helper to check alignment
#if EZ_COMPILE_FOR_DEVELOPMENT
  #define EZ_CHECK_ALIGNMENT(ptr, alignment) \
    EZ_ASSERT(((size_t)ptr & (alignment - 1)) == 0, "Wrong aligment. Expected %d bytes alignment", alignment)
#else
  #define EZ_CHECK_ALIGNMENT(ptr, alignment)
#endif

#define EZ_ALIGN_16(decl) EZ_ALIGN(decl, 16)
#define EZ_ALIGN_32(decl) EZ_ALIGN(decl, 32)
#define EZ_ALIGN_64(decl) EZ_ALIGN(decl, 64)
#define EZ_ALIGN_128(decl) EZ_ALIGN(decl, 128)
#define EZ_CHECK_ALIGNMENT_16(ptr) EZ_CHECK_ALIGNMENT(ptr, 16)
#define EZ_CHECK_ALIGNMENT_32(ptr) EZ_CHECK_ALIGNMENT(ptr, 32)
#define EZ_CHECK_ALIGNMENT_64(ptr) EZ_CHECK_ALIGNMENT(ptr, 64)
#define EZ_CHECK_ALIGNMENT_128(ptr) EZ_CHECK_ALIGNMENT(ptr, 128)
