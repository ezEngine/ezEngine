#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Types/RefCounted.h>

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
  virtual void Interpret(ezCommandInterpreterState& inout_state) = 0;

  virtual void AutoComplete(ezCommandInterpreterState& inout_state);

  /// \brief Iterates over all cvars and finds all that start with the string \a szVariable.
  static void FindPossibleCVars(ezStringView sVariable, ezDeque<ezString>& ref_commonStrings, ezDeque<ezConsoleString>& ref_consoleStrings);

  /// \brief Iterates over all console functions and finds all that start with the string \a szVariable.
  static void FindPossibleFunctions(ezStringView sVariable, ezDeque<ezString>& ref_commonStrings, ezDeque<ezConsoleString>& ref_consoleStrings);

  /// \brief Returns the prefix string that is common to all strings in the \a vStrings array.
  static const ezString FindCommonString(const ezDeque<ezString>& strings);

  /// \name Helpers
  /// @{

  /// \brief Returns a nice string containing all the important information about the cvar.
  static ezString GetFullInfoAsString(ezCVar* pCVar);

  /// \brief Returns the value of the cvar as a string.
  static const ezString GetValueAsString(ezCVar* pCVar);

  /// @}
};
