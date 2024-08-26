#include <Foundation/FoundationInternal.h>
EZ_FOUNDATION_INTERNAL_HEADER

#include <Foundation/Types/Uuid.h>

#if __has_include(<uuid/uuid.h>)
#  include <uuid/uuid.h>
#  define HAS_UUID 1
#else
// #  error "uuid.h does not exist on this distro."
#  define HAS_UUID 0
#endif

#if HAS_UUID

static_assert(sizeof(ezUInt64) * 2 == sizeof(uuid_t));

ezUuid ezUuid::MakeUuid()
{
  uuid_t uuid;
  uuid_generate(uuid);

  ezUInt64* uiUuidData = reinterpret_cast<ezUInt64*>(uuid);

  return ezUuid(uiUuidData[1], uiUuidData[0]);
}

#else

ezUuid ezUuid::MakeUuid()
{
  EZ_REPORT_FAILURE("This distro doesn't have support for UUID generation.");
  return ezUuid();
}

#endif
