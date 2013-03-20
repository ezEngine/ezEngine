#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Strings/String.h>

class ezDataDirectory_ReaderWriter_Base;
class ezDataDirectory_Reader;
class ezDataDirectory_Writer;

/// \brief The base class for all data directory types.
///
/// There are different data directory types, such as a simple folder, a ZIP file or some kind of library 
/// (e.g. image files from procedural data). Even a http server that actually transmits files over a network
/// can provided by implementing it as a data directory type.
/// Data directories are added through ezFileSystem, which uses factories to decide which ezDataDirectoryType
/// to use for handling which data directory.
class EZ_FOUNDATION_DLL ezDataDirectoryType
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezDataDirectoryType);

public:
  ezDataDirectoryType() { }
  virtual ~ezDataDirectoryType() { }

  /// \brief Returns the absolute path to the data directory.
  const ezString128& GetDataDirectoryPath() const { return m_sDataDirectoryPath; }

protected:
  friend class ezFileSystem;

  /// \brief Tries to setup the data directory. Can fail, if the type is incorrect (e.g. a ZIP file data directory type cannot handle a simple folder and vice versa)
  ezResult InitializeDataDirectory(const char* szDataDirPath);

  /// \brief Must be implemented to create a ezDataDirectory_Reader for accessing the given file. Returns NULL if the file could not be opened.
  virtual ezDataDirectory_Reader* OpenFileToRead(const char* szFile) = 0;

  /// \brief Must be implemented to create a ezDataDirectory_Writer for accessing the given file. Returns NULL if the file could not be opened. 
  ///
  /// If it always returns NULL (default) the data directory is read-only (at least through this type).
  virtual ezDataDirectory_Writer* OpenFileToWrite(const char* szFile) { return NULL; }

  /// \brief This function is called by the filesystem when a data directory is removed.
  ///
  /// It should delete itself using the proper allocator.
  virtual void RemoveDataDirectory() = 0;

  /// \brief If a Data Directory Type supports it, this function will remove the given file from it.
  virtual void DeleteFile(const char* szFile) { }

  /// \brief This function checks whether the given file exists in this data directory.
  ///
  /// The default implementation will simply try to open the file for reading.
  /// An optimized implementation might look this information up in some hash-map.
  virtual bool ExistsFile(const char* szFile);

private:
  friend class ezDataDirectory_ReaderWriter_Base;

  /// \brief This is automatically called whenever a ezDataDirectory_ReaderWriter_Base that was opened by this type is being closed.
  ///
  /// It allows the ezDataDirectoryType to return the reader/writer to a pool of reusable objects, or to destroy it
  /// using the proper allocator.
  virtual void OnReaderWriterClose(ezDataDirectory_ReaderWriter_Base* pClosed) { }

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
class EZ_FOUNDATION_DLL ezDataDirectory_ReaderWriter_Base
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezDataDirectory_ReaderWriter_Base);

public:
  /// \brief The derived class should pass along whether it is a reader or writer.
  ezDataDirectory_ReaderWriter_Base(bool bIsReader);

  virtual ~ezDataDirectory_ReaderWriter_Base() { }

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

private:
  /// \brief This function must be implemented by the derived class.
  virtual ezResult InternalOpen() = 0;

  /// \brief This function must be implemented by the derived class.
  virtual void InternalClose() = 0;

  bool m_bIsReader;
  ezDataDirectoryType* m_pDataDirectory;
  ezString128 m_sFilePath;
};

/// \brief A base class for readers that handle reading from a (virtual) file inside a data directory.
///
/// Different data directory types (ZIP file, simple folder, etc.) use different reader/writer types.
class EZ_FOUNDATION_DLL ezDataDirectory_Reader : public ezDataDirectory_ReaderWriter_Base
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezDataDirectory_Reader);

public:
  ezDataDirectory_Reader() : ezDataDirectory_ReaderWriter_Base(true) { }

  virtual ezUInt64 Read(void* pBuffer, ezUInt64 uiBytes) = 0;
};

/// \brief A base class for writers that handle writing to a (virtual) file inside a data directory.
///
/// Different data directory types (ZIP file, simple folder, etc.) use different reader/writer types.
class EZ_FOUNDATION_DLL ezDataDirectory_Writer : public ezDataDirectory_ReaderWriter_Base
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezDataDirectory_Writer);

public:
  ezDataDirectory_Writer() : ezDataDirectory_ReaderWriter_Base(false) { }

  virtual ezResult Write(const void* pBuffer, ezUInt64 uiBytes) = 0;
};


#include <Foundation/IO/FileSystem/Implementation/DataDirType_inl.h>