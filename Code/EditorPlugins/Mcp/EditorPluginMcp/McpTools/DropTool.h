#pragma once

#include <Mcp/McpTool.h>

/// Places an asset into a document the way dragging it from the asset browser does.
///
/// Runs the editor's own ezDragDropHandler for the asset type, so the result is exactly what a user gets: an object with the matching
/// component for a mesh, particle effect, sound and so on, an object with an ezPrefabReferenceComponent or an editable prefab instance
/// for a prefab, or a material assigned to a mesh. Which handlers exist depends on the loaded plugins.
class ezMcpDropTool : public ezMcpToolProvider
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMcpDropTool, ezMcpToolProvider);

public:
  virtual void GetSupportedTools(ezDynamicArray<ezMcpToolDesc>& out_tools) const override;
  virtual void Execute(ezStringView sToolName, const ezVariantDictionary& arguments, ezMcpToolResult& out_result) override;
};
