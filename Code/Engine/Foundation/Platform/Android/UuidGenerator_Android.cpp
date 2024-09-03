#include <Foundation/FoundationPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_ANDROID)

#  include <Foundation/Types/Uuid.h>

#  include <Foundation/Basics/Platform/Android/AndroidJni.h>
#  include <Foundation/Basics/Platform/Android/AndroidUtils.h>
#  include <android_native_app_glue.h>

ezUuid ezUuid::MakeUuid()
{
  ezJniAttachment attachment;

  ezJniClass uuidClass("java/util/UUID");
  EZ_ASSERT_DEBUG(!uuidClass.IsNull(), "UUID class not found.");
  ezJniObject javaUuid = uuidClass.CallStatic<ezJniObject>("randomUUID");
  jlong mostSignificant = javaUuid.Call<jlong>("getMostSignificantBits");
  jlong leastSignificant = javaUuid.Call<jlong>("getLeastSignificantBits");

  return ezUuid(leastSignificant, mostSignificant);

  // #TODO maybe faster to read /proc/sys/kernel/random/uuid, but that can't be done via ezOSFile
  //  see https://stackoverflow.com/questions/11888055/include-uuid-h-into-android-ndk-project
}

#endif


