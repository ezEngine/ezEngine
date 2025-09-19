#pragma once

/// \file

#include <Foundation/Basics.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Types/UniquePtr.h>

class ezOpenDdlReaderElement;

/// \brief Represents a named block of serialized data within a DDL document.
///
/// DDL documents can contain multiple named blocks (Header, Objects, Types) each containing
/// an object graph. This structure holds one such block with its name and deserialized content.
struct EZ_FOUNDATION_DLL ezSerializedBlock
{
  ezString m_Name;                            ///< Name of the block (e.g., "Header", "Objects", "Types")
  ezUniquePtr<ezAbstractObjectGraph> m_Graph; ///< Deserialized object graph for this block
};

/// \brief Low-level DDL serializer for ezAbstractObjectGraph instances.
///
/// This class provides efficient DDL (Data Definition Language) serialization of abstract object graphs.
/// DDL is a human-readable text format that supports comments, structured data, and type information.
class EZ_FOUNDATION_DLL ezAbstractGraphDdlSerializer
{
public:
  /// \brief Writes an object graph to a DDL stream with optional type information.
  ///
  /// \param pGraph The main object graph to serialize
  /// \param pTypesGraph Optional type information for versioning support
  /// \param bCompactMmode If true, minimizes whitespace for smaller files
  /// \param typeMode Controls verbosity of type names in output
  static void Write(ezStreamWriter& inout_stream, const ezAbstractObjectGraph* pGraph, const ezAbstractObjectGraph* pTypesGraph = nullptr, bool bCompactMmode = true, ezOpenDdlWriter::TypeStringMode typeMode = ezOpenDdlWriter::TypeStringMode::Shortest);

  /// \brief Reads an object graph from a DDL stream with optional patching.
  ///
  /// \param pGraph Output graph to populate with deserialized data
  /// \param pTypesGraph Optional type information output for version tracking
  /// \param bApplyPatches If true, applies version patches during deserialization
  static ezResult Read(ezStreamReader& inout_stream, ezAbstractObjectGraph* pGraph, ezAbstractObjectGraph* pTypesGraph = nullptr, bool bApplyPatches = true);

  static void Write(ezOpenDdlWriter& inout_stream, const ezAbstractObjectGraph* pGraph, const ezAbstractObjectGraph* pTypesGraph = nullptr);
  static ezResult Read(const ezOpenDdlReaderElement* pRootElement, ezAbstractObjectGraph* pGraph, ezAbstractObjectGraph* pTypesGraph = nullptr, bool bApplyPatches = true);

  /// \brief Writes a complete document with separate header, objects, and types sections.
  ///
  /// This creates a structured DDL document with named blocks for different types of data.
  /// Commonly used for asset files that need metadata (header), main content (objects),
  /// and type information (types) in separate, clearly organized sections.
  static void WriteDocument(ezStreamWriter& inout_stream, const ezAbstractObjectGraph* pHeader, const ezAbstractObjectGraph* pGraph, const ezAbstractObjectGraph* pTypes, bool bCompactMode = true, ezOpenDdlWriter::TypeStringMode typeMode = ezOpenDdlWriter::TypeStringMode::Shortest);

  /// \brief Reads a complete document and separates header, objects, and types into distinct graphs.
  static ezResult ReadDocument(ezStreamReader& inout_stream, ezUniquePtr<ezAbstractObjectGraph>& ref_pHeader, ezUniquePtr<ezAbstractObjectGraph>& ref_pGraph, ezUniquePtr<ezAbstractObjectGraph>& ref_pTypes, bool bApplyPatches = true);

  /// \brief Reads only the header section from a document without processing the full content.
  ///
  /// This is useful for quickly extracting metadata or file information without the overhead
  /// of deserializing the entire document. Commonly used for asset browsers and file inspection.
  static ezResult ReadHeader(ezStreamReader& inout_stream, ezAbstractObjectGraph* pGraph);

private:
  static ezResult ReadBlocks(ezStreamReader& stream, ezHybridArray<ezSerializedBlock, 3>& blocks);
};
