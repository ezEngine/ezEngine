#pragma once

#include <Foundation/Basics.h>

/// ***** Usage Guidelines *****
///
/// For your typical code, use EZ_ASSERT to check that vital preconditions are met.
/// Be aware that EZ_ASSERT is removed in release builds, INCLUDING your code in the assert condition.
/// If the code that you are checking must be executed, even in release builds, use EZ_VERIFY instead.
/// EZ_ASSERT and EZ_VERIFY will trigger a breakpoint in debug builds, but will not interrupt the application 
/// in release builds.
///
/// If you need to check something that is so vital that the application can only fail (ie. crash), if that condition 
/// is not met, even in release builds, then use EZ_ASSERT_ALWAYS. Its condition will always be evaluated and it will
/// always trigger a breakpoint (debug) or crash the application (release).
///
/// To ensure that users are using your code correctly (e.g. they don't call certain functions before initializing the 
/// sub-system) you can use EZ_ASSERT_API. This is always active (even in release builds) to ensure that such obvious 
/// usage errors are detected. EZ_ASSERT_API should not be used in frequently executed code, as it is not stripped from 
/// release builds by default.
///
/// If you need to squeeze the last bit of performance out of your code, EZ_ASSERT_API can be disabled, by defining 
/// EZ_DISABLE_API_ASSERTS.
/// Please be aware that EZ_ASSERT_API works like the other asserts, ie. once it is deactivated, the code in the condition 
/// is not executed anymore.
///



/// \brief Called by the assert macros whenever a check failed. Returns true if the user wants to trigger a break point
EZ_FOUNDATION_DLL bool ezFailedCheck(const char* szSourceFile, ezUInt32 uiLine, const char* szFunction, const char* szExpression, const char* szErrorMsg, ...);

/// \brief Macro to report a failure when that code is reached. This will ALWAYS be executed, even in release builds, therefore might crash the application (or trigger a debug break).
#define EZ_REPORT_FAILURE(szErrorMsg, ...) \
  { if (ezFailedCheck(EZ_SOURCE_FILE, EZ_SOURCE_LINE, EZ_SOURCE_FUNCTION, "", szErrorMsg, ##__VA_ARGS__)) EZ_DEBUG_BREAK; }

/// \brief Macro to raise an error, if a condition is not met. Allows to write a message using printf style. This assert will be triggered, even in non-development builds and cannot be deactivated.
#define EZ_ASSERT_ALWAYS(bCondition, szErrorMsg, ...) \
  EZ_ANALYSIS_ASSUME(bCondition); \
  if ((bCondition) == false) \
  { if (ezFailedCheck(EZ_SOURCE_FILE, EZ_SOURCE_LINE, EZ_SOURCE_FUNCTION, #bCondition, szErrorMsg, ##__VA_ARGS__)) EZ_DEBUG_BREAK; }
  

/// \brief This type of assert can be used to mark code as 'not (yet) implemented' and makes it easier to find it later on by just searching for these asserts.
#define EZ_ASSERT_NOT_IMPLEMENTED EZ_ASSERT_ALWAYS

// Occurrences of EZ_ASSERT are compiled out in non-development builds
#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)

  /// \brief Macro to raise an error, if a condition is not met.
  ///
  /// Allows to write a message using printf style.
  /// Compiled out in non-development builds.
  /// The condition is not evaluated, when this is compiled out, so do not execute important code in it.
  #define EZ_ASSERT EZ_ASSERT_ALWAYS

  /// \brief Macro to raise an error, if a condition is not met.
  ///
  /// Allows to write a message using printf style.
  /// Compiled out in non-development builds, however the condition is always evaluated,
  /// so you may execute important code in it.
  #define EZ_VERIFY EZ_ASSERT_ALWAYS

#else

  /// \brief Macro to raise an error, if a condition is not met.
  ///
  /// Allows to write a message using printf style.
  /// Compiled out in non-development builds.
  /// The condition is not evaluated, when this is compiled out, so do not execute important code in it.
  #define EZ_ASSERT(bCondition, szErrorMsg, ...) EZ_ANALYSIS_ASSUME(bCondition);

  /// \brief Macro to raise an error, if a condition is not met.
  ///
  /// Allows to write a message using printf style.
  /// Compiled out in non-development builds, however the condition is always evaluated,
  /// so you may execute important code in it.
  #define EZ_VERIFY(bCondition, szErrorMsg, ...) \
    if ((bCondition) == false) \
    { /* The condition is evaluated, even though nothing is done with it. */ }

#endif

#if EZ_DISABLE_API_ASSERTS

  /// \brief An assert to check the correct usage of API functions.
  ///
  /// Should only be used inside engine code. 
  /// These asserts can be disabled (and then their condition will not be evaluated), 
  /// but this needs to be specifically done by the user by defining EZ_DISABLE_API_ASSERTS.
  /// That should only be done, if you are intending to ship a product, and want get rid of all unnecessary overhead.
  #define EZ_ASSERT_API(bCondition, szErrorMsg, ...) EZ_ANALYSIS_ASSUME(bCondition)

#else

  /// \brief An assert to check the correct usage of API functions.
  ///
  /// Should only be used inside engine code. 
  /// These asserts can be disabled (and then their condition will not be evaluated), 
  /// but this needs to be specifically done by the user by defining EZ_DISABLE_API_ASSERTS.
  /// That should only be done, if you are intending to ship a product, and want get rid of all unnecessary overhead.
  #define EZ_ASSERT_API EZ_ASSERT_ALWAYS

#endif
