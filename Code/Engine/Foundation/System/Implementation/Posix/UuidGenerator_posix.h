#include <Foundation/FoundationInternal.h>
EZ_FOUNDATION_INTERNAL_HEADER

#include <uuid/uuid.h>


EZ_CHECK_AT_COMPILETIME(sizeof(ezUInt64) * 2 == sizeof(uuid_t));

ezUuid ezUuid::MakeUuid()
{
  uuid_t uuid;
  uuid_generate(uuid);

  ezUInt64* uiUuidData = reinterpret_cast<ezUInt64*>(uuid);

  return ezUuid(uiUuidData[1], uiUuidData[0]);
}
