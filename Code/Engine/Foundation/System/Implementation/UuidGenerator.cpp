#include <Foundation/FoundationPCH.h>

#include <Foundation/Types/Uuid.h>

ezUuid ezUuid::MakeStableUuidFromString(ezStringView sString)
{
  ezUuid NewUuid;
  NewUuid.m_uiLow = ezHashingUtils::xxHash64String(sString);
  NewUuid.m_uiHigh = ezHashingUtils::xxHash64String(sString, 0x7FFFFFFFFFFFFFE7u);

  return NewUuid;
}

ezUuid ezUuid::MakeStableUuidFromInt(ezInt64 iInt)
{
  ezUuid NewUuid;
  NewUuid.m_uiLow = ezHashingUtils::xxHash64(&iInt, sizeof(ezInt64));
  NewUuid.m_uiHigh = ezHashingUtils::xxHash64(&iInt, sizeof(ezInt64), 0x7FFFFFFFFFFFFFE7u);

  return NewUuid;
}
