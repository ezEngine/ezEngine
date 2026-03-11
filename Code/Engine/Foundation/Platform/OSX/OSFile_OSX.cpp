#include <Foundation/FoundationPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_OSX)
#  include <CoreFoundation/CoreFoundation.h>

#  define EZ_POSIX_FILE_NOGETAPPLICATIONPATH

#  include <Foundation/Platform/Posix/OSFile_Posix.inl>

ezStringView ezOSFile::GetApplicationPath()
{
  if (s_sApplicationPath.IsEmpty())
  {
    CFBundleRef appBundle = CFBundleGetMainBundle();
    CFURLRef bundleURL = CFBundleCopyBundleURL(appBundle);
    CFStringRef bundlePath = CFURLCopyFileSystemPath(bundleURL, kCFURLPOSIXPathStyle);

    if (bundlePath != nullptr)
    {
      CFIndex length = CFStringGetLength(bundlePath);
      CFIndex maxSize = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8) + 1;

      ezArrayPtr<char> temp = EZ_DEFAULT_NEW_ARRAY(char, static_cast<ezUInt32>(maxSize));

      if (CFStringGetCString(bundlePath, temp.GetPtr(), maxSize, kCFStringEncodingUTF8))
      {
        s_sApplicationPath = temp.GetPtr();
      }

      EZ_DEFAULT_DELETE_ARRAY(temp);
    }

    CFRelease(bundlePath);
    CFRelease(bundleURL);
    CFRelease(appBundle);
  }

  return s_sApplicationPath;
}

#endif
