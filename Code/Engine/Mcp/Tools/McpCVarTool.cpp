#include <Mcp/McpPCH.h>

#include <Mcp/McpJson.h>
#include <Mcp/McpJsonWriter.h>
#include <Mcp/Tools/McpCVarTool.h>

#include <Foundation/Configuration/CVar.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMcpCVarTool, 1, ezRTTIDefaultAllocator<ezMcpCVarTool>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

namespace
{
  /// The names used in tool arguments and results, lower case because that is what the AI has to type.
  ezStringView CVarTypeToString(ezCVarType::Enum type)
  {
    switch (type)
    {
      case ezCVarType::Int:
        return "int";
      case ezCVarType::Float:
        return "float";
      case ezCVarType::Bool:
        return "bool";
      case ezCVarType::String:
        return "string";
      default:
        return "unknown";
    }
  }

  /// Whether the CVar still holds the value it was declared with.
  ///
  /// One place, because both the listing filter and the decision whether to report 'default' need it,
  /// and each type has to be compared through its own pointer cast.
  bool IsAtDefaultValue(const ezCVar* pCVar)
  {
    switch (pCVar->GetType())
    {
      case ezCVarType::Int:
      {
        const ezCVarInt* p = static_cast<const ezCVarInt*>(pCVar);
        return p->GetValue() == p->GetValue(ezCVarValue::Default);
      }
      case ezCVarType::Float:
      {
        const ezCVarFloat* p = static_cast<const ezCVarFloat*>(pCVar);
        return p->GetValue() == p->GetValue(ezCVarValue::Default);
      }
      case ezCVarType::Bool:
      {
        const ezCVarBool* p = static_cast<const ezCVarBool*>(pCVar);
        return p->GetValue() == p->GetValue(ezCVarValue::Default);
      }
      case ezCVarType::String:
      {
        const ezCVarString* p = static_cast<const ezCVarString*>(pCVar);
        return p->GetValue() == p->GetValue(ezCVarValue::Default);
      }
      default:
        return true;
    }
  }

  /// Whether a value was written that the engine is not reading yet. Only ever true for CVars
  /// flagged RequiresDelayedSync.
  bool HasPendingValue(const ezCVar* pCVar)
  {
    switch (pCVar->GetType())
    {
      case ezCVarType::Int:
        return static_cast<const ezCVarInt*>(pCVar)->HasDelayedSyncValueChanged();
      case ezCVarType::Float:
        return static_cast<const ezCVarFloat*>(pCVar)->HasDelayedSyncValueChanged();
      case ezCVarType::Bool:
        return static_cast<const ezCVarBool*>(pCVar)->HasDelayedSyncValueChanged();
      case ezCVarType::String:
        return static_cast<const ezCVarString*>(pCVar)->HasDelayedSyncValueChanged();
      default:
        return false;
    }
  }
} // namespace

void ezMcpCVarTool::WriteValue(ezMcpJsonWriter& ref_writer, ezStringView sFieldName, const ezCVar* pCVar, ezUInt32 uiWhichValue)
{
  const ezCVarValue::Enum which = static_cast<ezCVarValue::Enum>(uiWhichValue);

  // Written as the JSON type that matches the CVar, not as a string: a client that reads a bool and
  // writes it straight back must not have to know it was quoted on the way out.
  switch (pCVar->GetType())
  {
    case ezCVarType::Int:
      ref_writer.AddVariableInt32(sFieldName, static_cast<const ezCVarInt*>(pCVar)->GetValue(which));
      break;
    case ezCVarType::Float:
      ref_writer.AddVariableFloat(sFieldName, static_cast<const ezCVarFloat*>(pCVar)->GetValue(which));
      break;
    case ezCVarType::Bool:
      ref_writer.AddVariableBool(sFieldName, static_cast<const ezCVarBool*>(pCVar)->GetValue(which));
      break;
    case ezCVarType::String:
      ref_writer.AddVariableString(sFieldName, static_cast<const ezCVarString*>(pCVar)->GetValue(which).GetView());
      break;
    default:
      break;
  }
}

void ezMcpCVarTool::WriteCVar(ezMcpJsonWriter& ref_writer, const ezCVar* pCVar)
{
  ref_writer.BeginObject();

  ref_writer.AddVariableString("name", pCVar->GetName());
  ref_writer.AddVariableString("type", CVarTypeToString(pCVar->GetType()));
  WriteValue(ref_writer, "value", pCVar, ezCVarValue::Current);

  // Only when it differs, per the 'omit empty fields' rule - a CVar sitting at its default is the
  // common case and repeating the same number twice costs tokens without saying anything.
  if (!IsAtDefaultValue(pCVar))
  {
    WriteValue(ref_writer, "default", pCVar, ezCVarValue::Default);
  }

  const ezBitflags<ezCVarFlags> flags = pCVar->GetFlags();

  if (flags.IsSet(ezCVarFlags::RequiresDelayedSync))
  {
    // The trap this field exists for: writing such a CVar does not change what the engine reads until
    // the owning subsystem syncs it, so a caller that only re-read 'value' would conclude its write was
    // ignored. Reported whenever the flag is set, not only when a write is outstanding, because that is
    // what tells a caller in advance that a write here will not take effect immediately.
    ref_writer.AddVariableBool("requiresRestart", true);

    if (HasPendingValue(pCVar))
    {
      WriteValue(ref_writer, "pendingValue", pCVar, ezCVarValue::DelayedSync);
    }
  }

  if (flags.IsSet(ezCVarFlags::Save))
  {
    ref_writer.AddVariableBool("saved", true);
  }

  if (!pCVar->GetPluginName().IsEmpty())
  {
    ref_writer.AddVariableString("plugin", pCVar->GetPluginName());
  }

  if (!pCVar->GetDescription().IsEmpty())
  {
    ref_writer.AddVariableString("description", pCVar->GetDescription());
  }

  ref_writer.EndObject();
}

void ezMcpCVarTool::GetSupportedTools(ezDynamicArray<ezMcpToolDesc>& out_tools) const
{
  {
    ezMcpToolDesc& desc = out_tools.ExpandAndGetRef();
    desc.m_sName = "cvar_list";
    desc.m_sDescription = "Lists the CVars registered in this process, with their current value, type, owning plugin and "
                          "description. CVars are the debug and configuration switches the engine and its plugins declare - "
                          "rendering, physics and AI visualisation, resource management - so this is how to find out what can "
                          "be toggled at runtime without knowing about each feature in advance. Filter, do not dump: a process "
                          "registers hundreds. 'default' is only reported when the CVar has been changed away from it, and "
                          "'requiresRestart' marks the ones whose new value the engine will not read until the owning "
                          "subsystem syncs it.";
    desc.m_sInputSchema = R"({"type":"object","properties":{)"
                          R"("contains":{"type":"string","description":"Only return CVars whose name or description contains this, case insensitive."},)"
                          R"("plugin":{"type":"string","description":"Only return CVars declared by this plugin, case insensitive. Use cvar_list without filters first to see which plugin names exist."},)"
                          R"("changedOnly":{"type":"boolean","description":"Only return CVars whose value differs from their default. Cheap way to see what this process was configured with. Default false."})"
                          R"(}})";
  }

  {
    ezMcpToolDesc& desc = out_tools.ExpandAndGetRef();
    desc.m_sName = "cvar_set";
    desc.m_sDescription = "Changes the value of one CVar. The value is converted to the CVar's own type, so a number may be "
                          "sent as a JSON number or as a string. Returns what the value was and what it is now.\n"
                          "If the CVar is flagged 'requiresRestart', the write goes to a pending value and what the engine "
                          "reads does NOT change until the owning subsystem syncs it - the response says so explicitly, "
                          "because re-reading the CVar afterwards would otherwise look as though the write was ignored.";
    desc.m_sInputSchema = R"({"type":"object","properties":{)"
                          R"("name":{"type":"string","description":"The CVar to change. Case insensitive. Use cvar_list to find it."},)"
                          R"("value":{"description":"The new value. Converted to the CVar's type; booleans also accept 'true'/'false' and 0/1."})"
                          R"(},"required":["name","value"]})";
  }
}

void ezMcpCVarTool::Execute(ezStringView sToolName, const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  if (sToolName == "cvar_list")
  {
    ExecuteList(arguments, out_result);
  }
  else if (sToolName == "cvar_set")
  {
    ExecuteSet(arguments, out_result);
  }
}

void ezMcpCVarTool::ExecuteList(const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  const ezStringView sContains = ezMcpJson::GetString(arguments, "contains");
  const ezStringView sPlugin = ezMcpJson::GetString(arguments, "plugin");
  const bool bChangedOnly = ezMcpJson::GetBool(arguments, "changedOnly", false);

  ezMcpJsonWriter writer;
  writer.BeginObject();

  ezUInt32 uiTotalMatches = 0;
  ezUInt32 uiReturned = 0;

  writer.BeginArray("cvars");

  // ezCVar is ezEnumerable, so every CVar of every loaded plugin is on this list without registration
  for (const ezCVar* pCVar = ezCVar::GetFirstInstance(); pCVar != nullptr; pCVar = pCVar->GetNextInstance())
  {
    if (!sPlugin.IsEmpty() && !pCVar->GetPluginName().IsEqual_NoCase(sPlugin))
      continue;

    if (!sContains.IsEmpty())
    {
      const bool bInName = pCVar->GetName().FindSubString_NoCase(sContains) != nullptr;
      const bool bInDesc = pCVar->GetDescription().FindSubString_NoCase(sContains) != nullptr;

      if (!bInName && !bInDesc)
        continue;
    }

    if (bChangedOnly && IsAtDefaultValue(pCVar))
      continue;

    ++uiTotalMatches;

    if (uiReturned >= s_uiMaxResults)
      continue; // keep counting, so that 'totalMatches' reports how much was left out

    WriteCVar(writer, pCVar);
    ++uiReturned;
  }

  writer.EndArray();

  writer.AddVariableUInt32("totalMatches", uiTotalMatches);
  writer.AddVariableUInt32("returned", uiReturned);

  if (uiTotalMatches > uiReturned)
  {
    writer.AddVariableBool("truncated", true);
  }

  writer.EndObject();
  out_result.m_sText = writer.GetResult();
}

void ezMcpCVarTool::ExecuteSet(const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  const ezStringView sName = ezMcpJson::GetString(arguments, "name");

  if (sName.IsEmpty())
  {
    out_result.SetError("No 'name' argument given.");
    return;
  }

  ezCVar* pCVar = ezCVar::FindCVarByName(sName);

  if (pCVar == nullptr)
  {
    ezStringBuilder sError;
    sError.SetFormat("No CVar named '{}' exists in this process. Use cvar_list to find the right name; a CVar only exists once "
                     "the plugin that declares it has been loaded.",
      sName);
    out_result.SetError(sError);
    return;
  }

  const ezVariant* pValue = nullptr;

  if (!arguments.TryGetValue("value", pValue) || !pValue->IsValid())
  {
    out_result.SetError("No 'value' argument given.");
    return;
  }

  // The target's type decides the conversion, not the type the client happened to send: AI clients send
  // numbers as strings and booleans as 0/1, and refusing those would be a type check dressed up as
  // validation. Everything below fails only when the value genuinely cannot be read as that type.
  ezMcpJsonWriter writer;
  writer.BeginObject();
  writer.AddVariableString("name", pCVar->GetName());
  writer.AddVariableString("type", CVarTypeToString(pCVar->GetType()));
  WriteValue(writer, "previousValue", pCVar, ezCVarValue::Current);

  ezStringBuilder sConversionError;

  switch (pCVar->GetType())
  {
    case ezCVarType::Int:
    {
      ezResult res = EZ_FAILURE;
      const ezInt32 iValue = static_cast<ezInt32>(pValue->ConvertTo<ezInt64>(&res));
      if (res.Failed())
        sConversionError.SetFormat("Value '{}' cannot be read as an integer.", pValue->ConvertTo<ezString>());
      else
        *static_cast<ezCVarInt*>(pCVar) = iValue;
      break;
    }
    case ezCVarType::Float:
    {
      ezResult res = EZ_FAILURE;
      const float fValue = pValue->ConvertTo<float>(&res);
      if (res.Failed())
        sConversionError.SetFormat("Value '{}' cannot be read as a float.", pValue->ConvertTo<ezString>());
      else
        *static_cast<ezCVarFloat*>(pCVar) = fValue;
      break;
    }
    case ezCVarType::Bool:
    {
      ezResult res = EZ_FAILURE;
      const bool bValue = pValue->ConvertTo<bool>(&res);
      if (res.Failed())
        sConversionError.SetFormat("Value '{}' cannot be read as a boolean. Use true/false, \"true\"/\"false\" or 1/0.", pValue->ConvertTo<ezString>());
      else
        *static_cast<ezCVarBool*>(pCVar) = bValue;
      break;
    }
    case ezCVarType::String:
    {
      ezResult res = EZ_FAILURE;
      const ezString sValue = pValue->ConvertTo<ezString>(&res);
      if (res.Failed())
        sConversionError = "Value cannot be read as a string.";
      else
        *static_cast<ezCVarString*>(pCVar) = sValue.GetView();
      break;
    }
    default:
      sConversionError = "This CVar has a type that this tool does not know how to write.";
      break;
  }

  if (!sConversionError.IsEmpty())
  {
    // Abandoned rather than finished: the object was opened before the conversion was attempted, and
    // returning from inside it would trip the JSON writer's 'stream was not closed' assert.
    writer.EndAll();
    out_result.SetError(sConversionError);
    return;
  }

  WriteValue(writer, "value", pCVar, ezCVarValue::Current);

  if (pCVar->GetFlags().IsSet(ezCVarFlags::RequiresDelayedSync))
  {
    WriteValue(writer, "pendingValue", pCVar, ezCVarValue::DelayedSync);
    writer.AddVariableBool("requiresRestart", true);
    writer.AddVariableString("note",
      "This CVar only takes effect after the owning subsystem syncs it, usually at startup. 'value' is what the engine still "
      "reads and 'pendingValue' is what was just written - so re-reading this CVar will keep reporting the old value, and that "
      "is not a failed write.");
  }

  writer.EndObject();
  out_result.m_sText = writer.GetResult();
}
