
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)

  #include <Foundation/Configuration/Plugin.h>
  #include <Foundation/Strings/StringBuilder.h>
  #include <Foundation/IO/OSFile.h>
  #include <Foundation/Logging/Log.h>

  typedef HMODULE ezPluginModule;

  void ezPlugin::GetPluginPaths(const char* szPluginName, ezStringBuilder& sOldPath, ezStringBuilder& sNewPath, ezUInt8 uiFileNumber)
  {
    sOldPath = ezOSFile::GetApplicationDirectory();
    sOldPath.AppendPath(szPluginName);
    sOldPath.Append(".dll");

    sNewPath = ezOSFile::GetApplicationDirectory();
    sNewPath.AppendPath(szPluginName);

    if (uiFileNumber > 0)
      sNewPath.AppendFormat("{0}", uiFileNumber);

    sNewPath.Append(".loaded");
  }

  ezResult UnloadPluginModule(ezPluginModule& Module, const char* szPluginFile)
  {
    // reset last error code
    if (GetLastError()) { }

    if (FreeLibrary(Module) == FALSE)
    {
      DWORD err = GetLastError();

      LPVOID lpMsgBuf = nullptr;

      FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr,
          err, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), (LPTSTR) &lpMsgBuf, 0, nullptr);

      ezLog::Error("Could not unload plugin '{0}'. Error-Code {1} (\"{2}\")", szPluginFile, (ezUInt32)err, (const char*)lpMsgBuf);

      LocalFree(lpMsgBuf);

      return EZ_FAILURE;
    }

    Module = nullptr;
    return EZ_SUCCESS;
  }

  ezResult LoadPluginModule(const char* szFileToLoad, ezPluginModule& Module, const char* szPluginFile)
  {
    // reset last error code
    if (GetLastError()) { }

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
    ezStringBuilder relativePath = szFileToLoad;
    relativePath.MakeRelativeTo(ezOSFile::GetApplicationDirectory());
    Module = LoadPackagedLibrary(ezStringWChar(relativePath).GetData(), 0);
#else
    Module = LoadLibraryW(ezStringWChar(szFileToLoad).GetData());
#endif

    if (Module == nullptr)
    {
      DWORD err = GetLastError();

      LPVOID lpMsgBuf = nullptr;

      FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr,
          err, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), (LPTSTR) &lpMsgBuf, 0, nullptr);

      if (ezUnicodeUtils::IsValidUtf8((const char*) lpMsgBuf)) // happens on localized systems
        ezLog::Error("Could not load plugin '{0}'. Error-Code {1} / 0x{2} (\"{3}\")", szPluginFile, ezArgU(err, 8, true, 16, true), (ezUInt32)err, (const char*)lpMsgBuf);
      else
        ezLog::Error("Could not load plugin '{0}'. Error-Code {1} / 0x{2}", szPluginFile, ezArgU(err, 8, true, 16, true), (ezUInt32)err);

      if (err == 126)
      {
        ezLog::Error("Please Note: This means that the plugin exists, but a DLL dependency of the plugin is missing. You probably need to copy 3rd party DLLs next to the plugin.");
      }

      LocalFree(lpMsgBuf);

      return EZ_FAILURE;
    }

    return EZ_SUCCESS;
  }

#else
  #error "This file should not have been included."
#endif

