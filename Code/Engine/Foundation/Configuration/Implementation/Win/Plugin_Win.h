
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
      sNewPath.AppendFormat("%i", uiFileNumber);

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
          err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &lpMsgBuf, 0, nullptr);

      ezLog::Error("Could not unload plugin '%s'. Error-Code %u (\"%s\")", szPluginFile, err, lpMsgBuf);

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

    Module = LoadLibraryW(ezStringWChar(szFileToLoad).GetData());

    if (Module == nullptr)
    {
      DWORD err = GetLastError();

      LPVOID lpMsgBuf = nullptr;

      FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr,
          err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &lpMsgBuf, 0, nullptr);

      ezLog::Error("Could not load plugin '%s'. Error-Code %u (\"%s\")", szPluginFile, err, lpMsgBuf);

      LocalFree(lpMsgBuf);

      return EZ_FAILURE;
    }

    return EZ_SUCCESS;
  }

#else
  #error "This file should not have been included."
#endif

