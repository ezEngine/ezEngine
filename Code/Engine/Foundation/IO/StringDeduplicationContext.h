
#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/SerializationContext.h>
#include <Foundation/Strings/String.h>

class ezStreamWriter;
class ezStreamReader;

/// \brief This class allows for automatic deduplication of strings written to a stream.
/// To use, create an object of this type on the stack, call Begin() and use the returned
/// ezStreamWriter for subsequent serialization operations. Call End() once you want to finish writing
/// deduplicated strings. For a sample see StreamOperationsTest.cpp
class EZ_FOUNDATION_DLL ezStringDeduplicationWriteContext : public ezSerializationContext<ezStringDeduplicationWriteContext>
{
  EZ_DECLARE_SERIALIZATION_CONTEXT(ezStringDeduplicationWriteContext);

public:
  /// \brief Setup the write context to perform string deduplication.
  ezStringDeduplicationWriteContext(ezStreamWriter& ref_originalStream);
  ~ezStringDeduplicationWriteContext();

  /// \brief Call this method to begin string deduplicaton. You need to use the returned stream writer for subsequent serialization operations until
  /// End() is called.
  ezStreamWriter& Begin();

  /// \brief Ends the string deduplication and writes the string table to the original stream
  ezResult End();

  /// \brief Internal method to serialize a string.
  void SerializeString(const ezStringView& sString, ezStreamWriter& ref_writer);

  /// \brief Returns the number of unique strings which were serialized with this instance.
  ezUInt32 GetUniqueStringCount() const;

  /// \brief Returns the original stream that was passed to the constructor.
  ezStreamWriter& GetOriginalStream() { return m_OriginalStream; }

protected:
  ezStreamWriter& m_OriginalStream;

  ezDefaultMemoryStreamStorage m_TempStreamStorage;
  ezMemoryStreamWriter m_TempStreamWriter;

  ezMap<ezHybridString<64>, ezUInt32> m_DeduplicatedStrings;
};

/// \brief This class to restore strings written to a stream using a ezStringDeduplicationWriteContext.
class EZ_FOUNDATION_DLL ezStringDeduplicationReadContext : public ezSerializationContext<ezStringDeduplicationReadContext>
{
  EZ_DECLARE_SERIALIZATION_CONTEXT(ezStringDeduplicationReadContext);

public:
  /// \brief Setup the string table used internally.
  ezStringDeduplicationReadContext(ezStreamReader& inout_stream);
  ~ezStringDeduplicationReadContext();

  /// \brief Internal method to deserialize a string.
  ezStringView DeserializeString(ezStreamReader& ref_reader);

protected:
  ezDynamicArray<ezHybridString<64>> m_DeduplicatedStrings;
};
