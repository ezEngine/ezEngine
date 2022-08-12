#pragma once

#include <Foundation/Containers/HashSet.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/SerializationContext.h>

class ezStreamWriter;
class ezStreamReader;

/// \brief This class allows for writing type versions to a stream in a centralized place so that
/// each object doesn't need to write its own version manually.
///
/// To use, create an object of this type on the stack, call Begin() and use the returned
/// ezStreamWriter for subsequent serialization operations. Call AddType to add a type and its parent types to the version table.
/// Call End() once you want to finish writing the type versions.
class EZ_FOUNDATION_DLL ezTypeVersionWriteContext : public ezSerializationContext<ezTypeVersionWriteContext>
{
  EZ_DECLARE_SERIALIZATION_CONTEXT(ezTypeVersionWriteContext);

public:
  ezTypeVersionWriteContext();
  ~ezTypeVersionWriteContext();

  /// \brief Call this method to begin collecting type version info. You need to use the returned stream writer for subsequent serialization operations until
  /// End() is called.
  ezStreamWriter& Begin(ezStreamWriter& originalStream);

  /// \brief Ends the type version collection and writes the data to the original stream.
  ezResult End();

  /// \brief Adds the given type and its parent types to the version table.
  void AddType(const ezRTTI* pRtti);

  /// \brief Manually write the version table to the given stream.
  /// Can be used instead of Begin()/End() if all necessary types are available in one place anyways.
  void WriteTypeVersions(ezStreamWriter& stream) const;

  /// \brief Returns the original stream that was passed to Begin().
  ezStreamWriter& GetOriginalStream() { return *m_pOriginalStream; }

protected:
  ezStreamWriter* m_pOriginalStream = nullptr;

  ezDefaultMemoryStreamStorage m_TempStreamStorage;
  ezMemoryStreamWriter m_TempStreamWriter;

  ezHashSet<const ezRTTI*> m_KnownTypes;
};

/// \brief Use this class to restore type versions written to a stream using a ezTypeVersionWriteContext.
class EZ_FOUNDATION_DLL ezTypeVersionReadContext : public ezSerializationContext<ezTypeVersionReadContext>
{
  EZ_DECLARE_SERIALIZATION_CONTEXT(ezTypeVersionReadContext);

public:
  /// \brief Setup the type info used internally.
  ezTypeVersionReadContext(ezStreamReader& stream);
  ~ezTypeVersionReadContext();

  ezUInt32 GetTypeVersion(const ezRTTI* pRtti) const;

protected:
  ezHashTable<const ezRTTI*, ezUInt32> m_TypeVersions;
};
