#pragma once

#include <Core/Console/Console.h>

#ifdef BUILDSYSTEM_ENABLE_LUA_SUPPORT

class EZ_CORE_DLL ezCommandInterpreterLua : public ezCommandInterpreter
{
public:
  /// \brief The default interpreter used by ezConsole. Uses Lua for parsing and execution.
  ///
  /// The Lua interpreter can modify ezCVar variables and call ezConsoleFunction functions.
  ///
  /// Typing 'some_bool_cvar =' is a short form for 'some_bool_cvar = not some_bool_cvar'
  /// which toggles the value of the boolean CVar variable.
  ///
  /// Mathematical operations are possible, such as 'int_cvar = int_cvar + 1',
  /// which can be used to modify the CVar gradually.
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
  virtual ezResult Interpret(ezCommandInterpreterState& inout_State) override;
};

#endif // BUILDSYSTEM_ENABLE_LUA_SUPPORT
