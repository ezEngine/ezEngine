
#include <uuid/uuid.h>

EZ_CHECK_AT_COMPILETIME(sizeof(ezUInt64) * 2 == sizeof(uuid_t));

void ezUuid::CreateNewUuid()
{
  uuid_t uuid;
  uuid_generate(uuid);

  ezUInt64* uiUuidData = reinterpret_cast<ezUInt64*>(uuid);

  m_uiHigh = uiUuidData[0];
  m_uiLow  = uiUuidData[1];
}