#include <Mcp/McpPCH.h>

#include <Mcp/McpToolRegistry.h>

ezSet<const ezRTTI*> ezMcpToolRegistry::s_KnownTypes;
ezDynamicArray<ezMcpToolProvider*> ezMcpToolRegistry::s_Providers;
ezDynamicArray<ezMcpToolDesc> ezMcpToolRegistry::s_Tools;
ezMap<ezString, ezMcpToolProvider*> ezMcpToolRegistry::s_ToolLookup;
ezMcpExecuteWrapper ezMcpToolRegistry::s_ExecuteWrapper;

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
      }
    },
    ezRTTI::ForEachOptions::ExcludeNotConcrete);
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
  s_KnownTypes.Clear();
}

ezResult ezMcpToolRegistry::Execute(ezStringView sToolName, const ezVariantDictionary& arguments, ezMcpToolResult& out_result)
{
  auto it = s_ToolLookup.Find(sToolName);

  if (!it.IsValid())
    return EZ_FAILURE;

  ezMcpToolProvider* pProvider = it.Value();

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
