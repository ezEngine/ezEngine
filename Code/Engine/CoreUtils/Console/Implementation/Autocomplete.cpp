#include <CoreUtils/PCH.h>
#include <CoreUtils/Console/Console.h>


void ezConsole::AutoCompleteInputLine()
{
  ezString sVarName = m_sInputLine;

  auto it = rbegin(m_sInputLine);

  while (it.IsValid() && !ezStringUtils::IsIdentifierDelimiter_C_Code(*it))
    ++it;

  const char* szLastWordDelimiter = nullptr;
  if (it.IsValid() && ezStringUtils::IsIdentifierDelimiter_C_Code(*it))
    szLastWordDelimiter = it.GetData();

  if (szLastWordDelimiter != nullptr)
    sVarName = szLastWordDelimiter + 1;

  ezDeque<ezString> AutoCompleteOptions;
  ezDeque<ConsoleString> AutoCompleteDescriptions;

  FindPossibleCVars(sVarName.GetData(), AutoCompleteOptions, AutoCompleteDescriptions);
  FindPossibleFunctions(sVarName.GetData(), AutoCompleteOptions, AutoCompleteDescriptions);

  // Broadcast that we want to autocomplete this and let other code add autocomplete suggestions to our arrays
  {
    ConsoleEvent ce;
    ce.m_EventType = ConsoleEvent::AutoCompleteRequest;
    ce.m_szCommand = sVarName.GetData();
    ce.m_pAutoCompleteOptions = &AutoCompleteOptions;
    ce.m_pAutoCompleteDescriptions = &AutoCompleteDescriptions;

    m_Events.Broadcast(ce);
  }

  if (AutoCompleteDescriptions.GetCount() > 1)
  {
    AutoCompleteDescriptions.Sort();

    AddConsoleString("");

    for (ezUInt32 i = 0; i < AutoCompleteDescriptions.GetCount(); i++)
      AddConsoleString(AutoCompleteDescriptions[i].m_sText.GetData(), AutoCompleteDescriptions[i].m_TextColor);

    AddConsoleString("");
  }

  if (AutoCompleteOptions.GetCount() > 0)
  {
    if (szLastWordDelimiter != nullptr)
      m_sInputLine = ezStringView(m_sInputLine.GetData(), szLastWordDelimiter + 1);
    else
      m_sInputLine.Clear();

    m_sInputLine.Append(FindCommonString(AutoCompleteOptions).GetData());

    MoveCaret(500);
  }
}

void ezConsole::FindPossibleCVars(const char* szVariable, ezDeque<ezString>& AutoCompleteOptions, ezDeque<ConsoleString>& AutoCompleteDescriptions)
{
  ezStringBuilder sText;

  ezCVar* pCVar = ezCVar::GetFirstInstance();
  while (pCVar)
  {
    if (ezStringUtils::StartsWith_NoCase(pCVar->GetName(), szVariable))
    {
      sText.Format("    %s = %s", pCVar->GetName(), GetFullInfoAsString(pCVar).GetData());

      ConsoleString cs;
      cs.m_sText = sText;
      cs.m_TextColor = ezColorGammaUB(255, 210, 0);
      cs.m_bShowOnScreen = false;
      cs.m_TimeStamp = ezTime::Now();
      AutoCompleteDescriptions.PushBack(cs);

      AutoCompleteOptions.PushBack(pCVar->GetName());
    }

    pCVar = pCVar->GetNextInstance();
  }
}

void ezConsole::FindPossibleFunctions(const char* szVariable, ezDeque<ezString>& AutoCompleteOptions, ezDeque<ConsoleString>& AutoCompleteDescriptions)
{
  ezStringBuilder sText;

  ezConsoleFunctionBase* pFunc = ezConsoleFunctionBase::GetFirstInstance();
  while (pFunc)
  {
    if (ezStringUtils::StartsWith_NoCase(pFunc->GetName(), szVariable))
    {
      sText.Format("    %s %s", pFunc->GetName(), pFunc->GetDescription());

      ConsoleString cs;
      cs.m_sText = sText;
      cs.m_TextColor = ezColorGammaUB(100, 255, 100);
      cs.m_bShowOnScreen = false;
      cs.m_TimeStamp = ezTime::Now();
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
      ezCVarInt* pInt = static_cast<ezCVarInt*> (pCVar);
      s.Format("%d", pInt->GetValue());
    }
    break;

  case ezCVarType::Bool:
    {
      ezCVarBool* pBool = static_cast<ezCVarBool*> (pCVar);
      if (pBool->GetValue() == true)
        s = "true";
      else
        s = "false";
    }
    break;

  case ezCVarType::String:
    {
      ezCVarString* pString = static_cast<ezCVarString*> (pCVar);
      s.Format("\"%s\"", pString->GetValue().GetData());
    }
    break;

  case ezCVarType::Float:
    {
      ezCVarFloat* pFloat = static_cast<ezCVarFloat*> (pCVar);
      s.Format("%.3f", pFloat->GetValue());
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

const ezString ezConsole::FindCommonString(const ezDeque<ezString>& vStrings)
{
  ezStringBuilder sCommon;
  ezUInt32 c;

  ezUInt32 uiPos = 0;
  auto it1 = vStrings[0].GetIteratorFront();
  while (it1.IsValid())
  {
    c = it1.GetCharacter();

    for (int v = 1; v < (int) vStrings.GetCount(); v++)
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



EZ_STATICLINK_FILE(CoreUtils, CoreUtils_Console_Implementation_Autocomplete);

