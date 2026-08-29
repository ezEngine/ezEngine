#pragma once

#include <Mcp/McpTool.h>

struct ezActionDescriptor;

/// Tools for listing and triggering editor actions - the things behind menu entries and toolbar buttons.
///
/// Actions of ezActionScope::Global need nothing but their name. Document and window actions act on one
/// document, which the caller names by guid or path in the 'document' argument; that document has to be
/// open, and a window action additionally needs it to have a window. This is how play-the-game is
/// started and stopped from outside the editor.
class ezMcpActionTool : public ezMcpToolProvider
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMcpActionTool, ezMcpToolProvider);

public:
  virtual void GetSupportedTools(ezDynamicArray<ezMcpToolDesc>& out_tools) const override;
  virtual void Execute(ezStringView sToolName, const ezVariantDictionary& arguments, ezMcpToolResult& out_result) override;

private:
  void ExecuteList(const ezVariantDictionary& arguments, ezMcpToolResult& out_result);
  void ExecuteState(const ezVariantDictionary& arguments, ezMcpToolResult& out_result);
  void ExecuteAction(const ezVariantDictionary& arguments, ezMcpToolResult& out_result);

  /// Resolves an action by name, optionally restricted to a category.
  ///
  /// Without a category the name has to be unique across all of them, which is not guaranteed: the same
  /// name may be registered in several categories. out_bAmbiguous reports that case separately from
  /// 'not found', so the caller can ask for a category instead of just failing.
  static const ezActionDescriptor* FindAction(ezStringView sName, ezStringView sCategory, bool& out_bAmbiguous);
};
