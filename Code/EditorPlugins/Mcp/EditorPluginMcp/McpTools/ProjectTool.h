#pragma once

#include <Mcp/McpTool.h>

/// \brief Information about the project that the editor currently has open.
///
/// This is what orients an agent that has just connected: every path an other tool returns is
/// relative to the data directories reported here, and asset states depend on the active profile.
class ezMcpProjectTool : public ezMcpToolProvider
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMcpProjectTool, ezMcpToolProvider);

public:
  virtual void GetSupportedTools(ezDynamicArray<ezMcpToolDesc>& out_tools) const override;
  virtual void Execute(ezStringView sToolName, const ezVariantDictionary& arguments, ezMcpToolResult& out_result) override;

private:
  void ExecuteProjectInfo(const ezVariantDictionary& arguments, ezMcpToolResult& out_result);
  void ExecuteProjectExport(const ezVariantDictionary& arguments, ezMcpToolResult& out_result);
};
