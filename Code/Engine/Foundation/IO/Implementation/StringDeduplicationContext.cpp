#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/StringDeduplicationContext.h>

static constexpr ezTypeVersion s_uiStringDeduplicationVersion = 1;

EZ_IMPLEMENT_SERIALIZATION_CONTEXT(ezStringDeduplicationWriteContext)

ezStringDeduplicationWriteContext::ezStringDeduplicationWriteContext(ezStreamWriter& ref_originalStream)
  : ezSerializationContext()
  , m_OriginalStream(ref_originalStream)
{
}

ezStringDeduplicationWriteContext::~ezStringDeduplicationWriteContext() = default;

ezStreamWriter& ezStringDeduplicationWriteContext::Begin()
{
  EZ_ASSERT_DEV(m_TempStreamStorage.GetStorageSize64() == 0, "Begin() can only be called once on a string deduplication context.");

  m_TempStreamWriter.SetStorage(&m_TempStreamStorage);

  return m_TempStreamWriter;
}

ezResult ezStringDeduplicationWriteContext::End()
{
  // We set the context manual to null here since we need normal
  // string serialization to write the de-duplicated map
  SetContext(nullptr);

  m_OriginalStream.WriteVersion(s_uiStringDeduplicationVersion);

  const ezUInt64 uiNumEntries = m_DeduplicatedStrings.GetCount();
  m_OriginalStream << uiNumEntries;

  ezMap<ezUInt32, ezHybridString<64>> StringsSortedByIndex;

  // Build a new map from index to string so we can use a plain
  // array for serialization and lookup purposes
  for (const auto& it : m_DeduplicatedStrings)
  {
    StringsSortedByIndex.Insert(it.Value(), std::move(it.Key()));
  }

  // Write the new map entries, but just the strings since the indices are linear ascending
  for (const auto& it : StringsSortedByIndex)
  {
    m_OriginalStream << it.Value();
  }

  // Now append the original stream
  EZ_SUCCEED_OR_RETURN(m_TempStreamStorage.CopyToStream(m_OriginalStream));

  return EZ_SUCCESS;
}

void ezStringDeduplicationWriteContext::SerializeString(const ezStringView& sString, ezStreamWriter& ref_writer)
{
  bool bAlreadDeduplicated = false;
  auto it = m_DeduplicatedStrings.FindOrAdd(sString, &bAlreadDeduplicated);

  if (!bAlreadDeduplicated)
  {
    it.Value() = m_DeduplicatedStrings.GetCount() - 1;
  }

  ref_writer << it.Value();
}

ezUInt32 ezStringDeduplicationWriteContext::GetUniqueStringCount() const
{
  return m_DeduplicatedStrings.GetCount();
}


EZ_IMPLEMENT_SERIALIZATION_CONTEXT(ezStringDeduplicationReadContext)

ezStringDeduplicationReadContext::ezStringDeduplicationReadContext(ezStreamReader& inout_stream)
  : ezSerializationContext()
{
  // We set the context manually to nullptr to get the original string table
  SetContext(nullptr);

  // Read the string table first
  /*auto version =*/inout_stream.ReadVersion(s_uiStringDeduplicationVersion);

  ezUInt64 uiNumEntries = 0;
  inout_stream >> uiNumEntries;

  m_DeduplicatedStrings.Reserve(static_cast<ezUInt32>(uiNumEntries));

  for (ezUInt64 i = 0; i < uiNumEntries; ++i)
  {
    ezStringBuilder s;
    inout_stream >> s;

    m_DeduplicatedStrings.PushBackUnchecked(std::move(s));
  }

  SetContext(this);
}

ezStringDeduplicationReadContext::~ezStringDeduplicationReadContext() = default;

ezStringView ezStringDeduplicationReadContext::DeserializeString(ezStreamReader& ref_reader)
{
  ezUInt32 uiIndex = ezInvalidIndex;
  ref_reader >> uiIndex;

  if (uiIndex >= m_DeduplicatedStrings.GetCount())
  {
    EZ_ASSERT_DEBUG(uiIndex < m_DeduplicatedStrings.GetCount(), "Failed to read data from file.");
    return {};
  }

  return m_DeduplicatedStrings[uiIndex].GetView();
}


