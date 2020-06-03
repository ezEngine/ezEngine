#include <FoundationPCH.h>

#include <Foundation/Algorithm/HashStream.h>

#define XXH_INLINE_ALL
#include <Foundation/ThirdParty/xxHash/xxhash.h>

ezHashStreamWriter32::ezHashStreamWriter32(ezUInt32 seed)
{
  m_state = XXH32_createState();
  EZ_ASSERT_ALWAYS(XXH_OK == XXH32_reset(m_state, seed), "");
}

ezHashStreamWriter32::~ezHashStreamWriter32()
{
  XXH32_freeState(m_state);
}

ezResult ezHashStreamWriter32::WriteBytes(const void* pWriteBuffer, ezUInt64 uiBytesToWrite)
{
  if (uiBytesToWrite > std::numeric_limits<size_t>::max())
    return EZ_FAILURE;

  if (XXH_OK == XXH32_update(m_state, pWriteBuffer, static_cast<size_t>(uiBytesToWrite)))
    return EZ_SUCCESS;

  return EZ_FAILURE;
}

ezUInt32 ezHashStreamWriter32::GetHashValue() const
{
  return XXH32_digest(m_state);
}


ezHashStreamWriter64::ezHashStreamWriter64(ezUInt64 seed)
{
  m_state = XXH64_createState();
  EZ_ASSERT_ALWAYS(XXH_OK == XXH64_reset(m_state, seed), "");
}

ezHashStreamWriter64::~ezHashStreamWriter64()
{
  XXH64_freeState(m_state);
}

ezResult ezHashStreamWriter64::WriteBytes(const void* pWriteBuffer, ezUInt64 uiBytesToWrite)
{
  if (uiBytesToWrite > std::numeric_limits<size_t>::max())
    return EZ_FAILURE;

  if (XXH_OK == XXH64_update(m_state, pWriteBuffer, static_cast<size_t>(uiBytesToWrite)))
    return EZ_SUCCESS;

  return EZ_FAILURE;
}

ezUInt64 ezHashStreamWriter64::GetHashValue() const
{
  return XXH64_digest(m_state);
}

EZ_STATICLINK_FILE(Foundation, Foundation_Algorithm_Implementation_HashStream);
