#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Reflection/Reflection.h>

static constexpr ezTypeVersion s_uiTypeVersionContextVersion = 1;

EZ_IMPLEMENT_SERIALIZATION_CONTEXT(ezTypeVersionWriteContext)

ezTypeVersionWriteContext::ezTypeVersionWriteContext() = default;
ezTypeVersionWriteContext::~ezTypeVersionWriteContext() = default;

ezStreamWriter& ezTypeVersionWriteContext::Begin(ezStreamWriter& ref_originalStream)
{
  m_pOriginalStream = &ref_originalStream;

  EZ_ASSERT_DEV(m_TempStreamStorage.GetStorageSize64() == 0, "Begin() can only be called once on a type version context.");
  m_TempStreamWriter.SetStorage(&m_TempStreamStorage);

  return m_TempStreamWriter;
}

ezResult ezTypeVersionWriteContext::End()
{
  EZ_ASSERT_DEV(m_pOriginalStream != nullptr, "End() called before Begin()");

  WriteTypeVersions(*m_pOriginalStream);

  // Now append the original stream
  EZ_SUCCEED_OR_RETURN(m_TempStreamStorage.CopyToStream(*m_pOriginalStream));

  return EZ_SUCCESS;
}

void ezTypeVersionWriteContext::AddType(const ezRTTI* pRtti)
{
  if (m_KnownTypes.Insert(pRtti) == false)
  {
    if (const ezRTTI* pParentRtti = pRtti->GetParentType())
    {
      AddType(pParentRtti);
    }
  }
}

void ezTypeVersionWriteContext::WriteTypeVersions(ezStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(s_uiTypeVersionContextVersion);

  const ezUInt32 uiNumTypes = m_KnownTypes.GetCount();
  inout_stream << uiNumTypes;

  ezMap<ezString, const ezRTTI*> sortedTypes;
  for (auto pType : m_KnownTypes)
  {
    sortedTypes.Insert(pType->GetTypeName(), pType);
  }

  for (const auto& it : sortedTypes)
  {
    inout_stream << it.Key();
    inout_stream << it.Value()->GetTypeVersion();
  }
}

//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_SERIALIZATION_CONTEXT(ezTypeVersionReadContext)

ezTypeVersionReadContext::ezTypeVersionReadContext(ezStreamReader& inout_stream)
{
  auto version = inout_stream.ReadVersion(s_uiTypeVersionContextVersion);
  EZ_IGNORE_UNUSED(version);

  ezUInt32 uiNumTypes = 0;
  inout_stream >> uiNumTypes;

  ezStringBuilder sTypeName;
  ezUInt32 uiTypeVersion;

  for (ezUInt32 i = 0; i < uiNumTypes; ++i)
  {
    inout_stream >> sTypeName;
    inout_stream >> uiTypeVersion;

    if (const ezRTTI* pType = ezRTTI::FindTypeByName(sTypeName))
    {
      m_TypeVersions.Insert(pType, uiTypeVersion);
    }
    else
    {
      ezLog::Warning("Ignoring unknown type '{}'", sTypeName);
    }
  }
}

ezTypeVersionReadContext::~ezTypeVersionReadContext() = default;

ezUInt32 ezTypeVersionReadContext::GetTypeVersion(const ezRTTI* pRtti) const
{
  ezUInt32 uiVersion = ezInvalidIndex;
  m_TypeVersions.TryGetValue(pRtti, uiVersion);

  return uiVersion;
}


