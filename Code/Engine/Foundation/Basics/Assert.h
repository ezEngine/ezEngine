#pragma once

/// \file

#include <Foundation/Basics.h>

/// ***** Assert Usage Guidelines *****
///
/// For your typical code, use EZ_ASSERT_DEV to check that vital preconditions are met.
/// Be aware that EZ_ASSERT_DEV is removed in non-development builds (ie. when EZ_COMPILE_FOR_DEVELOPMENT is disabled), 
/// INCLUDING your code in the assert condition.
/// If the code that you are checking must be executed, even in non-development builds, use EZ_VERIFY instead.
/// EZ_ASSERT_DEV and EZ_VERIFY will trigger a breakpoint in debug builds, but will not interrupt the application 
/// in release builds.
///
/// For conditions that are rarely violated or checking is very costly, use EZ_ASSERT_DEBUG. This assert is only active
/// in debug builds. This allows to have extra checking while debugging a program, but not waste performance when a
/// development release build is used.
///
/// If you need to check something that is so vital that the application can only fail (i.e. crash), if that condition 
/// is not met, even in release builds, then use EZ_ASSERT_RELEASE. This should not be used in frequently executed code, 
/// as it is not stripped from non-development builds by default.
///
/// If you need to squeeze the last bit of performance out of your code, EZ_ASSERT_RELEASE can be disabled, by defining 
/// EZ_DISABLE_RELEASE_ASSERTS.
/// Please be aware that EZ_ASSERT_RELEASE works like the other asserts, i.e. once it is deactivated, the code in the condition 
/// is not executed anymore.
///



/// \brief Assert handler callback. Should return true to trigger a break point or false if the assert should be ignored
typedef bool (*ezAssertHandler)(const char* szSourceFile, ezUInt32 uiLine, const char* szFunction, const char* szExpression, const char* szAssertMsg);

/// \brief Gets the current assert handler. The default assert handler shows a dialog on windows or prints to the console on other platforms.
EZ_FOUNDATION_DLL ezAssertHandler ezGetAssertHandler();

/// \brief Sets the assert handler. It is the responsibility of the user to chain assert handlers if needed.
EZ_FOUNDATION_DLL void ezSetAssertHandler(ezAssertHandler handler);


/// \brief Called by the assert macros whenever a check failed. Returns true if the user wants to trigger a break point
EZ_FOUNDATION_DLL bool ezFailedCheck(const char* szSourceFile, ezUInt32 uiLine, const char* szFunction, const char* szExpression, const char* szErrorMsg, ...);

/// \brief Macro to report a failure when that code is reached. This will ALWAYS be executed, even in release builds, therefore might crash the application (or trigger a debug break).
#define EZ_REPORT_FAILURE(szErrorMsg, ...) \
  { if (ezFailedCheck(EZ_SOURCE_FILE, EZ_SOURCE_LINE, EZ_SOURCE_FUNCTION, "", szErrorMsg, ##__VA_ARGS__)) EZ_DEBUG_BREAK; }

/// \brief Macro to raise an error, if a condition is not met. Allows to write a message using printf style. This assert will be triggered, even in non-development builds and cannot be deactivated.
#define EZ_ASSERT_ALWAYS(bCondition, szErrorMsg, ...) \
  do { \
  EZ_ANALYSIS_ASSUME(bCondition); \
  if ((bCondition) == false) \
  { if (ezFailedCheck(EZ_SOURCE_FILE, EZ_SOURCE_LINE, EZ_SOURCE_FUNCTION, #bCondition, szErrorMsg, ##__VA_ARGS__)) EZ_DEBUG_BREAK; } \
  } while (false)
  

/// \brief This type of assert can be used to mark code as 'not (yet) implemented' and makes it easier to find it later on by just searching for these asserts.
#define EZ_ASSERT_NOT_IMPLEMENTED EZ_REPORT_FAILURE("Not implemented")

// Occurrences of EZ_ASSERT_DEBUG are compiled out in non-debug builds
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  /// \brief Macro to raise an error, if a condition is not met.
  ///
  /// Allows to write a message using printf style.
  /// Compiled out in non-debug builds.
  /// The condition is not evaluated, when this is compiled out, so do not execute important code in it.
  #define EZ_ASSERT_DEBUG EZ_ASSERT_ALWAYS
#else
  /// \brief Macro to raise an error, if a condition is not met.
  ///
  /// Allows to write a message using printf style.
  /// Compiled out in non-debug builds.
  /// The condition is not evaluated, when this is compiled out, so do not execute important code in it.
  #define EZ_ASSERT_DEBUG(bCondition, szErrorMsg, ...) EZ_ANALYSIS_ASSUME(bCondition)
#endif


// Occurrences of EZ_ASSERT_DEV are compiled out in non-development builds
#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT) || EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)

  /// \brief Macro to raise an error, if a condition is not met.
  ///
  /// Allows to write a message using printf style.
  /// Compiled out in non-development builds.
  /// The condition is not evaluated, when this is compiled out, so do not execute important code in it.
  #define EZ_ASSERT_DEV EZ_ASSERT_ALWAYS

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
  #define EZ_ASSERT_DEV(bCondition, szErrorMsg, ...) EZ_ANALYSIS_ASSUME(bCondition)

  /// \brief Macro to raise an error, if a condition is not met.
  ///
  /// Allows to write a message using printf style.
  /// Compiled out in non-development builds, however the condition is always evaluated,
  /// so you may execute important code in it.
  #define EZ_VERIFY(bCondition, szErrorMsg, ...) \
    if ((bCondition) == false) \
    { /* The condition is evaluated, even though nothing is done with it. */ }

#endif

#if EZ_DISABLE_RELEASE_ASSERTS

  /// \brief An assert to check conditions even in release builds.
  ///
  /// These asserts can be disabled (and then their condition will not be evaluated), 
  /// but this needs to be specifically done by the user by defining EZ_DISABLE_RELEASE_ASSERTS.
  /// That should only be done, if you are intending to ship a product, and want get rid of all unnecessary overhead.
  #define EZ_ASSERT_RELEASE(bCondition, szErrorMsg, ...) EZ_ANALYSIS_ASSUME(bCondition)

#else

  /// \brief An assert to check conditions even in release builds.
  ///
  /// These asserts can be disabled (and then their condition will not be evaluated), 
  /// but this needs to be specifically done by the user by defining EZ_DISABLE_RELEASE_ASSERTS.
  /// That should only be done, if you are intending to ship a product, and want get rid of all unnecessary overhead.
  #define EZ_ASSERT_RELEASE EZ_ASSERT_ALWAYS

#endif

