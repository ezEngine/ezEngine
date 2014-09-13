
#include <rpc.h>

EZ_CHECK_AT_COMPILETIME(sizeof(ezUInt64) * 2 == sizeof(UUID));

ezUuid ezUuidGenerator::NewUuid()
{
  ezUInt64 uiUuidData[2];

  UuidCreate(reinterpret_cast<UUID*>(uiUuidData));

  return ezUuid(uiUuidData[0], uiUuidData[1]);
}