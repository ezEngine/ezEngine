#include <Foundation/FoundationPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)

#  define EZ_POSIX_FILE_USEOLDAPI
#  define EZ_POSIX_FILE_USEWINDOWSAPI
#  define EZ_POSIX_FILE_NOINTERNALGETFILESTATS
#  define EZ_POSIX_FILE_NOGETUSERDATAFOLDER
#  define EZ_POSIX_FILE_NOGETTEMPDATAFOLDER
#  define EZ_POSIX_FILE_NOGETUSERDOCUMENTSFOLDER

// For UWP we're currently using a mix of WinRT functions and Posix.
#  include <Foundation/Platform/Posix/OSFile_Posix.inl>

#  include <Foundation/Platform/UWP/Utils/UWPUtils.h>
#  include <windows.storage.h>

ezString ezOSFile::GetUserDataFolder(ezStringView sSubFolder)
{
  if (s_sUserDataPath.IsEmpty())
  {
    ComPtr<ABI::Windows::Storage::IApplicationDataStatics> appDataStatics;
    if (SUCCEEDED(ABI::Windows::Foundation::GetActivationFactory(HStringReference(RuntimeClass_Windows_Storage_ApplicationData).Get(), &appDataStatics)))
    {
      ComPtr<ABI::Windows::Storage::IApplicationData> applicationData;
      if (SUCCEEDED(appDataStatics->get_Current(&applicationData)))
      {
        ComPtr<ABI::Windows::Storage::IStorageFolder> applicationDataLocal;
        if (SUCCEEDED(applicationData->get_LocalFolder(&applicationDataLocal)))
        {
          ComPtr<ABI::Windows::Storage::IStorageItem> localFolderItem;
          if (SUCCEEDED(applicationDataLocal.As(&localFolderItem)))
          {
            HSTRING path;
            localFolderItem->get_Path(&path);
            s_sUserDataPath = ezStringUtf8(path).GetData();
          }
        }
      }
    }
  }

  ezStringBuilder s = s_sUserDataPath;
  s.AppendPath(sSubFolder);
  s.MakeCleanPath();
  return s;
}

ezString ezOSFile::GetTempDataFolder(ezStringView sSubFolder /*= nullptr*/)
{
  ezStringBuilder s;

  if (s_sTempDataPath.IsEmpty())
  {
    ComPtr<ABI::Windows::Storage::IApplicationDataStatics> appDataStatics;
    if (SUCCEEDED(ABI::Windows::Foundation::GetActivationFactory(HStringReference(RuntimeClass_Windows_Storage_ApplicationData).Get(), &appDataStatics)))
    {
      ComPtr<ABI::Windows::Storage::IApplicationData> applicationData;
      if (SUCCEEDED(appDataStatics->get_Current(&applicationData)))
      {
        ComPtr<ABI::Windows::Storage::IStorageFolder> applicationTempData;
        if (SUCCEEDED(applicationData->get_TemporaryFolder(&applicationTempData)))
        {
          ComPtr<ABI::Windows::Storage::IStorageItem> tempFolderItem;
          if (SUCCEEDED(applicationTempData.As(&tempFolderItem)))
          {
            HSTRING path;
            tempFolderItem->get_Path(&path);
            s_sTempDataPath = ezStringUtf8(path).GetData();
          }
        }
      }
    }
  }

  s = s_sTempDataPath;
  s.AppendPath(sSubFolder);
  s.MakeCleanPath();
  return s;
}

ezString ezOSFile::GetUserDocumentsFolder(ezStringView sSubFolder /*= {}*/)
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
