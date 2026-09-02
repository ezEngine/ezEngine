#pragma once

#include <Foundation/Time/Time.h>
#include <Foundation/Types/Uuid.h>
#include <Mcp/McpTool.h>

/// Tools for listing and running long ops - the operations behind the "Long Ops" panel.
///
/// A long op is registered automatically for every component in a scene that has an ezLongOpAttribute,
/// for instance baking a scene or placing reflection probes. They run in the engine process, so unlike
/// an editor action they are asynchronous: longop_execute therefore waits for the operation to finish
/// before it answers, which needs the host to keep pumping in between.
class ezMcpLongOpTool : public ezMcpToolProvider
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMcpLongOpTool, ezMcpToolProvider);

public:
  virtual void GetSupportedTools(ezDynamicArray<ezMcpToolDesc>& out_tools) const override;
  virtual void Execute(ezStringView sToolName, const ezVariantDictionary& arguments, ezMcpToolResult& out_result) override;

private:
  void ExecuteList(const ezVariantDictionary& arguments, ezMcpToolResult& out_result);
  void ExecuteRun(const ezVariantDictionary& arguments, ezMcpToolResult& out_result);

  /// Works out which operation the arguments name, either directly by guid or by document + component type.
  ///
  /// Writes the reason into out_result and fails when the arguments don't identify exactly one operation.
  /// The result is only a guid, so it stays valid to call repeatedly for the same (re-entered) request.
  static ezResult ResolveOperation(class ezLongOpControllerManager& ref_manager, const ezVariantDictionary& arguments, ezUuid& out_opGuid, ezMcpToolResult& out_result);

  /// Guid of the operation that ExecuteRun() is currently waiting for, invalid while nothing is running.
  ezUuid m_WaitingForOp;

  /// When the wait started, so that a long op that never finishes doesn't block the caller forever.
  ezTime m_WaitStarted;
};
