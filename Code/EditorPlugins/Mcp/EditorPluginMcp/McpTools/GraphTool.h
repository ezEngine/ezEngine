#pragma once

#include <Mcp/McpTool.h>

#include <Foundation/Types/UniquePtr.h>

class ezDocument;
class ezDocumentObject;
class ezMcpJsonWriter;
class ezVisualGraphObjectManager;
class ezVisualGraphPin;

/// Edits node graphs: visual scripts, visual shaders, render pipelines, state machines, animation and ProcGen graphs.
///
/// The nodes of a graph are ordinary document objects whose properties ezMcpObjectTool reads and writes. What it cannot
/// do is what only exists in ezVisualGraphObjectManager: the pins of a node, its position in the graph view, and the
/// connections between pins. Connections are document objects as well, but creating or changing them as objects leaves
/// the manager's record of what is connected out of sync, so they are only made and removed through ezNodeCommands here.
///
/// All changes go through the command history, as in ezQtVisualGraphScene, so each call is one undoable step.
class ezMcpGraphTool : public ezMcpToolProvider
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMcpGraphTool, ezMcpToolProvider);

public:
  virtual void GetSupportedTools(ezDynamicArray<ezMcpToolDesc>& out_tools) const override;
  virtual void Execute(ezStringView sToolName, const ezVariantDictionary& arguments, ezMcpToolResult& out_result) override;

private:
  void ExecuteInfo(const ezVariantDictionary& arguments, ezMcpToolResult& out_result);
  void ExecuteNodeTemplates(const ezVariantDictionary& arguments, ezMcpToolResult& out_result);
  void ExecuteAddNode(const ezVariantDictionary& arguments, ezMcpToolResult& out_result);
  void ExecuteMoveNodes(const ezVariantDictionary& arguments, ezMcpToolResult& out_result);
  void ExecuteConnect(const ezVariantDictionary& arguments, ezMcpToolResult& out_result);
  void ExecuteDisconnect(const ezVariantDictionary& arguments, ezMcpToolResult& out_result);

  /// Finds the open document named by the 'document' argument and makes sure it is a node graph.
  ///
  /// Returns nullptr and sets the error on out_result otherwise.
  static ezDocument* FindGraphDocument(const ezVariantDictionary& arguments, const ezVisualGraphObjectManager*& out_pManager, ezMcpToolResult& out_result);

  /// Resolves a node guid and checks that it is a node (not a comment or connection) of the graph.
  static const ezDocumentObject* ResolveNode(const ezDocument* pDocument, const ezVisualGraphObjectManager* pManager, ezStringView sGuid, ezStringView sArgumentName, ezMcpToolResult& out_result);

  /// Writes guid, type, position and, if requested, the pins of a node or comment.
  ///
  /// \param sMemberName Writes the node as a member of that name, or as an array element when empty.
  static void WriteNode(ezMcpJsonWriter& ref_writer, const ezVisualGraphObjectManager* pManager, const ezDocumentObject* pObject, bool bIncludePins, ezStringView sMemberName = {});

  static void WritePins(ezMcpJsonWriter& ref_writer, ezStringView sName, const ezVisualGraphObjectManager* pManager, ezArrayPtr<const ezUniquePtr<const ezVisualGraphPin>> pins);

  static void WriteConnection(ezMcpJsonWriter& ref_writer, const ezVisualGraphObjectManager* pManager, const ezDocumentObject* pConnectionObject, ezStringView sMemberName = {});
};
