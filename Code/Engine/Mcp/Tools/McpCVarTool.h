#pragma once

#include <Mcp/McpTool.h>

class ezCVar;
class ezMcpJsonWriter;

/// \brief Reading and writing the CVars this process has registered.
///
/// CVars are where the engine and its plugins already expose their debug switches - render passes,
/// physics visualisation, AI overlays, resource management - so this reaches a large amount of existing
/// behaviour for very little code. Nothing has to be written per switch: a plugin that declares a CVar
/// is reachable the moment it is loaded.
///
/// Host independent, hence concrete and living in the Mcp library. It is most useful in a game process,
/// where the interesting switches are, but the editor registers its own and answers the same way.
class ezMcpCVarTool : public ezMcpToolProvider
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMcpCVarTool, ezMcpToolProvider);

public:
  virtual void GetSupportedTools(ezDynamicArray<ezMcpToolDesc>& out_tools) const override;
  virtual void Execute(ezStringView sToolName, const ezVariantDictionary& arguments, ezMcpToolResult& out_result) override;

private:
  void ExecuteList(const ezVariantDictionary& arguments, ezMcpToolResult& out_result);
  void ExecuteSet(const ezVariantDictionary& arguments, ezMcpToolResult& out_result);

  /// \brief Writes one CVar as an object: name, type, current value, and what differs from it.
  static void WriteCVar(ezMcpJsonWriter& ref_writer, const ezCVar* pCVar);

  /// \brief The 'Current' value of any CVar type, as the JSON type that matches it.
  static void WriteValue(ezMcpJsonWriter& ref_writer, ezStringView sFieldName, const ezCVar* pCVar, ezUInt32 uiWhichValue);

  /// The cap exists for the same reason as everywhere else - a process can register hundreds and an
  /// unfiltered dump would bury the answer.
  static constexpr ezUInt32 s_uiMaxResults = 200;
};
