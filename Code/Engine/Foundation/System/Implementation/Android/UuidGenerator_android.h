#include <Foundation/FoundationInternal.h>
EZ_FOUNDATION_INTERNAL_HEADER

void ezUuid::CreateNewUuid()
{
  // TODO read /sys/kernel/random/uuid as uuid.h is not available
  // see https://stackoverflow.com/questions/11888055/include-uuid-h-into-android-ndk-project

  m_uiHigh = 0;
  m_uiLow = 0;
  EZ_ASSERT_NOT_IMPLEMENTED;
}

