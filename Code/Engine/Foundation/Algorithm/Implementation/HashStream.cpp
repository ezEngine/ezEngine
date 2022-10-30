#include <Foundation/FoundationPCH.h>

#include <Foundation/Algorithm/HashStream.h>

#define XXH_INLINE_ALL
#include <Foundation/ThirdParty/xxHash/xxhash.h>

ezHashStreamWriter32::ezHashStreamWriter32(ezUInt32 seed)
{
  m_pState = XXH32_createState();
  EZ_VERIFY(XXH_OK == XXH32_reset((XXH32_state_t*)m_pState, seed), "");
}

ezHashStreamWriter32::~ezHashStreamWriter32()
{
  XXH32_freeState((XXH32_state_t*)m_pState);
}

ezResult ezHashStreamWriter32::WriteBytes(const void* pWriteBuffer, ezUInt64 uiBytesToWrite)
{
  if (uiBytesToWrite > std::numeric_limits<size_t>::max())
    return EZ_FAILURE;

  if (XXH_OK == XXH32_update((XXH32_state_t*)m_pState, pWriteBuffer, static_cast<size_t>(uiBytesToWrite)))
    return EZ_SUCCESS;

  return EZ_FAILURE;
}

ezUInt32 ezHashStreamWriter32::GetHashValue() const
{
  return XXH32_digest((XXH32_state_t*)m_pState);
}


ezHashStreamWriter64::ezHashStreamWriter64(ezUInt64 seed)
{
  m_pState = XXH64_createState();
  EZ_VERIFY(XXH_OK == XXH64_reset((XXH64_state_t*)m_pState, seed), "");
}

ezHashStreamWriter64::~ezHashStreamWriter64()
{
  XXH64_freeState((XXH64_state_t*)m_pState);
}

ezResult ezHashStreamWriter64::WriteBytes(const void* pWriteBuffer, ezUInt64 uiBytesToWrite)
{
  if (uiBytesToWrite > std::numeric_limits<size_t>::max())
    return EZ_FAILURE;

  if (XXH_OK == XXH64_update((XXH64_state_t*)m_pState, pWriteBuffer, static_cast<size_t>(uiBytesToWrite)))
    return EZ_SUCCESS;

  return EZ_FAILURE;
}

ezUInt64 ezHashStreamWriter64::GetHashValue() const
{
  return XXH64_digest((XXH64_state_t*)m_pState);
}

EZ_STATICLINK_FILE(Foundation, Foundation_Algorithm_Implementation_HashStream);
