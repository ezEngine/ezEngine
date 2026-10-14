#pragma once

#include <Mcp/McpTool.h>

class ezMcpJsonWriter;
class ezScene2Document;

/// Lists and manages the layers of an open scene.
///
/// A layer is a separate document ('.ezSceneLayer') whose objects are part of the scene. Only the active layer can be edited, and
/// while a layer is active, the scene document's objects are that layer's. The generic object tools therefore refuse a scene or layer
/// that is not the active one, and this tool is how an agent switches.
class ezMcpSceneLayerTool : public ezMcpToolProvider
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMcpSceneLayerTool, ezMcpToolProvider);

public:
  virtual void GetSupportedTools(ezDynamicArray<ezMcpToolDesc>& out_tools) const override;
  virtual void Execute(ezStringView sToolName, const ezVariantDictionary& arguments, ezMcpToolResult& out_result) override;

private:
  static void WriteLayers(ezMcpJsonWriter& ref_writer, ezScene2Document* pScene);
};
