
#include <rpc.h>

EZ_CHECK_AT_COMPILETIME(sizeof(ezUInt64) * 2 == sizeof(UUID));

void ezUuid::CreateNewUuid()
{
  ezUInt64 uiUuidData[2];

  UuidCreate(reinterpret_cast<UUID*>(uiUuidData));

  m_uiHigh = uiUuidData[0];
  m_uiLow  = uiUuidData[1];
}