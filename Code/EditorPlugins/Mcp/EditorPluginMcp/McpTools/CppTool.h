#pragma once

#include <Mcp/McpTool.h>

class ezCppSettings;

/// \brief The project's C++ plugin: whether it exists, generating it and compiling it.
///
/// Covers what the 'C++ Project' dialog and the Cpp actions in the project menu do, none of which are
/// reachable through action_execute, because they all end in a modal dialog. Everything here goes
/// through the ezCppProject statics, which are dialog free.
class ezMcpCppTool : public ezMcpToolProvider
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMcpCppTool, ezMcpToolProvider);

public:
  virtual void GetSupportedTools(ezDynamicArray<ezMcpToolDesc>& out_tools) const override;
  virtual void Execute(ezStringView sToolName, const ezVariantDictionary& arguments, ezMcpToolResult& out_result) override;

private:
  void ExecuteStatus(const ezVariantDictionary& arguments, ezMcpToolResult& out_result);
  void ExecuteGenerate(const ezVariantDictionary& arguments, ezMcpToolResult& out_result);
  void ExecuteBuild(const ezVariantDictionary& arguments, ezMcpToolResult& out_result);

  /// \brief Common preconditions: a project has to be open and the C++ settings have to load.
  ///
  /// Returns false and fills out_result with the reason when a tool cannot run at all.
  static bool PrepareSettings(ezCppSettings& out_settings, ezMcpToolResult& out_result);

  /// \brief The name the plugin binary is built under, which is not stored anywhere: an empty setting
  /// means the project name is used.
  static ezString GetEffectivePluginName(const ezCppSettings& settings);

  /// \brief Absolute path of the built plugin library, i.e. what the editor loads.
  static ezString GetPluginBinaryPath(const ezCppSettings& settings);
};
