#pragma once

#include <Mcp/McpDLL.h>

#include <Foundation/IO/JSONWriter.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Strings/StringBuilder.h>

/// \brief Writes the JSON that tool results and protocol responses are made of.
///
///  - The output goes into an ezStringBuilder that the writer owns, so tools never deal with streams.
///  - No whitespace is emitted, because every character is a token the client pays for.
///  - WriteVariant() covers what a language model reading arbitrary editor data runs into: variants
///    holding reflected pointers or objects, which the base class refuses.
class EZ_MCP_DLL ezMcpJsonWriter : public ezStandardJSONWriter
{
public:
  ezMcpJsonWriter();
  ~ezMcpJsonWriter();

  /// \brief The JSON written so far.
  ///
  /// Only meaningful once every Begin* has been matched by its End*, because the closing brace is
  /// written by the latter.
  ezStringView GetResult();

  /// Adds the variant types that the base class asserts on: pointers and embedded objects, which turn
  /// up when reflection data is written generically. The base class' assert is fine for a caller that
  /// knows what it writes, but a tool reading arbitrary reflection data cannot know, and an assert in
  /// a tool call takes the whole editor down.
  virtual void WriteVariant(const ezVariant& value) override;

private:
  /// The written bytes are collected here and only turned into a string by GetResult(), because the
  /// writer produces a byte stream and has no notion of a terminating zero.
  ezContiguousMemoryStreamStorage m_Storage;
  ezMemoryStreamWriter m_Writer;
  ezStringBuilder m_sResult;
};
