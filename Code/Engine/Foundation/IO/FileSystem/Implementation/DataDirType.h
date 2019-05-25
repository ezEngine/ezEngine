#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Strings/String.h>

class ezDataDirectoryReaderWriterBase;
class ezDataDirectoryReader;
class ezDataDirectoryWriter;
struct ezFileStats;

/// \brief The base class for all data directory types.
///
/// There are different data directory types, such as a simple folder, a ZIP file or some kind of library
/// (e.g. image files from procedural data). Even a HTTP server that actually transmits files over a network
/// can provided by implementing it as a data directory type.
/// Data directories are added through ezFileSystem, which uses factories to decide which ezDataDirectoryType
/// to use for handling which data directory.
class EZ_FOUNDATION_DLL ezDataDirectoryType
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezDataDirectoryType);

public:
  ezDataDirectoryType() {}
  virtual ~ezDataDirectoryType() {}

  /// \brief Returns the absolute path to the data directory.
  const ezString128& GetDataDirectoryPath() const { return m_sDataDirectoryPath; }

  /// \brief By default this is the same as GetDataDirectoryPath(), but derived implementations may use a different location where they
  /// actually get the files from.
  virtual const ezString128& GetRedirectedDataDirectoryPath() const { return GetDataDirectoryPath(); }

  /// \brief Some data directory types may use external configuration files (e.g. asset lookup tables)
  ///        that may get updated, while the directory is mounted. This function allows each directory type to implement
  ///        reloading and reapplying of configurations, without dismounting and remounting the data directory.
  virtual void ReloadExternalConfigs(){};

protected:
  friend class ezFileSystem;

  /// \brief Tries to setup the data directory. Can fail, if the type is incorrect (e.g. a ZIP file data directory type cannot handle a
  /// simple folder and vice versa)
  ezResult InitializeDataDirectory(const char* szDataDirPath);

  /// \brief Must be implemented to create a ezDataDirectoryReader for accessing the given file. Returns nullptr if the file could not be
  /// opened.
  ///
  /// \param szFile is given as a path relative to the data directory's path.
  /// So unless the data directory path is empty, this will never be an absolute path.
  /// If a rooted path was given, the root name is also removed and only the relative part is passed along.
  /// \param bSpecificallyThisDataDir This is true when the original path specified to open the file through exactly this data directory,
  /// by using a rooted path.
  /// If an absolute path is used, which incidentally matches the prefix of this data directory, bSpecificallyThisDataDir is NOT set to
  /// true, as there might be other data directories that also match.
  virtual ezDataDirectoryReader* OpenFileToRead(const char* szFile, bool bSpecificallyThisDataDir) = 0;

  /// \brief Must be implemented to create a ezDataDirectoryWriter for accessing the given file. Returns nullptr if the file could not be
  /// opened.
  ///
  /// If it always returns nullptr (default) the data directory is read-only (at least through this type).
  virtual ezDataDirectoryWriter* OpenFileToWrite(const char* szFile) { return nullptr; }

  /// \brief This function is called by the filesystem when a data directory is removed.
  ///
  /// It should delete itself using the proper allocator.
  virtual void RemoveDataDirectory() = 0;

  /// \brief If a Data Directory Type supports it, this function will remove the given file from it.
  virtual void DeleteFile(const char* szFile) {}

  /// \brief This function checks whether the given file exists in this data directory.
  ///
  /// The default implementation simply calls ezOSFile::ExistsFile
  /// An optimized implementation might look this information up in some hash-map.
  virtual bool ExistsFile(const char* szFile, bool bOneSpecificDataDir);

  /// \brief Upon success returns the ezFileStats for a file in this data directory.
  virtual ezResult GetFileStats(const char* szFileOrFolder, bool bOneSpecificDataDir, ezFileStats& out_Stats) = 0;

  /// \brief If this data directory knows how to redirect the given path, it should do so and return true.
  /// Called by ezFileSystem::ResolveAssetRedirection
  virtual bool ResolveAssetRedirection(const char* szPathOrAssetGuid, ezStringBuilder& out_sRedirection) { return false; }

protected:
  friend class ezDataDirectoryReaderWriterBase;

  /// \brief This is automatically called whenever a ezDataDirectoryReaderWriterBase that was opened by this type is being closed.
  ///
  /// It allows the ezDataDirectoryType to return the reader/writer to a pool of reusable objects, or to destroy it
  /// using the proper allocator.
  virtual void OnReaderWriterClose(ezDataDirectoryReaderWriterBase* pClosed) {}

  /// \brief This function should only be used by a Factory (which should be a static function in the respective ezDataDirectoryType).
  ///
  /// It is used to initialize the data directory. If this ezDataDirectoryType cannot handle the given type,
  /// it must return EZ_FAILURE and the Factory needs to clean it up properly.
  virtual ezResult InternalInitializeDataDirectory(const char* szDirectory) = 0;

  /// \brief Derived classes can use 'GetDataDirectoryPath' to access this data.
  ezString128 m_sDataDirectoryPath;
};



/// \brief This is the base class for all data directory readers/writers.
///
/// Different data directory types (ZIP file, simple folder, etc.) use different reader/writer types.
class EZ_FOUNDATION_DLL ezDataDirectoryReaderWriterBase
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezDataDirectoryReaderWriterBase);

public:
  /// \brief The derived class should pass along whether it is a reader or writer.
  ezDataDirectoryReaderWriterBase(ezInt32 iDataDirUserData, bool bIsReader);

  virtual ~ezDataDirectoryReaderWriterBase() {}

  /// \brief Used by ezDataDirectoryType's to try to open the given file. They need to pass along their own pointer.
  ezResult Open(const char* szFilePath, ezDataDirectoryType* pOwnerDataDirectory);

  /// \brief Closes this data stream.
  void Close();

  /// \brief Returns the relative path of this file within the owner data directory.
  const ezString128& GetFilePath() const;

  /// \brief Returns the pointer to the data directory, which created this reader/writer.
  ezDataDirectoryType* GetDataDirectory() const;

  /// \brief Returns true if this is a reader stream, false if it is a writer stream.
  bool IsReader() const { return m_bIsReader; }

  /// \brief Returns the current total size of the file.
  virtual ezUInt64 GetFileSize() const = 0;

  ezInt32 GetDataDirUserData() const { return m_iDataDirUserData; }

protected:
  /// \brief This function must be implemented by the derived class.
  virtual ezResult InternalOpen() = 0;

  /// \brief This function must be implemented by the derived class.
  virtual void InternalClose() = 0;

  bool m_bIsReader;
  ezInt32 m_iDataDirUserData = 0;
  ezDataDirectoryType* m_pDataDirectory;
  ezString128 m_sFilePath;
};

/// \brief A base class for readers that handle reading from a (virtual) file inside a data directory.
///
/// Different data directory types (ZIP file, simple folder, etc.) use different reader/writer types.
class EZ_FOUNDATION_DLL ezDataDirectoryReader : public ezDataDirectoryReaderWriterBase
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezDataDirectoryReader);

public:
  ezDataDirectoryReader(ezInt32 iDataDirUserData)
    : ezDataDirectoryReaderWriterBase(iDataDirUserData, true)
  {
  }

  virtual ezUInt64 Read(void* pBuffer, ezUInt64 uiBytes) = 0;
};

/// \brief A base class for writers that handle writing to a (virtual) file inside a data directory.
///
/// Different data directory types (ZIP file, simple folder, etc.) use different reader/writer types.
class EZ_FOUNDATION_DLL ezDataDirectoryWriter : public ezDataDirectoryReaderWriterBase
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezDataDirectoryWriter);

public:
  ezDataDirectoryWriter(ezInt32 iDataDirUserData)
    : ezDataDirectoryReaderWriterBase(iDataDirUserData, false)
  {
  }

  virtual ezResult Write(const void* pBuffer, ezUInt64 uiBytes) = 0;
};


#include <Foundation/IO/FileSystem/Implementation/DataDirType_inl.h>
