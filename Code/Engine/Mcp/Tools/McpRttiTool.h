#pragma once

#include <Mcp/McpTool.h>

class ezRTTI;
class ezMcpJsonWriter;
class ezAbstractProperty;
class ezAbstractFunctionProperty;

/// Exposes the reflection data, so an agent can find out which types exist and what they look like.
///
/// Split into four narrow tools rather than one, because the codebase has well over a thousand reflected
/// types: a single 'dump everything' call would cost more tokens than a client can spend and would bury
/// whatever was actually asked for. The intended flow is rtti_find_types to narrow down to a few names,
/// then the detail tools for those.
///
/// Host independent, hence concrete and living in the Mcp library: reflection is the same system in a
/// game as in the editor, and it is how an agent finds out what a component or a game's own types look
/// like. Whatever the host has registered by the time of the call is what gets reported - in the editor
/// that includes the phantom types ezPhantomRttiManager puts into ezRTTI, which the same traversal picks
/// up without having to know about them.
class ezMcpRttiTool : public ezMcpToolProvider
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMcpRttiTool, ezMcpToolProvider);

public:
  virtual void GetSupportedTools(ezDynamicArray<ezMcpToolDesc>& out_tools) const override;
  virtual void Execute(ezStringView sToolName, const ezVariantDictionary& arguments, ezMcpToolResult& out_result) override;

private:
  void ExecuteFindTypes(const ezVariantDictionary& arguments, ezMcpToolResult& out_result);
  void ExecuteTypeInfo(const ezVariantDictionary& arguments, ezMcpToolResult& out_result);
  void ExecuteTypeProperties(const ezVariantDictionary& arguments, ezMcpToolResult& out_result);
  void ExecuteDerivedTypes(const ezVariantDictionary& arguments, ezMcpToolResult& out_result);

  static void WriteTypeFlags(ezMcpJsonWriter& ref_writer, const ezRTTI* pType);
  static void WritePropertyFlags(ezMcpJsonWriter& ref_writer, ezStringView sName, ezBitflags<ezPropertyFlags> flags);
  static void WriteAttributes(ezMcpJsonWriter& ref_writer, ezArrayPtr<const ezPropertyAttribute* const> attributes);

  /// Writes one attribute as an object: its concrete type plus its reflected members.
  static void WriteAttribute(ezMcpJsonWriter& ref_writer, const ezPropertyAttribute* pAttr);

  /// Returns the attribute a variant points at, or nullptr if it does not hold one. Attributes nested
  /// inside another attribute arrive as a TypedPointer and would otherwise lose their contents.
  static const ezPropertyAttribute* GetAttributeFromVariant(const ezVariant& value);
  /// \param pOwnerType The type the property was listed for. Only used to look up its translations, which
  ///        are keyed on the declaring type - ezAbstractProperty does not know which type that is.
  static void WriteProperty(ezMcpJsonWriter& ref_writer, const ezRTTI* pOwnerType, const ezAbstractProperty* pProp);
  static void WriteFunction(ezMcpJsonWriter& ref_writer, const ezAbstractFunctionProperty* pFunc);
};
