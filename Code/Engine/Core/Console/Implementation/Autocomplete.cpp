#include <Core/CorePCH.h>

#include <Core/Console/Console.h>
#include <Core/Console/QuakeConsole.h>

void ezCommandInterpreter::FindPossibleCVars(ezStringView sVariable, ezDeque<ezString>& inout_autoCompleteOptions, ezDeque<ezConsoleString>& inout_autoCompleteDescriptions)
{
  ezStringBuilder sText;

  ezCVar* pCVar = ezCVar::GetFirstInstance();
  while (pCVar)
  {
    if (pCVar->GetName().StartsWith_NoCase(sVariable))
    {
      sText.Format("    {0} = {1}", pCVar->GetName(), ezQuakeConsole::GetFullInfoAsString(pCVar));

      ezConsoleString cs;
      cs.m_sText = sText;
      cs.m_Type = ezConsoleString::Type::VarName;
      inout_autoCompleteDescriptions.PushBack(cs);

      inout_autoCompleteOptions.PushBack(pCVar->GetName());
    }

    pCVar = pCVar->GetNextInstance();
  }
}

void ezCommandInterpreter::FindPossibleFunctions(ezStringView sVariable, ezDeque<ezString>& inout_autoCompleteOptions, ezDeque<ezConsoleString>& inout_autoCompleteDescriptions)
{
  ezStringBuilder sText;

  ezConsoleFunctionBase* pFunc = ezConsoleFunctionBase::GetFirstInstance();
  while (pFunc)
  {
    if (pFunc->GetName().StartsWith_NoCase(sVariable))
    {
      sText.Format("    {0} {1}", pFunc->GetName(), pFunc->GetDescription());

      ezConsoleString cs;
      cs.m_sText = sText;
      cs.m_Type = ezConsoleString::Type::FuncName;
      inout_autoCompleteDescriptions.PushBack(cs);

      inout_autoCompleteOptions.PushBack(pFunc->GetName());
    }

    pFunc = pFunc->GetNextInstance();
  }
}


const ezString ezQuakeConsole::GetValueAsString(ezCVar* pCVar)
{
  ezStringBuilder s = "undefined";

  switch (pCVar->GetType())
  {
    case ezCVarType::Int:
    {
      ezCVarInt* pInt = static_cast<ezCVarInt*>(pCVar);
      s.Format("{0}", pInt->GetValue());
    }
    break;

    case ezCVarType::Bool:
    {
      ezCVarBool* pBool = static_cast<ezCVarBool*>(pCVar);
      if (pBool->GetValue() == true)
        s = "true";
      else
        s = "false";
    }
    break;

    case ezCVarType::String:
    {
      ezCVarString* pString = static_cast<ezCVarString*>(pCVar);
      s.Format("\"{0}\"", pString->GetValue());
    }
    break;

    case ezCVarType::Float:
    {
      ezCVarFloat* pFloat = static_cast<ezCVarFloat*>(pCVar);
      s.Format("{0}", ezArgF(pFloat->GetValue(), 3));
    }
    break;

    case ezCVarType::ENUM_COUNT:
      break;
  }

  return s.GetData();
}

ezString ezQuakeConsole::GetFullInfoAsString(ezCVar* pCVar)
{
  ezStringBuilder s = GetValueAsString(pCVar);

  const bool bAnyFlags = pCVar->GetFlags().IsAnySet(ezCVarFlags::RequiresRestart | ezCVarFlags::Save);

  if (bAnyFlags)
    s.Append(" [ ");

  if (pCVar->GetFlags().IsAnySet(ezCVarFlags::Save))
    s.Append("SAVE ");

  if (pCVar->GetFlags().IsAnySet(ezCVarFlags::RequiresRestart))
    s.Append("RESTART ");

  if (bAnyFlags)
    s.Append("]");

  return s;
}

const ezString ezCommandInterpreter::FindCommonString(const ezDeque<ezString>& strings)
{
  ezStringBuilder sCommon;
  ezUInt32 c;

  ezUInt32 uiPos = 0;
  auto it1 = strings[0].GetIteratorFront();
  while (it1.IsValid())
  {
    c = it1.GetCharacter();

    for (int v = 1; v < (int)strings.GetCount(); v++)
    {
      auto it2 = strings[v].GetIteratorFront();

      it2 += uiPos;

      if (it2.GetCharacter() != c)
        return sCommon;
    }

    sCommon.Append(c);

    ++uiPos;
    ++it1;
  }

  return sCommon;
}

void ezCommandInterpreter::AutoComplete(ezCommandInterpreterState& inout_state)
{
  ezString sVarName = inout_state.m_sInput;

  auto it = rbegin(inout_state.m_sInput);

  // dots are allowed in CVar names
  while (it.IsValid() && (it.GetCharacter() == '.' || !ezStringUtils::IsIdentifierDelimiter_C_Code(*it)))
    ++it;

  const char* szLastWordDelimiter = nullptr;
  if (it.IsValid() && ezStringUtils::IsIdentifierDelimiter_C_Code(*it) && it.GetCharacter() != '.')
    szLastWordDelimiter = it.GetData();

  if (szLastWordDelimiter != nullptr)
    sVarName = szLastWordDelimiter + 1;

  ezDeque<ezString> AutoCompleteOptions;
  ezDeque<ezConsoleString> AutoCompleteDescriptions;

  FindPossibleCVars(sVarName.GetData(), AutoCompleteOptions, AutoCompleteDescriptions);
  FindPossibleFunctions(sVarName.GetData(), AutoCompleteOptions, AutoCompleteDescriptions);

  if (AutoCompleteDescriptions.GetCount() > 1)
  {
    AutoCompleteDescriptions.Sort();

    inout_state.AddOutputLine("");

    for (ezUInt32 i = 0; i < AutoCompleteDescriptions.GetCount(); i++)
    {
      inout_state.AddOutputLine(AutoCompleteDescriptions[i].m_sText.GetData(), AutoCompleteDescriptions[i].m_Type);
    }

    inout_state.AddOutputLine("");
  }

  if (AutoCompleteOptions.GetCount() > 0)
  {
    if (szLastWordDelimiter != nullptr)
      inout_state.m_sInput = ezStringView(inout_state.m_sInput.GetData(), szLastWordDelimiter + 1);
    else
      inout_state.m_sInput.Clear();

    inout_state.m_sInput.Append(FindCommonString(AutoCompleteOptions).GetData());
  }
}


EZ_STATICLINK_FILE(Core, Core_Console_Implementation_Autocomplete);
