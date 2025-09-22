#pragma once

#include <Core/Console/CommandInterpreter.h>
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

/// \brief Base console system for command input, output display, and history management.
///
/// Provides infrastructure for command execution through pluggable interpreters,
/// maintains input history, and broadcasts events when output is added.
/// Thread-safe through internal mutex protection.
///
/// This base class handles core functionality but doesn't store output strings.
/// Derived classes typically provide persistent storage and visual representation.
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
  void SetCommandInterpreter(const ezSharedPtr<ezCommandInterpreter>& pInterpreter) { m_pCommandInterpreter = pInterpreter; }

  /// \brief Returns the currently used command interpreter.
  const ezSharedPtr<ezCommandInterpreter>& GetCommandInterpreter() const { return m_pCommandInterpreter; }

  /// \brief Auto-completes the given text.
  ///
  /// Returns true, if the string was modified in any way.
  /// Adds additional strings to the console output, if there are further auto-completion suggestions.
  virtual bool AutoComplete(ezStringBuilder& ref_sText);

  /// \brief Executes the given input string.
  ///
  /// The command is forwarded to the set command interpreter.
  virtual void ExecuteCommand(ezStringView sInput);

protected:
  ezSharedPtr<ezCommandInterpreter> m_pCommandInterpreter;

  /// @}

  /// \name Console Display
  /// @{

public:
  /// \brief Adds a string to the console.
  ///
  /// The base class only broadcasts an event, but does not store the string anywhere.
  virtual void AddConsoleString(ezStringView sText, ezConsoleString::Type type = ezConsoleString::Type::Default);

  /// \brief Display the console state.
  virtual void RenderConsole(bool bIsOpen) {}

  /// @}

  /// \name Input
  /// @{

public:
  /// \brief Update the console with the latest input.
  virtual void HandleInput(bool bIsOpen) {}

  /// \brief Adds an item to the input history.
  void AddToInputHistory(ezStringView sText);

  /// \brief Returns the current input history.
  ///
  /// Make sure to lock the console's mutex while working with the history.
  const ezStaticArray<ezString, 16>& GetInputHistory() const { return m_InputHistory; }

  /// \brief Replaces the input line by the next (or previous) history item.
  void RetrieveInputHistory(ezInt32 iHistoryUp, ezStringBuilder& ref_sResult);

  /// \brief Writes the current input history to a text file.
  ezResult SaveInputHistory(ezStringView sFile);

  /// \brief Reads the text file and appends all lines to the input history.
  void LoadInputHistory(ezStringView sFile);

protected:
  ezInt32 m_iCurrentInputHistoryElement = -1;
  ezStaticArray<ezString, 16> m_InputHistory;

  /// @}
};
