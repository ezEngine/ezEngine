#pragma once

#include <CoreUtils/Console/Console.h>

namespace ezConsoleInterpreter
{
  /// \brief The default interpreter used by ezConsole. Uses Lua for parsing and execution.
  ///
  /// The lua interpreter can modify ezCVar variables and call ezConsoleFunction functions.
  ///
  /// Typing 'some_bool_cvar =' is a short form for 'some_bool_cvar = not some_bool_cvar'
  /// which toggles the value of the boolean cvar variable.
  ///
  /// Mathematical operations are possible, such as 'int_cvar = int_cvar + 1',
  /// which can be used to modify the cvar gradually.
  /// Such a command can be very useful when the console is configured such that it is
  /// easy to re-execute the last commands by hitting just one button.
  /// It can also be used when binding keys to commands.
  ///
  /// CVars can be read and written, thus 'int_cvar1 = int_cvar2' would read the current value
  /// from int_cvar2 and copy it into int_cvar1.
  ///
  /// Console functions are called simply by appending parentheses and (if required) parameters:
  /// SomeConsoleFunc(2, some_cvar)
  ///
  /// If there is any kind of error (e.g. a Lua syntax error), the interpreter will return EZ_FAILURE;
  EZ_COREUTILS_DLL ezResult Lua(const char* szCommand, ezConsole* pConsole);

}

