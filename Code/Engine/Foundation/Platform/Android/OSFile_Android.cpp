#include <Foundation/FoundationPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_ANDROID)
#  include <Foundation/Platform/Android/Utils/AndroidJni.h>
#  include <Foundation/Platform/Android/Utils/AndroidUtils.h>
#  include <android_native_app_glue.h>

#  define EZ_POSIX_FILE_NOGETAPPLICATIONPATH
#  define EZ_POSIX_FILE_NOGETUSERDATAFOLDER
#  define EZ_POSIX_FILE_NOGETTEMPDATAFOLDER
#  define EZ_POSIX_FILE_NOGETUSERDOCUMENTSFOLDER

#  include <Foundation/Platform/Posix/OSFile_Posix.inl>

ezStringView ezOSFile::GetApplicationPath()
{
  if (s_sApplicationPath.IsEmpty())
  {
    ezJniAttachment attachment;

    ezJniString packagePath = attachment.GetActivity().Call<ezJniString>("getPackageCodePath");
    // By convention, android requires assets to be placed in the 'Assets' folder
    // inside the apk thus we use that as our SDK root.
    ezStringBuilder sTemp = packagePath.GetData();
    sTemp.AppendPath("Assets/ezDummyBin");
    s_sApplicationPath = sTemp;
  }

  return s_sApplicationPath;
}

ezString ezOSFile::GetUserDataFolder(ezStringView sSubFolder)
{
  if (s_sUserDataPath.IsEmpty())
  {
    android_app* app = ezAndroidUtils::GetAndroidApp();
    s_sUserDataPath = app->activity->internalDataPath;
  }

  ezStringBuilder s = s_sUserDataPath;
  s.AppendPath(sSubFolder);
  s.MakeCleanPath();
  return s;
}

ezString ezOSFile::GetTempDataFolder(ezStringView sSubFolder)
{
  if (s_sTempDataPath.IsEmpty())
  {
    ezJniAttachment attachment;

    ezJniObject cacheDir = attachment.GetActivity().Call<ezJniObject>("getCacheDir");
    ezJniString path = cacheDir.Call<ezJniString>("getPath");
    s_sTempDataPath = path.GetData();
  }

  ezStringBuilder s = s_sTempDataPath;
  s.AppendPath(sSubFolder);
  s.MakeCleanPath();
  return s;
}

ezString ezOSFile::GetUserDocumentsFolder(ezStringView sSubFolder)
{
  if (s_sUserDocumentsPath.IsEmpty())
  {
    EZ_ASSERT_NOT_IMPLEMENTED;
  }

  ezStringBuilder s = s_sUserDocumentsPath;
  s.AppendPath(sSubFolder);
  s.MakeCleanPath();
  return s;
}

#endif
