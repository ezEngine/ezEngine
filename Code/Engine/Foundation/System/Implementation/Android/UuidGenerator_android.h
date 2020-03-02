#include <Foundation/FoundationInternal.h>
EZ_FOUNDATION_INTERNAL_HEADER

#  include <android_native_app_glue.h>
#  include <Foundation/Basics/Platform/Android/AndroidUtils.h>
#  include <Foundation/Basics/Platform/Android/AndroidJni.h>

void ezUuid::CreateNewUuid()
{
  ezJniAttachment attachment;

  ezJniClass uuidClass("java/util/UUID");
  EZ_ASSERT_DEBUG(!uuidClass.IsNull(), "UUID class not found.");
  ezJniObject javaUuid = uuidClass.CallStatic<ezJniObject>("randomUUID");
  jlong mostSignificant = javaUuid.Call<jlong>("getMostSignificantBits");
  jlong leastSignificant = javaUuid.Call<jlong>("getLeastSignificantBits");

  m_uiHigh = mostSignificant;
  m_uiLow = leastSignificant;

  //#TODO maybe faster to read /proc/sys/kernel/random/uuid, but that can't be done via ezOSFile
  // see https://stackoverflow.com/questions/11888055/include-uuid-h-into-android-ndk-project
}

