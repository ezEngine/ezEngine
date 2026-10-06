#include <Mcp/McpPCH.h>

#include <Foundation/IO/JSONReader.h>
#include <Foundation/IO/MemoryStream.h>
#include <Mcp/McpToolRegistry.h>

ezSet<const ezRTTI*> ezMcpToolRegistry::s_KnownTypes;
ezMap<ezString, ezDynamicArray<ezString>> ezMcpToolRegistry::s_ToolArguments;
ezDynamicArray<ezMcpToolProvider*> ezMcpToolRegistry::s_Providers;
ezDynamicArray<ezMcpToolDesc> ezMcpToolRegistry::s_Tools;
ezMap<ezString, ezMcpToolProvider*> ezMcpToolRegistry::s_ToolLookup;
ezMcpExecuteWrapper ezMcpToolRegistry::s_ExecuteWrapper;

namespace
{
  /// Reads the names of the top level properties from a tool's input schema.
  /// Fails if the schema can't be parsed, doesn't list properties, or allows additional ones.
  ezResult ReadArgumentNames(ezStringView sSchema, ezDynamicArray<ezString>& out_names)
  {
    if (sSchema.IsEmpty())
      return EZ_FAILURE;

    ezRawMemoryStreamReader reader(sSchema.GetStartPointer(), sSchema.GetElementCount());

    ezJSONReader json;
    json.SetLogInterface(ezLog::GetThreadLocalLogSystem());
    if (json.Parse(reader).Failed() || json.GetTopLevelElementType() != ezJSONReader::ElementType::Dictionary)
      return EZ_FAILURE;

    const ezVariantDictionary& root = json.GetTopLevelObject();

    if (const ezVariant* pAdditional = root.GetValue("additionalProperties"))
    {
      if (!pAdditional->IsA<bool>() || pAdditional->Get<bool>())
        return EZ_FAILURE;
    }

    const ezVariant* pProperties = root.GetValue("properties");
    if (pProperties == nullptr || !pProperties->IsA<ezVariantDictionary>())
      return EZ_FAILURE;

    for (auto it : pProperties->Get<ezVariantDictionary>())
    {
      out_names.PushBack(it.Key());
    }

    return EZ_SUCCESS;
  }
} // namespace

void ezMcpToolRegistry::UpdateProviders()
{
  ezRTTI::ForEachDerivedType<ezMcpToolProvider>(
    [](const ezRTTI* pRtti)
    {
      // remember the type even if it can't be allocated, so we don't look at it again
      if (s_KnownTypes.Contains(pRtti))
        return;

      s_KnownTypes.Insert(pRtti);

      // an abstract provider base declares the tools that several hosts share; only the host's concrete
      // subclass is instantiated, so the tool names cannot collide with themselves
      if (!pRtti->GetAllocator()->CanAllocate())
        return;

      ezMcpToolProvider* pProvider = pRtti->GetAllocator()->Allocate<ezMcpToolProvider>();
      s_Providers.PushBack(pProvider);

      pProvider->OnActivate();

      ezDynamicArray<ezMcpToolDesc> tools;
      pProvider->GetSupportedTools(tools);

      for (const ezMcpToolDesc& tool : tools)
      {
        if (s_ToolLookup.Contains(tool.m_sName))
        {
          // two providers claiming the same name would make dispatch ambiguous, and the client would
          // see a duplicate entry in its tool list
          ezLog::Error("MCP: Tool name '{}' is already in use, the one from '{}' is ignored.", tool.m_sName, pRtti->GetTypeName());
          continue;
        }

        s_ToolLookup[tool.m_sName] = pProvider;
        s_Tools.PushBack(tool);

        ezDynamicArray<ezString> argumentNames;
        if (ReadArgumentNames(tool.m_sInputSchema, argumentNames).Succeeded())
        {
          s_ToolArguments[tool.m_sName] = std::move(argumentNames);
        }
      }
    },
    ezRTTI::ForEachOptions::ExcludeNotConcrete);
}

void ezMcpToolRegistry::RemoveProvider(const ezRTTI* pProviderType)
{
  for (ezUInt32 i = s_Providers.GetCount(); i > 0; --i)
  {
    ezMcpToolProvider* pProvider = s_Providers[i - 1];

    // GetDynamicRTTI() reads the vtable, so this must run before the module is actually unmapped
    const ezRTTI* pRtti = pProvider->GetDynamicRTTI();
    if (pRtti != pProviderType)
      continue;

    for (ezUInt32 uiTool = s_Tools.GetCount(); uiTool > 0; --uiTool)
    {
      const ezString& sToolName = s_Tools[uiTool - 1].m_sName;

      if (s_ToolLookup.GetValueOrDefault(sToolName, nullptr) == pProvider)
      {
        s_ToolLookup.Remove(sToolName);
        s_ToolArguments.Remove(sToolName);
        s_Tools.RemoveAtAndCopy(uiTool - 1);
      }
    }

    pProvider->OnDeactivate();
    pRtti->GetAllocator()->Deallocate(pProvider);

    s_Providers.RemoveAtAndCopy(i - 1);
    s_KnownTypes.Remove(pRtti);
  }
}

void ezMcpToolRegistry::Clear()
{
  for (ezMcpToolProvider* pProvider : s_Providers)
  {
    pProvider->OnDeactivate();
    pProvider->GetDynamicRTTI()->GetAllocator()->Deallocate(pProvider);
  }

  s_Providers.Clear();
  s_Tools.Clear();
  s_ToolLookup.Clear();
  s_ToolArguments.Clear();
  s_KnownTypes.Clear();
}

ezResult ezMcpToolRegistry::Execute(ezStringView sToolName, const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  auto it = s_ToolLookup.Find(sToolName);

  if (!it.IsValid())
    return EZ_FAILURE;

  ezMcpToolProvider* pProvider = it.Value();

  if (const ezDynamicArray<ezString>* pKnownArguments = nullptr; s_ToolArguments.TryGetValue(sToolName, pKnownArguments))
  {
    ezStringBuilder sUnknown;
    for (auto itArg : arguments)
    {
      if (!pKnownArguments->Contains(itArg.Key()))
      {
        sUnknown.AppendWithSeparator(", ", "'", itArg.Key(), "'");
      }
    }

    if (!sUnknown.IsEmpty())
    {
      ezStringBuilder sKnown;
      for (const ezString& sArg : *pKnownArguments)
      {
        sKnown.AppendWithSeparator(", ", "'", sArg, "'");
      }

      if (sKnown.IsEmpty())
        sKnown = "none";

      ezStringBuilder sError;
      sError.SetFormat("Unknown argument {} for tool '{}'. It takes: {}. Nothing was done.", sUnknown, sToolName, sKnown);
      out_result.SetError(sError);
      return EZ_SUCCESS;
    }
  }

  ezDelegate<void()> execute = [&]()
  {
    pProvider->Execute(sToolName, arguments, out_result);
  };

  if (s_ExecuteWrapper.IsValid())
  {
    s_ExecuteWrapper(sToolName, out_result, execute);
  }
  else
  {
    execute();
  }

  return EZ_SUCCESS;
}
