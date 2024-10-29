#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptAttributes.h>
#include <Core/Scripting/ScriptClasses/ScriptExtensionClass_CVar.h>

#include <Foundation/Configuration/CVar.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezScriptExtensionClass_CVar, ezNoBase, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(GetValue, In, "Name")->AddFlags(ezPropertyFlags::Const),
    EZ_SCRIPT_FUNCTION_PROPERTY(GetBoolValue, In, "Name")->AddFlags(ezPropertyFlags::Const),
    EZ_SCRIPT_FUNCTION_PROPERTY(GetIntValue, In, "Name")->AddFlags(ezPropertyFlags::Const),
    EZ_SCRIPT_FUNCTION_PROPERTY(GetFloatValue, In, "Name")->AddFlags(ezPropertyFlags::Const),
    EZ_SCRIPT_FUNCTION_PROPERTY(GetStringValue, In, "Name")->AddFlags(ezPropertyFlags::Const),
    EZ_SCRIPT_FUNCTION_PROPERTY(SetValue, In, "Name", In, "Value"),
    EZ_SCRIPT_FUNCTION_PROPERTY(SetBoolValue, In, "Name", In, "Value"),
    EZ_SCRIPT_FUNCTION_PROPERTY(SetIntValue, In, "Name", In, "Value"),
    EZ_SCRIPT_FUNCTION_PROPERTY(SetFloatValue, In, "Name", In, "Value"),
    EZ_SCRIPT_FUNCTION_PROPERTY(SetStringValue, In, "Name", In, "Value"),
  }
  EZ_END_FUNCTIONS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezScriptExtensionAttribute("CVar"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_STATIC_REFLECTED_TYPE;
// clang-format on

static ezHashTable<ezTempHashedString, ezCVar*> s_CachedCVars;

static ezCVar* FindCVarByNameCached(ezStringView sName)
{
  ezTempHashedString sNameHashed(sName);

  ezCVar* pCVar = nullptr;
  if (!s_CachedCVars.TryGetValue(sNameHashed, pCVar))
  {
    pCVar = ezCVar::FindCVarByName(sName);

    s_CachedCVars.Insert(sNameHashed, pCVar);
  }

  ezCVar::s_AllCVarEvents.AddEventHandler(
    [&](const ezCVarEvent& e)
    {
      if (e.m_EventType == ezCVarEvent::Type::ListOfVarsChanged)
      {
        s_CachedCVars.Clear();
      }
    });

  return pCVar;
}

// static
ezVariant ezScriptExtensionClass_CVar::GetValue(ezStringView sName)
{
  ezCVar* pCVar = FindCVarByNameCached(sName);
  if (pCVar == nullptr)
  {
    return {};
  }

  switch (pCVar->GetType())
  {
    case ezCVarType::Bool:
      return static_cast<ezCVarBool*>(pCVar)->GetValue();
    case ezCVarType::Int:
      return static_cast<ezCVarInt*>(pCVar)->GetValue();
    case ezCVarType::Float:
      return static_cast<ezCVarFloat*>(pCVar)->GetValue();
    case ezCVarType::String:
      return static_cast<ezCVarString*>(pCVar)->GetValue();

    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
  }

  return {};
}

// static
bool ezScriptExtensionClass_CVar::GetBoolValue(ezStringView sName)
{
  ezCVar* pCVar = FindCVarByNameCached(sName);
  if (pCVar == nullptr || pCVar->GetType() != ezCVarType::Bool)
  {
    ezLog::Error("CVar '{}' does not exist or is not of type bool.", sName);
    return false;
  }

  return static_cast<ezCVarBool*>(pCVar)->GetValue();
}

// static
int ezScriptExtensionClass_CVar::GetIntValue(ezStringView sName)
{
  ezCVar* pCVar = FindCVarByNameCached(sName);
  if (pCVar == nullptr || pCVar->GetType() != ezCVarType::Int)
  {
    ezLog::Error("CVar '{}' does not exist or is not of type int.", sName);
    return 0;
  }

  return static_cast<ezCVarInt*>(pCVar)->GetValue();
}

// static
float ezScriptExtensionClass_CVar::GetFloatValue(ezStringView sName)
{
  ezCVar* pCVar = FindCVarByNameCached(sName);
  if (pCVar == nullptr || pCVar->GetType() != ezCVarType::Float)
  {
    ezLog::Error("CVar '{}' does not exist or is not of type float.", sName);
    return 0;
  }

  return static_cast<ezCVarFloat*>(pCVar)->GetValue();
}

// static
ezString ezScriptExtensionClass_CVar::GetStringValue(ezStringView sName)
{
  ezCVar* pCVar = FindCVarByNameCached(sName);
  if (pCVar == nullptr || pCVar->GetType() != ezCVarType::String)
  {
    ezLog::Error("CVar '{}' does not exist or is not of type string.", sName);
    return "";
  }

  return static_cast<ezCVarString*>(pCVar)->GetValue();
}

// static
void ezScriptExtensionClass_CVar::SetValue(ezStringView sName, const ezVariant& value)
{
  ezCVar* pCVar = FindCVarByNameCached(sName);
  if (pCVar == nullptr)
  {
    ezLog::Error("CVar '{}' does not exist.", sName);
    return;
  }

  switch (pCVar->GetType())
  {
    case ezCVarType::Bool:
    {
      ezCVarBool* pVar = static_cast<ezCVarBool*>(pCVar);
      *pVar = value.ConvertTo<bool>();
      break;
    }

    case ezCVarType::Int:
    {
      ezCVarInt* pVar = static_cast<ezCVarInt*>(pCVar);
      *pVar = value.ConvertTo<int>();
      break;
    }

    case ezCVarType::Float:
    {
      ezCVarFloat* pVar = static_cast<ezCVarFloat*>(pCVar);
      *pVar = value.ConvertTo<float>();
      break;
    }

    case ezCVarType::String:
    {
      ezCVarString* pVar = static_cast<ezCVarString*>(pCVar);
      *pVar = value.ConvertTo<ezString>();
      break;
    }

    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
  }
}

// static
void ezScriptExtensionClass_CVar::SetBoolValue(ezStringView sName, bool bValue)
{
  ezCVar* pCVar = FindCVarByNameCached(sName);
  if (pCVar == nullptr || pCVar->GetType() != ezCVarType::Bool)
  {
    ezLog::Error("CVar '{}' does not exist or is not of type bool.", sName);
    return;
  }

  ezCVarBool* pVar = static_cast<ezCVarBool*>(pCVar);
  *pVar = bValue;
}

// static
void ezScriptExtensionClass_CVar::SetIntValue(ezStringView sName, int iValue)
{
  ezCVar* pCVar = FindCVarByNameCached(sName);
  if (pCVar == nullptr || pCVar->GetType() != ezCVarType::Int)
  {
    ezLog::Error("CVar '{}' does not exist or is not of type int.", sName);
    return;
  }

  ezCVarInt* pVar = static_cast<ezCVarInt*>(pCVar);
  *pVar = iValue;
}

// static
void ezScriptExtensionClass_CVar::SetFloatValue(ezStringView sName, float fValue)
{
  ezCVar* pCVar = FindCVarByNameCached(sName);
  if (pCVar == nullptr || pCVar->GetType() != ezCVarType::Float)
  {
    ezLog::Error("CVar '{}' does not exist or is not of type float.", sName);
    return;
  }

  ezCVarFloat* pVar = static_cast<ezCVarFloat*>(pCVar);
  *pVar = fValue;
}

// static
void ezScriptExtensionClass_CVar::SetStringValue(ezStringView sName, const ezString& sValue)
{
  ezCVar* pCVar = FindCVarByNameCached(sName);
  if (pCVar == nullptr || pCVar->GetType() != ezCVarType::String)
  {
    ezLog::Error("CVar '{}' does not exist or is not of type string.", sName);
    return;
  }

  ezCVarString* pVar = static_cast<ezCVarString*>(pCVar);
  *pVar = sValue;
}
