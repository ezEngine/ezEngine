#pragma once

#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/Implementation/DataDirType.h>
#include <Foundation/IO/Stream.h>

/// The base class for all file readers.
/// Provides access to ezFileSystem::GetFileReader, which is necessary to get access to the streams that
/// ezDataDirectoryType's provide.
/// Derive from this class if you want to implement different policies on how to read files.
/// E.g. the default reader (ezFileReader) implements a buffered read policy (using an internal cache).
class EZ_FOUNDATION_DLL ezFileReaderBase : public ezStreamReader
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezFileReaderBase);

public:
  ezFileReaderBase() { m_pDataDirReader = nullptr; }

  /// Returns the absolute path with which the file was opened (including the prefix of the data directory).
  ezString128 GetFilePathAbsolute() const
  {
    ezStringBuilder sAbs = m_pDataDirReader->GetDataDirectory()->GetRedirectedDataDirectoryPath();
    sAbs.AppendPath(m_pDataDirReader->GetFilePath().GetView());
    return sAbs;
  }

  /// Returns the relative path of the file within its data directory (excluding the prefix of the data directory).
  ezString128 GetFilePathRelative() const { return m_pDataDirReader->GetFilePath(); }

  /// Returns the ezDataDirectoryType over which this file has been opened.
  ezDataDirectoryType* GetDataDirectory() const { return m_pDataDirReader->GetDataDirectory(); }

  /// Returns true, if the file is currently open.
  bool IsOpen() const { return m_pDataDirReader != nullptr; }

  /// \brief Returns the current total size of the file.
  ezUInt64 GetFileSize() const { return m_pDataDirReader->GetFileSize(); }

protected:
  ezDataDirectoryReader* GetFileReader(ezStringView sFile, ezFileShareMode::Enum FileShareMode, bool bAllowFileEvents)
  {
    return ezFileSystem::GetFileReader(sFile, FileShareMode, bAllowFileEvents);
  }

  ezDataDirectoryReader* m_pDataDirReader;
};


/// The base class for all file writers.
/// Provides access to ezFileSystem::GetFileWriter, which is necessary to get access to the streams that
/// ezDataDirectoryType's provide.
/// Derive from this class if you want to implement different policies on how to write files.
/// E.g. the default writer (ezFileWriter) implements a buffered write policy (using an internal cache).
class EZ_FOUNDATION_DLL ezFileWriterBase : public ezStreamWriter
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezFileWriterBase);

public:
  ezFileWriterBase() { m_pDataDirWriter = nullptr; }

  /// Returns the absolute path with which the file was opened (including the prefix of the data directory).
  ezString128 GetFilePathAbsolute() const
  {
    ezStringBuilder sAbs = m_pDataDirWriter->GetDataDirectory()->GetRedirectedDataDirectoryPath();
    sAbs.AppendPath(m_pDataDirWriter->GetFilePath().GetView());
    return sAbs;
  }

  /// Returns the relative path of the file within its data directory (excluding the prefix of the data directory).
  ezString128 GetFilePathRelative() const { return m_pDataDirWriter->GetFilePath(); }

  /// Returns the ezDataDirectoryType over which this file has been opened.
  ezDataDirectoryType* GetDataDirectory() const { return m_pDataDirWriter->GetDataDirectory(); }

  /// Returns true, if the file is currently open.
  bool IsOpen() const { return m_pDataDirWriter != nullptr; }

  /// \brief Returns the current total size of the file.
  ezUInt64 GetFileSize() const { return m_pDataDirWriter->GetFileSize(); } // [tested]

protected:
  ezDataDirectoryWriter* GetFileWriter(ezStringView sFile, ezFileShareMode::Enum FileShareMode, bool bAllowFileEvents)
  {
    return ezFileSystem::GetFileWriter(sFile, FileShareMode, bAllowFileEvents);
  }

  ezDataDirectoryWriter* m_pDataDirWriter;
};
