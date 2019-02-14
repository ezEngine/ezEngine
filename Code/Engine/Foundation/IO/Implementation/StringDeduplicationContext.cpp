
#include <FoundationPCH.h>
#include <Foundation/IO/StringDeduplicationContext.h>

static const ezTypeVersion s_uiStringDeduplicationVersion = 1;

EZ_IMPLEMENT_SERIALIZATION_CONTEXT(ezStringDeduplicationWriteContext)

ezStringDeduplicationWriteContext::ezStringDeduplicationWriteContext(ezStreamWriter& OriginalStream)
  : ezSerializationContext()
  , m_OriginalStream(OriginalStream)
{
}

ezStringDeduplicationWriteContext::~ezStringDeduplicationWriteContext() = default;

ezStreamWriter& ezStringDeduplicationWriteContext::Begin()
{
  EZ_ASSERT_DEV(m_TempStreamStorage.GetStorageSize() == 0, "Begin() can only be called once on a string deduplication context.");

  m_TempStreamWriter = ezMemoryStreamWriter(&m_TempStreamStorage);

  return m_TempStreamWriter;
}

ezResult ezStringDeduplicationWriteContext::End()
{
  // We set the context manual to null here since we need normal
  // string serialization to write the deduplicated map
  SetContext(nullptr);

  m_OriginalStream.WriteVersion(s_uiStringDeduplicationVersion);

  const ezUInt64 uiNumEntries = m_DeduplicatedStrings.GetCount();
  m_OriginalStream << uiNumEntries;

  ezMap<ezUInt32, ezHybridString<64>> StringsSortedByIndex;

  // Build a new map from index to string so we can use a plain
  // array for serialization and lookup purposes
  for(const auto& it : m_DeduplicatedStrings)
  {
    StringsSortedByIndex.Insert(it.Value(), std::move(it.Key()));
  }

  // Write the new map entries, but just the strings since the indices are linear ascending
  for (const auto& it : StringsSortedByIndex)
  {
    m_OriginalStream << it.Value();
  }

  // Now append the original stream
  EZ_SUCCEED_OR_RETURN(m_OriginalStream.WriteBytes(m_TempStreamStorage.GetData(), m_TempStreamStorage.GetStorageSize()));

  return EZ_SUCCESS;
}

void ezStringDeduplicationWriteContext::SerializeString(const ezStringView& String, ezStreamWriter& Writer)
{
  EZ_ASSERT_DEV(&Writer == &m_TempStreamWriter, "The writer paassed to the context needs to be the same as the writer returned by Begin().");

  bool bAlreadDeduplicated = false;
  auto it = m_DeduplicatedStrings.FindOrAdd(String, &bAlreadDeduplicated);

  if(!bAlreadDeduplicated)
  {
    it.Value() = m_DeduplicatedStrings.GetCount() - 1;
  }

  Writer << it.Value();
}

ezUInt32 ezStringDeduplicationWriteContext::GetUniqueStringCount() const
{
  return m_DeduplicatedStrings.GetCount();
}


EZ_IMPLEMENT_SERIALIZATION_CONTEXT(ezStringDeduplicationReadContext)

ezStringDeduplicationReadContext::ezStringDeduplicationReadContext(ezStreamReader& Stream)
  : ezSerializationContext()
{
  // We set the context manually to nullptr to get the original string table
  SetContext(nullptr);

  // Read the string table first
  /*auto version =*/ Stream.ReadVersion(s_uiStringDeduplicationVersion);

  ezUInt64 uiNumEntries = 0;
  Stream >> uiNumEntries;

  for(ezUInt64 i = 0; i < uiNumEntries; ++i)
  {
    ezStringBuilder Builder;
    Stream >> Builder;

    m_DeduplicatedStrings.ExpandAndGetRef() = std::move(Builder);
  }

  SetContext(this);
}

ezStringDeduplicationReadContext::~ezStringDeduplicationReadContext() = default;

ezStringView ezStringDeduplicationReadContext::DeserializeString(ezStreamReader& Reader)
{
  ezUInt32 uiIndex;
  Reader >> uiIndex;

  return m_DeduplicatedStrings[uiIndex].GetView();
}
