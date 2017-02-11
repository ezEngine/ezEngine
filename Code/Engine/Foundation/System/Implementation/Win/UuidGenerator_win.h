
#include <rpc.h>

EZ_CHECK_AT_COMPILETIME(sizeof(ezUInt64) * 2 == sizeof(UUID));

void ezUuid::CreateNewUuid()
{
  ezUInt64 uiUuidData[2];

  // this works on desktop Windows
  //UuidCreate(reinterpret_cast<UUID*>(uiUuidData));

  // this also works on UWP
  GUID* guid = reinterpret_cast<GUID*>(&uiUuidData[0]);
  CoCreateGuid(guid);

  m_uiHigh = uiUuidData[0];
  m_uiLow  = uiUuidData[1];
}

