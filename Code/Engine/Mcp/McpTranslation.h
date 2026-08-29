#pragma once

#include <Mcp/McpDLL.h>

#include <Foundation/Strings/String.h>

class ezMcpJsonWriter;
class ezRTTI;

/// Translation lookups that report a missing entry as an empty string.
///
/// The ezTranslate* macros return the key unchanged when nothing is registered for it, so a tool using
/// them directly reports every action without a tooltip as having its own name as one, and every type
/// without documentation as having a help URL that is really just the type name. Comparing the result
/// against the key is the only way to tell the two apart, and doing it at each call site is how that
/// check gets forgotten.
///
/// These matter because the editor's UI shows translated text: the asset browser lists
/// ezTranslate(typeName), so what a user calls 'the Decal asset' is not the name any tool wants. The
/// help URL is prose about what a type is for, which reflection data never conveys.
namespace ezMcpTranslation
{
  /// The display name for sKey, or an empty view when there is no entry.
  ///
  /// Also empty when the result is only sKey made readable - the editor registers a translator that
  /// splits CamelCase names for keys nobody translated, and reporting 'Projection Axis' for
  /// 'ProjectionAxis' costs tokens without adding anything.
  EZ_MCP_DLL ezStringView GetDisplayName(ezStringView sKey);

  /// The tooltip for sKey, or an empty view when there is no entry.
  EZ_MCP_DLL ezStringView GetTooltip(ezStringView sKey);

  /// The documentation URL for sKey, or an empty view when there is no entry.
  EZ_MCP_DLL ezStringView GetHelpURL(ezStringView sKey);

  /// The display name for a reflected property, or an empty view when there is no entry.
  ///
  /// Property translations are keyed '<TypeName>::<PropertyName>' and registered on the type that
  /// declares the property, so the lookup walks up from pType until it finds an entry. That matters for
  /// an inherited property, whose key names a base type the caller never asked about.
  EZ_MCP_DLL ezStringView GetPropertyDisplayName(const ezRTTI* pType, ezStringView sPropertyName);

  /// The tooltip for a reflected property, or an empty view when there is no entry.
  ///
  /// This is the one place where prose about what a property *does* exists - reflection only ever gives
  /// its name and type. Keyed as described for GetPropertyDisplayName().
  EZ_MCP_DLL ezStringView GetPropertyTooltip(const ezRTTI* pType, ezStringView sPropertyName);

  /// Writes sFieldName only if sValue is not empty, so an absent translation costs no tokens.
  ///
  /// The field name is passed in because the same lookup appears under different names depending on
  /// whose translation it is - an asset's own help URL versus that of its type, for instance.
  EZ_MCP_DLL void AddOptionalString(ezMcpJsonWriter& ref_writer, ezStringView sFieldName, ezStringView sValue);
} // namespace ezMcpTranslation
