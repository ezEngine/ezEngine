#pragma once

#include <Mcp/McpTool.h>

class ezDocumentObject;
class ezMcpJsonWriter;

/// Reads and sets the selection of an open document.
///
/// The selection is the one piece of editor state that is visible to both sides: it is what a user
/// means by "this object", and setting it is how an agent points at something instead of describing
/// it. It is per document and is not saved.
///
/// Selecting an object does not require its window to exist, but a document that was opened without
/// one shows nothing, so a selection meant for the user to see belongs to a document they have open.
class ezMcpSelectionTool : public ezMcpToolProvider
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMcpSelectionTool, ezMcpToolProvider);

public:
  virtual void GetSupportedTools(ezDynamicArray<ezMcpToolDesc>& out_tools) const override;
  virtual void Execute(ezStringView sToolName, const ezVariantDictionary& arguments, ezMcpToolResult& out_result) override;

private:
  void ExecuteGet(const ezVariantDictionary& arguments, ezMcpToolResult& out_result);
  void ExecuteSet(const ezVariantDictionary& arguments, ezMcpToolResult& out_result);

  /// Writes guid, name and type - enough to recognise an object without a second call per entry.
  static void WriteObject(ezMcpJsonWriter& ref_writer, const ezDocumentObject* pObject);
};
