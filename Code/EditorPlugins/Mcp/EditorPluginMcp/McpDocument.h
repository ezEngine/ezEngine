#pragma once

#include <EditorPluginMcp/EditorPluginMcpDLL.h>

#include <Foundation/Strings/StringBuilder.h>

class ezDocument;
class ezDocumentObject;
struct ezMcpToolResult;

/// \brief Resolving the 'document' and 'object' arguments that most tools take.
///
/// Every tool addresses a document by guid or path and holds no session state, so this lookup runs on
/// each call. Documents are never opened implicitly: a document opened behind the agent's back stays
/// open invisibly, and the undo stack lives on the document, so opening and closing per call would
/// make undo meaningless.
namespace ezMcpDocument
{
  /// \brief Finds an open document by guid, by absolute path, or by a trailing part of its path.
  ///
  /// A guid is the reliable identifier - a path can be spelled several ways and a document that was
  /// renamed no longer answers to its old one. Returns nullptr when nothing matches, which for a path
  /// usually means the document is not open rather than that it does not exist.
  EZ_EDITORPLUGINMCP_DLL ezDocument* Find(ezStringView sIdentifier);

  /// \brief Reports a failed Find() in a way that tells the agent what to do about it.
  EZ_EDITORPLUGINMCP_DLL void SetNotOpenError(ezMcpToolResult& out_result, ezStringView sIdentifier);

  /// \brief The object named by a guid, or the document's top level object when sObjectGuid is empty.
  ///
  /// The top level object is the root's single child. That is what ezSimpleAssetDocument's
  /// GetPropertyObject() returns, reproduced here through the generic ezDocumentObject interface,
  /// because GetPropertyObject() is declared on the template and cannot be called without knowing the
  /// asset type. A document with several top level objects - a scene - has no single answer and fails
  /// with a message saying so.
  ///
  /// Returns nullptr and fills out_sError, which is written for the agent to read.
  EZ_EDITORPLUGINMCP_DLL const ezDocumentObject* ResolveObject(const ezDocument* pDocument, ezStringView sObjectGuid, ezStringBuilder& out_sError);

  /// \brief The name a user would recognise the object by, or an empty string when it has none.
  ///
  /// A game object's name is an ordinary reflected property that most object types do not have, so this
  /// checks the type before reading. Returned by value: the variant it comes out of is a temporary.
  EZ_EDITORPLUGINMCP_DLL ezString GetObjectName(const ezDocumentObject* pObject);
} // namespace ezMcpDocument
