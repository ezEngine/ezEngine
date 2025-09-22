#include <GameEngine/GameEnginePCH.h>

#include <Core/Scripting/LuaWrapper.h>
#include <GameEngine/Console/LuaInterpreter.h>

#ifdef BUILDSYSTEM_ENABLE_LUA_SUPPORT

static void AllowScriptCVarAccess(ezLuaWrapper& ref_script);

static const ezString GetNextWord(ezStringView& ref_sString)
{
  const char* szStartWord = ezStringUtils::SkipCharacters(ref_sString.GetStartPointer(), ezStringUtils::IsWhiteSpace, false);
  const char* szEndWord = ezStringUtils::FindWordEnd(szStartWord, ezStringUtils::IsIdentifierDelimiter_C_Code, true);

  ref_sString = ezStringView(szEndWord);

  return ezStringView(szStartWord, szEndWord);
}

static ezString GetRestWords(ezStringView sString)
{
  return ezStringUtils::SkipCharacters(sString.GetStartPointer(), ezStringUtils::IsWhiteSpace, false);
}

static int LUAFUNC_ConsoleFunc(lua_State* pState)
{
  ezLuaWrapper s(pState);

  ezConsoleFunctionBase* pFunc = (ezConsoleFunctionBase*)s.GetFunctionLightUserData();

  if (pFunc->GetNumParameters() != s.GetNumberOfFunctionParameters())
  {
    ezLog::Error("Function '{0}' expects {1} parameters, {2} were provided.", pFunc->GetName(), pFunc->GetNumParameters(), s.GetNumberOfFunctionParameters());
    return s.ReturnToScript();
  }

  ezHybridArray<ezVariant, 8> m_Params;
  m_Params.SetCount(pFunc->GetNumParameters());

  for (ezUInt32 p = 0; p < pFunc->GetNumParameters(); ++p)
  {
    switch (pFunc->GetParameterType(p))
    {
      case ezVariant::Type::Bool:
        m_Params[p] = s.GetBoolParameter(p);
        break;
      case ezVariant::Type::Int8:
      case ezVariant::Type::Int16:
      case ezVariant::Type::Int32:
      case ezVariant::Type::Int64:
      case ezVariant::Type::UInt8:
      case ezVariant::Type::UInt16:
      case ezVariant::Type::UInt32:
      case ezVariant::Type::UInt64:
        m_Params[p] = s.GetIntParameter(p);
        break;
      case ezVariant::Type::Float:
      case ezVariant::Type::Double:
        m_Params[p] = s.GetFloatParameter(p);
        break;
      case ezVariant::Type::String:
        m_Params[p] = s.GetStringParameter(p);
        break;
      default:
        ezLog::Error("Function '{0}': Type of parameter {1} is not supported by the Lua interpreter.", pFunc->GetName(), p);
        return s.ReturnToScript();
    }
  }

  if (!m_Params.IsEmpty())
    pFunc->Call(ezArrayPtr<ezVariant>(&m_Params[0], m_Params.GetCount())).IgnoreResult();
  else
    pFunc->Call(ezArrayPtr<ezVariant>()).IgnoreResult();

  return s.ReturnToScript();
}

static void SanitizeCVarNames(ezStringBuilder& ref_sCommand)
{
  ezStringBuilder sanitizedCVarName;

  for (const ezCVar* pCVar = ezCVar::GetFirstInstance(); pCVar != nullptr; pCVar = pCVar->GetNextInstance())
  {
    sanitizedCVarName = pCVar->GetName();
    sanitizedCVarName.ReplaceAll(".", "_");

    ref_sCommand.ReplaceAll(pCVar->GetName(), sanitizedCVarName);
  }
}

static void UnSanitizeCVarName(ezStringBuilder& ref_sCvarName)
{
  ezStringBuilder sanitizedCVarName;

  for (const ezCVar* pCVar = ezCVar::GetFirstInstance(); pCVar != nullptr; pCVar = pCVar->GetNextInstance())
  {
    sanitizedCVarName = pCVar->GetName();
    sanitizedCVarName.ReplaceAll(".", "_");

    if (ref_sCvarName == sanitizedCVarName)
    {
      ref_sCvarName = pCVar->GetName();
      return;
    }
  }
}

void ezCommandInterpreterLua::Interpret(ezCommandInterpreterState& inout_state)
{
  inout_state.m_sOutput.Clear();

  ezStringBuilder sRealCommand = inout_state.m_sInput;

  if (sRealCommand.IsEmpty())
  {
    inout_state.AddOutputLine("");
    return;
  }

  sRealCommand.Trim(" \t\n\r");
  ezStringBuilder sSanitizedCommand = sRealCommand;
  SanitizeCVarNames(sSanitizedCommand);

  ezStringView sCommandIt = sSanitizedCommand;

  const ezString sSanitizedVarName = GetNextWord(sCommandIt);
  ezStringBuilder sRealVarName = sSanitizedVarName;
  UnSanitizeCVarName(sRealVarName);

  while (ezStringUtils::IsWhiteSpace(sCommandIt.GetCharacter()))
  {
    sCommandIt.Shrink(1, 0);
  }

  const bool bSetValue = sCommandIt.StartsWith("=");

  if (bSetValue)
  {
    sCommandIt.Shrink(1, 0);
  }

  ezStringBuilder sValue = GetRestWords(sCommandIt);
  bool bValueEmpty = sValue.IsEmpty();

  ezStringBuilder sTemp;

  ezLuaWrapper Script;
  AllowScriptCVarAccess(Script);

  // Register all ConsoleFunctions
  {
    ezConsoleFunctionBase* pFunc = ezConsoleFunctionBase::GetFirstInstance();
    while (pFunc)
    {
      Script.RegisterCFunction(pFunc->GetName().GetData(sTemp), LUAFUNC_ConsoleFunc, pFunc);

      pFunc = pFunc->GetNextInstance();
    }
  }

  sTemp = "> ";
  sTemp.Append(sRealCommand);
  inout_state.AddOutputLine(sTemp, ezConsoleString::Type::Executed);

  ezCVar* pCVAR = ezCVar::FindCVarByName(sRealVarName.GetData());
  if (pCVAR != nullptr)
  {
    if ((bSetValue) && (sValue == "") && (pCVAR->GetType() == ezCVarType::Bool))
    {
      // someone typed "myvar =" -> on bools this is the short form for "myvar = not myvar" (toggle), so insert the rest here

      bValueEmpty = false;

      sSanitizedCommand.AppendFormat(" not {0}", sSanitizedVarName);
    }

    if (bSetValue && !bValueEmpty)
    {
      ezMuteLog muteLog;

      if (Script.ExecuteString(sSanitizedCommand, "console", &muteLog).Failed())
      {
        inout_state.AddOutputLine("  Error Executing Command.", ezConsoleString::Type::Error);
        return;
      }
      else
      {
        if (pCVAR->GetFlags().IsAnySet(ezCVarFlags::ShowRequiresRestartMsg))
        {
          inout_state.AddOutputLine("  This change takes only effect after a restart.", ezConsoleString::Type::Note);
        }

        sTemp.SetFormat("  {0} = {1}", sRealVarName, GetFullInfoAsString(pCVAR));
        inout_state.AddOutputLine(sTemp, ezConsoleString::Type::Success);
      }
    }
    else
    {
      sTemp.SetFormat("{0} = {1}", sRealVarName, GetFullInfoAsString(pCVAR));
      inout_state.AddOutputLine(sTemp);

      if (!pCVAR->GetDescription().IsEmpty())
      {
        sTemp.SetFormat("  Description: {0}", pCVAR->GetDescription());
        inout_state.AddOutputLine(sTemp, ezConsoleString::Type::Success);
      }
      else
        inout_state.AddOutputLine("  No Description available.", ezConsoleString::Type::Success);
    }

    return;
  }
  else
  {
    ezMuteLog muteLog;

    if (Script.ExecuteString(sSanitizedCommand, "console", &muteLog).Failed())
    {
      inout_state.AddOutputLine("  Error Executing Command.", ezConsoleString::Type::Error);
      return;
    }
  }
}

static int LUAFUNC_ReadCVAR(lua_State* pState)
{
  ezLuaWrapper s(pState);

  ezStringBuilder cvarName = s.GetStringParameter(0);
  UnSanitizeCVarName(cvarName);

  ezCVar* pCVar = ezCVar::FindCVarByName(cvarName);

  if (pCVar == nullptr)
  {
    s.PushReturnValueNil();
    return s.ReturnToScript();
  }

  switch (pCVar->GetType())
  {
    case ezCVarType::Int:
    {
      ezCVarInt* pVar = (ezCVarInt*)pCVar;
      s.PushReturnValue(pVar->GetValue());
    }
    break;
    case ezCVarType::Bool:
    {
      ezCVarBool* pVar = (ezCVarBool*)pCVar;
      s.PushReturnValue(pVar->GetValue());
    }
    break;
    case ezCVarType::Float:
    {
      ezCVarFloat* pVar = (ezCVarFloat*)pCVar;
      s.PushReturnValue(pVar->GetValue());
    }
    break;
    case ezCVarType::String:
    {
      ezCVarString* pVar = (ezCVarString*)pCVar;
      s.PushReturnValue(pVar->GetValue().GetData());
    }
    break;
    case ezCVarType::ENUM_COUNT:
      break;
  }

  return s.ReturnToScript();
}


static int LUAFUNC_WriteCVAR(lua_State* pState)
{
  ezLuaWrapper s(pState);

  ezStringBuilder cvarName = s.GetStringParameter(0);
  UnSanitizeCVarName(cvarName);

  ezCVar* pCVar = ezCVar::FindCVarByName(cvarName);

  if (pCVar == nullptr)
  {
    s.PushReturnValue(false);
    return s.ReturnToScript();
  }

  s.PushReturnValue(true);

  switch (pCVar->GetType())
  {
    case ezCVarType::Int:
    {
      ezCVarInt* pVar = (ezCVarInt*)pCVar;
      *pVar = s.GetIntParameter(1);
    }
    break;
    case ezCVarType::Bool:
    {
      ezCVarBool* pVar = (ezCVarBool*)pCVar;
      *pVar = s.GetBoolParameter(1);
    }
    break;
    case ezCVarType::Float:
    {
      ezCVarFloat* pVar = (ezCVarFloat*)pCVar;
      *pVar = s.GetFloatParameter(1);
    }
    break;
    case ezCVarType::String:
    {
      ezCVarString* pVar = (ezCVarString*)pCVar;
      *pVar = s.GetStringParameter(1);
    }
    break;
    case ezCVarType::ENUM_COUNT:
      break;
  }

  return s.ReturnToScript();
}

static void AllowScriptCVarAccess(ezLuaWrapper& ref_script)
{
  ref_script.RegisterCFunction("ReadCVar", LUAFUNC_ReadCVAR);
  ref_script.RegisterCFunction("WriteCVar", LUAFUNC_WriteCVAR);

  ezStringBuilder sInit = "\
function readcvar (t, key)\n\
return (ReadCVar (key))\n\
end\n\
\n\
function writecvar (t, key, value)\n\
if not WriteCVar (key, value) then\n\
rawset (t, key, value or false)\n\
end\n\
end\n\
\n\
setmetatable (_G, {\n\
__newindex = writecvar,\n\
__index = readcvar,\n\
__metatable = \"Access Denied\",\n\
})";

  ref_script.ExecuteString(sInit.GetData()).IgnoreResult();
}

#endif // BUILDSYSTEM_ENABLE_LUA_SUPPORT
