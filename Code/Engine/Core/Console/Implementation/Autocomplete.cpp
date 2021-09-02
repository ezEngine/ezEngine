#include <Core/CorePCH.h>

#include <Core/Console/Console.h>

void ezConsole::AutoCompleteInputLine()
{
  if (m_CommandInterpreter)
  {
    ezCommandInterpreterState s;
    s.m_sInput = m_sInputLine;

    m_CommandInterpreter->AutoComplete(s);

    for (auto& l : s.m_sOutput)
    {
      AddConsoleString(l.m_sText, l.m_Type);
    }

    if (m_sInputLine != s.m_sInput)
    {
      m_sInputLine = s.m_sInput;
      MoveCaret(500);
    }
  }
}

void ezCommandInterpreter::FindPossibleCVars(const char* szVariable, ezDeque<ezString>& AutoCompleteOptions, ezDeque<ezConsoleString>& AutoCompleteDescriptions)
{
  ezStringBuilder sText;

  ezCVar* pCVar = ezCVar::GetFirstInstance();
  while (pCVar)
  {
    if (ezStringUtils::StartsWith_NoCase(pCVar->GetName(), szVariable))
    {
      sText.Format("    {0} = {1}", pCVar->GetName(), ezConsole::GetFullInfoAsString(pCVar));

      ezConsoleString cs;
      cs.m_sText = sText;
      cs.m_Type = ezConsoleString::Type::VarName;
      AutoCompleteDescriptions.PushBack(cs);

      AutoCompleteOptions.PushBack(pCVar->GetName());
    }

    pCVar = pCVar->GetNextInstance();
  }
}

void ezCommandInterpreter::FindPossibleFunctions(const char* szVariable, ezDeque<ezString>& AutoCompleteOptions, ezDeque<ezConsoleString>& AutoCompleteDescriptions)
{
  ezStringBuilder sText;

  ezConsoleFunctionBase* pFunc = ezConsoleFunctionBase::GetFirstInstance();
  while (pFunc)
  {
    if (ezStringUtils::StartsWith_NoCase(pFunc->GetName(), szVariable))
    {
      sText.Format("    {0} {1}", pFunc->GetName(), pFunc->GetDescription());

      ezConsoleString cs;
      cs.m_sText = sText;
      cs.m_Type = ezConsoleString::Type::FuncName;
      AutoCompleteDescriptions.PushBack(cs);

      AutoCompleteOptions.PushBack(pFunc->GetName());
    }

    pFunc = pFunc->GetNextInstance();
  }
}


const ezString ezConsole::GetValueAsString(ezCVar* pCVar)
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

ezString ezConsole::GetFullInfoAsString(ezCVar* pCVar)
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

const ezString ezCommandInterpreter::FindCommonString(const ezDeque<ezString>& vStrings)
{
  ezStringBuilder sCommon;
  ezUInt32 c;

  ezUInt32 uiPos = 0;
  auto it1 = vStrings[0].GetIteratorFront();
  while (it1.IsValid())
  {
    c = it1.GetCharacter();

    for (int v = 1; v < (int)vStrings.GetCount(); v++)
    {
      auto it2 = vStrings[v].GetIteratorFront();

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

void ezCommandInterpreter::AutoComplete(ezCommandInterpreterState& inout_State)
{
  ezString sVarName = inout_State.m_sInput;

  auto it = rbegin(inout_State.m_sInput);

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

    inout_State.AddOutputLine("");

    for (ezUInt32 i = 0; i < AutoCompleteDescriptions.GetCount(); i++)
    {
      inout_State.AddOutputLine(AutoCompleteDescriptions[i].m_sText.GetData(), AutoCompleteDescriptions[i].m_Type);
    }

    inout_State.AddOutputLine("");
  }

  if (AutoCompleteOptions.GetCount() > 0)
  {
    if (szLastWordDelimiter != nullptr)
      inout_State.m_sInput = ezStringView(inout_State.m_sInput.GetData(), szLastWordDelimiter + 1);
    else
      inout_State.m_sInput.Clear();

    inout_State.m_sInput.Append(FindCommonString(AutoCompleteOptions).GetData());
  }
}


EZ_STATICLINK_FILE(Core, Core_Console_Implementation_Autocomplete);
