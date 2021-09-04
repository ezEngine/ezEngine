#pragma once

#include <Core/Console/ConsoleFunction.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/StaticArray.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Types/RefCounted.h>
#include <Foundation/Types/SharedPtr.h>

struct EZ_CORE_DLL ezConsoleString
{
  enum class Type : ezUInt8
  {
    Default,
    Error,
    SeriousWarning,
    Warning,
    Note,
    Success,
    Executed,
    VarName,
    FuncName,
    Dev,
    Debug,
  };

  Type m_Type = Type::Default;
  ezString m_sText;
  ezColor GetColor() const;

  bool operator<(const ezConsoleString& rhs) const { return m_sText < rhs.m_sText; }
};

struct EZ_CORE_DLL ezCommandInterpreterState
{
  ezStringBuilder m_sInput;
  ezHybridArray<ezConsoleString, 16> m_sOutput;

  void AddOutputLine(const ezFormatString& text, ezConsoleString::Type type = ezConsoleString::Type::Default);
};

class EZ_CORE_DLL ezCommandInterpreter : public ezRefCounted
{
public:
  virtual void Interpret(ezCommandInterpreterState& inout_State) = 0;

  virtual void AutoComplete(ezCommandInterpreterState& inout_State);

  /// \brief Iterates over all cvars and finds all that start with the string \a szVariable.
  static void FindPossibleCVars(const char* szVariable, ezDeque<ezString>& CommonStrings, ezDeque<ezConsoleString>& ConsoleStrings);

  /// \brief Iterates over all console functions and finds all that start with the string \a szVariable.
  static void FindPossibleFunctions(const char* szVariable, ezDeque<ezString>& CommonStrings, ezDeque<ezConsoleString>& ConsoleStrings);

  /// \brief Returns the prefix string that is common to all strings in the \a vStrings array.
  static const ezString FindCommonString(const ezDeque<ezString>& vStrings);
};

/// \brief The event data that is broadcast by the console
struct ezConsoleEvent
{
  enum class Type : ezInt32
  {
    OutputLineAdded, ///< A string was added to the console
  };

  Type m_Type;

  /// \brief The console string that was just added.
  const ezConsoleString* m_AddedpConsoleString;
};

class EZ_CORE_DLL ezConsole
{
public:
  ezConsole();
  virtual ~ezConsole();

  /// \name Events
  /// @{

public:
  /// \brief Grants access to subscribe and unsubscribe from console events.
  const ezEvent<const ezConsoleEvent&>& Events() const { return m_Events; }

protected:
  /// \brief The console event variable, to attach to.
  ezEvent<const ezConsoleEvent&> m_Events;

  /// @}

  /// \name Helpers
  /// @{

public:
  /// \brief Returns the mutex that's used to prevent multi-threaded access
  ezMutex& GetMutex() const { return m_Mutex; }

  static void SetMainConsole(ezConsole* pConsole);
  static ezConsole* GetMainConsole();

protected:
  mutable ezMutex m_Mutex;

private:
  static ezConsole* s_pMainConsole;

  /// @}

  /// \name Command Interpreter
  /// @{

public:
  /// \brief Replaces the current command interpreter.
  ///
  /// This base class doesn't set any default interpreter, but derived classes may do so.
  void SetCommandInterpreter(const ezSharedPtr<ezCommandInterpreter>& interpreter) { m_CommandInterpreter = interpreter; }

  /// \brief Returns the currently used command interpreter.
  const ezSharedPtr<ezCommandInterpreter>& GetCommandInterpreter() const { return m_CommandInterpreter; }

  /// \brief Auto-completes the given text.
  ///
  /// Returns true, if the string was modified in any way.
  /// Adds additional strings to the console output, if there are further auto-completion suggestions.
  virtual bool AutoComplete(ezStringBuilder& text);

  /// \brief Executes the given input string.
  ///
  /// The command is forwarded to the set command interpreter.
  virtual void ExecuteCommand(ezStringView input);

protected:
  ezSharedPtr<ezCommandInterpreter> m_CommandInterpreter;

  /// @}

  /// \name Console Display
  /// @{

public:
  /// \brief Adds a string to the console.
  ///
  /// The base class only broadcasts an event, but does not store the string anywhere.
  virtual void AddConsoleString(ezStringView text, ezConsoleString::Type type = ezConsoleString::Type::Default);

  /// @}

  /// \name Input History
  /// @{

public:
  /// \brief Adds an item to the input history.
  void AddToInputHistory(ezStringView text);

  /// \brief Returns the current input history.
  ///
  /// Make sure to lock the console's mutex while working with the history.
  const ezStaticArray<ezString, 16>& GetInputHistory() const { return m_InputHistory; }

  /// \brief Replaces the input line by the next (or previous) history item.
  void RetrieveInputHistory(ezInt32 iHistoryUp, ezStringBuilder& result);

protected:
  ezInt32 m_iCurrentInputHistoryElement = -1;
  ezStaticArray<ezString, 16> m_InputHistory;

  /// @}
};
