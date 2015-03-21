#pragma once

#include <Foundation/Basics.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Time/Timestamp.h>

/// \brief This class represents a set of files of which one wants to know when any one of them changes.
///
/// ezDependencyFile stores a list of files that are the 'dependency set'. It can be serialized.
/// Through HasAnyFileChanged() one can detect whether any of the files has changed, since the last call to StoreCurrentTimeStamp().
/// The time stamp that is retrieved through StoreCurrentTimeStamp() will also be serialized.
class EZ_FOUNDATION_DLL ezDependencyFile
{
public:
  ezDependencyFile();

  /// \brief Clears all files that were added with AddFileDependency()
  void Clear();

  /// \brief Adds one file as a dependency to the list
  void AddFileDependency(const char* szFile);

  /// \brief Allows read access to all currently stored file dependencies
  const ezHybridArray<ezString, 16>& GetFileDependencies() const { return m_FileDependencies; }

  /// \brief Writes the current state to a stream. Note that you probably should call StoreCurrentTimeStamp() before this, to serialize the latest file stamp
  ezResult WriteDependencyFile(ezStreamWriterBase& stream) const;

  /// \brief Reads the state from a stream. Call HasAnyFileChanged() afterwards to determine whether anything has changed since when the data was serialized.
  ezResult ReadDependencyFile(ezStreamReaderBase& stream);

  /// \brief Writes the current state to a file. Note that you probably should call StoreCurrentTimeStamp() before this, to serialize the latest file stamp
  ezResult WriteDependencyFile(const char* szFile) const;

  /// \brief Reads the state from a file. Call HasAnyFileChanged() afterwards to determine whether anything has changed since when the data was serialized.
  ezResult ReadDependencyFile(const char* szFile);

  /// \brief Retrieves the current file time stamps from the filesystem and determines whether any file has changed since the last call to StoreCurrentTimeStamp() (or ReadDependencyFile())
  bool HasAnyFileChanged();

  /// \brief Retrieves the current file time stamps from the filesystem and stores it for later comparison. This value is also serialized through WriteDependencyFile(), so it should be called before that, to store the latest state.
  void StoreCurrentTimeStamp();

private:
  ezResult RetrieveFileTimeStamp(const char* szFile, ezTimestamp& out_Result);

  ezHybridArray<ezString, 16> m_FileDependencies;
  ezInt64 m_iMaxTimeStampStored;

  struct FileCheckCache
  {
    ezTimestamp m_FileTimestamp;
    ezTime m_LastCheck;
  };

  static ezMap<ezString, FileCheckCache> s_FileTimestamps;
};

