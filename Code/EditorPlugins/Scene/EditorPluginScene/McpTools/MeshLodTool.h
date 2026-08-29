#pragma once

#include <Mcp/McpTool.h>

/// Creates the LOD mesh assets that sit next to a mesh asset, which is otherwise only reachable
/// through the asset browser's context menu and its dialog.
///
/// Split the same way as the prefab tools: 'mesh_lod_info' reports what a mesh would get and what it
/// already has, 'mesh_lod_create' performs it. Creating with the defaults needs no info call.
///
/// The LODs are what makes 'mesh_prefab_create' build an ezLodMeshComponent, so this runs before it.
class ezMcpMeshLodTool : public ezMcpToolProvider
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMcpMeshLodTool, ezMcpToolProvider);

public:
  virtual void GetSupportedTools(ezDynamicArray<ezMcpToolDesc>& out_tools) const override;
  virtual void Execute(ezStringView sToolName, const ezVariantDictionary& arguments, ezMcpToolResult& out_result) override;

private:
  void ExecuteInfo(const ezVariantDictionary& arguments, ezMcpToolResult& out_result);
  void ExecuteCreate(const ezVariantDictionary& arguments, ezMcpToolResult& out_result);
};
