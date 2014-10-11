#pragma once

#include <CoreUtils/Basics.h>
#include <Foundation/Utilities/EnumerableClass.h>
#include <Foundation/Types/Variant.h>

/// \brief Base class for all types of ezConsoleFunction, represents functions to be exposed to ezConsole.
///
/// Console functions are similar to ezCVar's in that they can be executed from the ezConsole.
/// A console function can wrap many different types of functions with differing number and types of parameters.
/// ezConsoleFunction uses an ezDelegate internally to store the function reference, so even member functions would be possible.
///
/// All console functions are enumerable, as their base class ezConsoleFunctionBase is an ezEnumerable class.
///
/// Console functions can have between zero and six parameters. The LuaInterpreter for ezConsole only supports parameter types
/// (unsigned) int, float/double, bool and string and uses the conversion feature of ezVariant to map the lua input to the final function.
///
/// To make a function available as a console function, create a global variable of type ezConsoleFunction with the proper template
/// arguments to mirror its parameters and return type.
/// Note that although functions with return types are accepted, the return value is currently always ignored.
///
/// \code{.cpp}
///   void MyConsoleFunc1(int a, float b, const char* sz) { ... }
///   ezConsoleFunction<void ()> ConFunc_MyConsoleFunc1("MyConsoleFunc1", "()", MyConsoleFunc1); 
///
///   int MyConsoleFunc2(int a, float b, const char* sz) { ... }
///   ezConsoleFunction<int (int, float, ezString)> ConFunc_MyConsoleFunc2("MyConsoleFunc2", "(int a, float b, string c)", MyConsoleFunc2); 
/// \endcode
///
/// Here the global function MyConsoleFunc2 is exposed to the console. The return value type and parameter types are passed as template
/// arguments. ConFunc_MyConsoleFunc2 is now the global variable that represents the function for the console.
/// The first string is the name with which the function is exposed, which is also used for auto-completion.
/// The second string is the description of the function. Here we inserted the parameter list with types, so that the user knows how to
/// use it. Finally the last parameter is the actual function to expose.
class EZ_COREUTILS_DLL ezConsoleFunctionBase : public ezEnumerable<ezConsoleFunctionBase>
{
  EZ_DECLARE_ENUMERABLE_CLASS(ezConsoleFunctionBase);

public:

  /// \brief The constructor takes the function name and description as it should appear in the console.
  ezConsoleFunctionBase(const char* szFunctionName, const char* szDescription)
  {
    m_szFunctionName = szFunctionName;
    m_szDescription = szDescription;
  }

  /// \brief Returns the name of the function as it should be exposed in the console.
  const char* GetName() const { return m_szFunctionName; }

  /// \brief Returns the description of the function as it should appear in the console.
  const char* GetDescription() const { return m_szDescription; }

  /// \brief Returns the number of parameters that this function takes.
  virtual ezUInt32 GetNumParameters() const = 0;

  /// \brief Returns the type of the n-th parameter.
  virtual ezVariant::Type::Enum GetParameterType(ezUInt32 uiParam) const = 0;

  /// \brief Calls the function. Each parameter must be put into an ezVariant and all of them are passed along as an array.
  ///
  /// Returns EZ_FAILURE, if the number of parameters did not match, or any parameter was not convertible to the actual type that
  /// the function expects.
  virtual ezResult Call(ezArrayPtr<ezVariant> params) = 0;

private:
  const char* m_szFunctionName;
  const char* m_szDescription;
};


/// \brief Implements the functionality of ezConsoleFunctionBase for functions with different parameter types. See ezConsoleFunctionBase for more details.
template<typename R>
class ezConsoleFunction : public ezConsoleFunctionBase
{
};


#define ARG_COUNT 0
#include <CoreUtils/Console/Implementation/ConsoleFunctionHelper_inl.h>
#undef ARG_COUNT

#define ARG_COUNT 1
#include <CoreUtils/Console/Implementation/ConsoleFunctionHelper_inl.h>
#undef ARG_COUNT

#define ARG_COUNT 2
#include <CoreUtils/Console/Implementation/ConsoleFunctionHelper_inl.h>
#undef ARG_COUNT

#define ARG_COUNT 3
#include <CoreUtils/Console/Implementation/ConsoleFunctionHelper_inl.h>
#undef ARG_COUNT

#define ARG_COUNT 4
#include <CoreUtils/Console/Implementation/ConsoleFunctionHelper_inl.h>
#undef ARG_COUNT

#define ARG_COUNT 5
#include <CoreUtils/Console/Implementation/ConsoleFunctionHelper_inl.h>
#undef ARG_COUNT

#define ARG_COUNT 6
#include <CoreUtils/Console/Implementation/ConsoleFunctionHelper_inl.h>
#undef ARG_COUNT

