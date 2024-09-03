#include <Foundation/FoundationPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)

#  include <Foundation/Types/Uuid.h>

#  include <combaseapi.h>
#  include <rpc.h>

static_assert(sizeof(ezUInt64) * 2 == sizeof(UUID));

ezUuid ezUuid::MakeUuid()
{
  ezUInt64 uiUuidData[2];

  // this works on desktop Windows
  // UuidCreate(reinterpret_cast<UUID*>(uiUuidData));

  // this also works on UWP
  GUID* guid = reinterpret_cast<GUID*>(&uiUuidData[0]);
  HRESULT hr = CoCreateGuid(guid);
  EZ_IGNORE_UNUSED(hr);
  EZ_ASSERT_DEBUG(SUCCEEDED(hr), "CoCreateGuid failed, guid might be invalid!");

  return ezUuid(uiUuidData[1], uiUuidData[0]);
}

#endif


