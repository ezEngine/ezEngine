#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Types/UniquePtr.h>

// A general problem when implementing a directory watcher is, that moving a folder out of the watched directory only communicates
// Which folder was moved (but not to where, nor its contents). This means when a folder is moved out of view,
// this needs to be treated as a delete. At the point of the move, it is no longer possible to query the contents of the folder.
// So a in memory copy of the file system is required in order to correctly implement a directory watcher.
class ezFileSystemMirror
{
public:
  enum class Type
  {
    File,
    Directory
  };

  struct DirEntry
  {
    ezMap<ezString, DirEntry> m_subDirectories;
    ezDynamicArray<ezString> m_files;
  };

  ezFileSystemMirror();
  ~ezFileSystemMirror();

  // \brief Adds the directory, and all files in it recursively.
  void AddDirectory(const char* path);

  // \brief Adds a file. Creates directories if they do not exist.
  void AddFile(const char* path);

  // \brief Removes a file.
  void RemoveFile(const char* path);

  // \brief Removes a directory. Deletes any files & directories inside.
  void RemoveDirectory(const char* path);

  using EnumerateFunc = ezDelegate<void(const ezStringBuilder& path, Type type)>;

  // \brief Enumerates the files & directories under the given path
  void Enumerate(const char* path, EnumerateFunc callbackFunc);

private:
  DirEntry m_topLevelDir;
  ezString m_topLevelDirPath;
};

namespace 
{
  template <typename T, typename U>
  uint32_t FindInsertPosition(const T& container, const U& value)
  {
    auto* begin = container.GetData();
    auto* end = container.GetData() + container.GetCount();
    auto insertPos = std::upper_bound(begin, end, value);
    return insertPos - begin;
  }

  template <typename T, typename U>
  uint32_t FindInsertPositionLikeyOrdered(const T& container, const U& value)
  {
    if(container.IsEmpty())
    {
      return 0;
    }
    if(container.PeekBack() < value)
    {
      return container.GetCount();
    }
    return FindInsertPosition(container, value);
  }
}

ezFileSystemMirror::ezFileSystemMirror() = default;
ezFileSystemMirror::~ezFileSystemMirror() = default;

void ezFileSystemMirror::AddDirectory(const char* path)
{
  ezStringBuilder currentDirAbsPath = path;
  currentDirAbsPath.MakeCleanPath();
  if(currentDirAbsPath.GetElementCount() > 1 && currentDirAbsPath.EndsWith("/"))
  {
    currentDirAbsPath.Shrink(0, 1);
  }

  if (m_topLevelDirPath.IsEmpty())
  {
    m_topLevelDirPath = currentDirAbsPath;

    ezDynamicArray<DirEntry*> m_dirStack;
    DirEntry* currentDir = &m_topLevelDir;

    ezFileSystemIterator files;
    files.StartSearch(currentDirAbsPath.GetData(), ezFileSystemIteratorFlags::ReportFilesAndFoldersRecursive);
    for (; files.IsValid(); files.Next())
    {
      const ezFileStats& stats = files.GetStats();
      ezLog::Info("{} {}", stats.m_sParentPath, stats.m_sName);

      // In case we are done with a directory, move back up
      while (currentDirAbsPath != stats.m_sParentPath)
      {
        EZ_ASSERT_DEV(m_dirStack.GetCount() > 0, "Unexpected file iteration order");
        currentDir = m_dirStack.PeekBack();
        m_dirStack.PopBack();
        currentDirAbsPath.PathParentDirectory();
        if(currentDirAbsPath.GetElementCount() > 1 && currentDirAbsPath.EndsWith("/"))
        {
            currentDirAbsPath.Shrink(0, 1);
        }
      }

      if (stats.m_bIsDirectory)
      {
        m_dirStack.PushBack(currentDir);
        auto insertIt = currentDir->m_subDirectories.Insert(stats.m_sName, DirEntry());
        currentDir = &insertIt.Value();
        currentDirAbsPath.AppendPath(stats.m_sName);
      }
      else
      {
        uint32_t insertPos = FindInsertPositionLikeyOrdered(currentDir->m_files, stats.m_sName);
        currentDir->m_files.Insert(std::move(stats.m_sName), insertPos);
      }
    }
  }
}

void ezFileSystemMirror::AddFile(const char* path)
{
}

void ezFileSystemMirror::RemoveFile(const char* path)
{
}

void ezFileSystemMirror::RemoveDirectory(const char* path)
{
}

void ezFileSystemMirror::Enumerate(const char* path, EnumerateFunc callbackFunc)
{
}