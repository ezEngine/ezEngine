#pragma once

#include <Mcp/McpTool.h>

class ezDocument;
class ezDocumentObject;
class ezObjectAccessorBase;
class ezMcpJsonWriter;
class ezAbstractProperty;

/// \brief Reads and modifies the properties of the objects inside a document.
///
/// The unit of addressing is one object plus one property name. An object is named by its guid, or
/// implicitly: every tool here defaults to the document's top level object, which for the asset types
/// deriving from ezSimpleAssetDocument is the single object holding all of the asset's settings.
///
/// Only direct properties are read. A property whose value is another object reports that object's
/// guid instead of its contents, so walking a tree is a sequence of calls rather than one large
/// response - the same listing/detail split the rest of the tools use.
///
/// Modifications go through the document's ezObjectCommandAccessor, so each one is a single undoable
/// transaction. Nothing here saves; that stays an explicit ezMcpDocumentTool call.
class ezMcpObjectTool : public ezMcpToolProvider
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMcpObjectTool, ezMcpToolProvider);

public:
  virtual void GetSupportedTools(ezDynamicArray<ezMcpToolDesc>& out_tools) const override;
  virtual void Execute(ezStringView sToolName, const ezVariantDictionary& arguments, ezMcpToolResult& out_result) override;

private:
  void ExecuteTree(const ezVariantDictionary& arguments, ezMcpToolResult& out_result);
  void ExecuteReadProperties(const ezVariantDictionary& arguments, ezMcpToolResult& out_result);
  void ExecuteModify(const ezVariantDictionary& arguments, ezMcpToolResult& out_result);
  void ExecuteUndo(const ezVariantDictionary& arguments, ezMcpToolResult& out_result);

  /// Deletes one object, wherever it sits in the document.
  ///
  /// This and the two below are split out of ExecuteModify() because they address an object rather than
  /// a property of one: 'property' either does not apply or names a property of a *different* object,
  /// so the lookup ExecuteModify() does before dispatching would be wrong for them.
  void ExecuteDeleteObject(ezDocument* pDocument, const ezDocumentObject* pObject, ezObjectAccessorBase* pAccessor, ezMcpToolResult& out_result);

  /// Moves one object to a different parent, or to a different place under the same one.
  void ExecuteMoveObject(const ezVariantDictionary& arguments, ezDocument* pDocument, const ezDocumentObject* pObject,
    ezObjectAccessorBase* pAccessor, ezMcpToolResult& out_result);

  /// Copies one object, with everything below it, into a parent.
  void ExecuteDuplicateObject(const ezVariantDictionary& arguments, ezDocument* pDocument, const ezDocumentObject* pObject,
    ezMcpToolResult& out_result);

  /// Writes one object of the hierarchy and, while there is depth and budget left, its children.
  ///
  /// \param uiRemainingDepth How many more levels to descend. Zero still writes the object itself, but
  ///        reports its children as a count only.
  /// \param ref_uiBudget Objects left before the response is truncated. Shared across the whole walk,
  ///        so a wide hierarchy runs out at the same total size a deep one does.
  static void WriteTreeNode(ezMcpJsonWriter& ref_writer, const ezDocumentObject* pObject, ezUInt32 uiRemainingDepth, ezUInt32& ref_uiBudget);

  /// Writes one property's name, category, type and current value.
  ///
  /// Containers report their element count and, for maps, their keys, but not their elements - those
  /// are a follow up call with an index. Values that are objects report a guid for the same reason.
  static void WritePropertyValue(ezMcpJsonWriter& ref_writer, ezObjectAccessorBase* pAccessor, const ezDocumentObject* pObject,
    const ezAbstractProperty* pProp, bool bIncludeValues);
};
