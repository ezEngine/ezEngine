#pragma once

#include <Foundation/IO/FileSystem/Implementation/DataDirType.h>

class EZ_FOUNDATION_DLL ezDataDirectoryReaderWeb : public ezDataDirectoryReader
{
public:
  ezDataDirectoryReaderWeb();

  virtual ezUInt64 Read(void* pBuffer, ezUInt64 uiBytes) override;
  virtual ezUInt64 GetFileSize() const override;

protected:
  friend class ezDataDirectoryTypeWeb;

  virtual ezResult InternalOpen(ezFileShareMode::Enum FileShareMode) override;

  bool m_bIsInUse = false;

  void InternalClose() override;

  ezUInt64 m_uiReadPosition = 0;
  ezDynamicArray<ezUInt8> m_Data;
};

/// \brief A data directory type to handle access to files that are served by a webserver.
///
/// Currently this only works in Emscripten builds (EZ_PLATFORM_WEB), though it could be implemented for other platforms as well.
///
/// This data directory type is used when ezFileSystem::AddDataDirectory() is called with a path starting with "web:".
/// For example ezFileSystem::AddDataDirectory("web:Images", ...)  would mean that a file read to "cat.jpg" would be attempted
/// to be fetched from the server using the relative path "Images/cat.jpg".
/// The server that is asked, is the same from which the WASM binary was loaded.
///
/// This data directory type only supports reading files.
///
/// Note that fetching files is very costly and you should generally reduce the amount of file access that you do through this system.
/// For important files that are always needed, consider just packaging them with the application.
///
/// Also be aware that the browser caches these files, so a second read will typically not do another server fetch and instead
/// return the cached file. This also happens across application runs, so you may get a cached file that is days old, even though
/// it may have changed on the server.
/// For development builds, when you test with a server on your local own machine, you can enable ezDataDirectoryTypeWeb::IgnoreClientCache to prevent that.
///
/// For a shipped game, the way to solve this is currently work-in-progress.
class EZ_FOUNDATION_DLL ezDataDirectoryTypeWeb : public ezDataDirectoryType
{
public:
  /// \brief The factory that can be registered at ezFileSystem to create data directories of this type.
  static ezDataDirectoryType* Factory(ezStringView sDataDirectory, ezStringView sGroup, ezStringView sRootName, ezDataDirUsage usage);

  /// \brief [internal] Makes sure the redirection config files are up to date and then reloads them.
  virtual void ReloadExternalConfigs() override;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT) || EZ_DOCS
  /// \brief This can be enabled in debug builds to not read any file from the browser cache and always fetch from the server.
  /// Only do this, when testing with a local server (on your own machine).
  static bool IgnoreClientCache /*= false*/;
#endif

  static ezString s_sRedirectionFile;
  static ezString s_sRedirectionPrefix;

protected:
  void LoadRedirectionFile();

  virtual ezDataDirectoryReader* OpenFileToRead(ezStringView sFile, ezFileShareMode::Enum FileShareMode, bool bSpecificallyThisDataDir) override;
  virtual bool ResolveAssetRedirection(ezStringView sPathOrAssetGuid, ezStringBuilder& out_sRedirection) override;
  virtual ezResult InternalInitializeDataDirectory(ezStringView sDirectory) override;
  virtual bool ExistsFile(ezStringView sFile, bool bOneSpecificDataDir) override;
  /// \brief Limitation: This doesn't handle folders, only files. If someone stats a folder, this will fail.
  virtual ezResult GetFileStats(ezStringView sFileOrFolder, bool bOneSpecificDataDir, ezFileStats& out_Stats) override;
  virtual void OnReaderWriterClose(ezDataDirectoryReaderWriterBase* pClosed) override;

  virtual void RemoveDataDirectory() override;

  mutable ezMutex m_ReaderWriterMutex; ///< Locks m_Readers / m_Writers as well as the m_bIsInUse flag of each reader / writer.
  ezHybridArray<ezDataDirectoryReaderWeb*, 4> m_Readers;

  mutable ezMutex m_RedirectionMutex;
  ezMap<ezString, ezString> m_FileRedirection;
};
