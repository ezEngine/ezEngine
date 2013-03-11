#pragma once

#include <Foundation/IO/FileSystem/Implementation/DataDirType.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Containers/HybridArray.h>

class ezDataDirectory_Folder_Reader;
class ezDataDirectory_Folder_Writer;

/// A data directory type to handle access to ordinary files.
/// Register the 'Factory' function at ezFileSystem to allow it to mount local directories.
class EZ_FOUNDATION_DLL ezDataDirectoryType_Folder : public ezDataDirectoryType
{
public:
  ~ezDataDirectoryType_Folder();

  /// The factory that can be registered at ezFileSystem to create data directories of this type.
  static ezDataDirectoryType* Factory(const char* szDataDirectory);

protected:
  // The implementations of the abstract functions.

  virtual ezDataDirectory_Reader* OpenFileToRead(const char* szFile) EZ_OVERRIDE;
  virtual ezDataDirectory_Writer* OpenFileToWrite(const char* szFile) EZ_OVERRIDE;
  virtual void RemoveDataDirectory() EZ_OVERRIDE;
  virtual void DeleteFile(const char* szFile) EZ_OVERRIDE;

private:
  /// Called by 'ezDataDirectoryType_Folder::Factory'
  virtual ezResult InternalInitializeDataDirectory(const char* szDirectory) EZ_OVERRIDE;

  /// Marks the given reader/writer as reusable.
  virtual void OnReaderWriterClose(ezDataDirectory_ReaderWriter_Base* pClosed) EZ_OVERRIDE;

  ezHybridArray<ezDataDirectory_Folder_Reader*, 4> m_Readers;
  ezHybridArray<ezDataDirectory_Folder_Writer*, 4> m_Writers;
};


/// Handles reading from ordinary files.
class EZ_FOUNDATION_DLL ezDataDirectory_Folder_Reader : public ezDataDirectory_Reader
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezDataDirectory_Folder_Reader);

public:
  ezDataDirectory_Folder_Reader() { m_bIsInUse = false; } 
  virtual ezUInt64 Read(void* pBuffer, ezUInt64 uiBytes) EZ_OVERRIDE;

private:
  virtual ezResult InternalOpen() EZ_OVERRIDE;
  virtual void InternalClose() EZ_OVERRIDE;

  friend class ezDataDirectoryType_Folder;

  bool m_bIsInUse;
  ezOSFile m_File;
};

/// Handles writing to ordinary files.
class EZ_FOUNDATION_DLL ezDataDirectory_Folder_Writer : public ezDataDirectory_Writer
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezDataDirectory_Folder_Writer);

public:
  ezDataDirectory_Folder_Writer() { m_bIsInUse = false; }
  virtual ezResult Write(const void* pBuffer, ezUInt64 uiBytes) EZ_OVERRIDE;

private:
  virtual ezResult InternalOpen() EZ_OVERRIDE;
  virtual void InternalClose() EZ_OVERRIDE;

  friend class ezDataDirectoryType_Folder;

  bool m_bIsInUse;
  ezOSFile m_File;
};





