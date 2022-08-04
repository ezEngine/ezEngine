#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Types/UniquePtr.h>

// A general problem when implementing a directory watcher is, that moving a folder out of the watched directory only communicates
// Which folder was moved (but not to where, nor its contents). This means when a folder is moved out of view,
// this needs to be treated as a delete. At the point of the move, it is no longer possible to query the contents of the folder.
// So a in memory copy of the file system is required in order to correctly implement a directory watcher.
template <typename T>
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
    ezMap<ezString, T> m_files;
  };

  ezFileSystemMirror();
  ~ezFileSystemMirror();

  // \brief Adds the directory, and all files in it recursively.
  ezResult AddDirectory(const char* path, bool* outDirectoryExistsAlready = nullptr);

  // \brief Adds a file. Creates directories if they do not exist.
  ezResult AddFile(const char* path, const T& value, bool* outFileExistsAlready, T* outOldValue);

  // \brief Removes a file.
  ezResult RemoveFile(const char* path);

  // \brief Removes a directory. Deletes any files & directories inside.
  ezResult RemoveDirectory(const char* path);

  // \brief Moves a directory. Any files & folders inside are moved with it.
  ezResult MoveDirectory(const char* fromPath, const char* toPath);

  using EnumerateFunc = ezDelegate<void(const ezStringBuilder& path, Type type)>;

  // \brief Enumerates the files & directories under the given path
  ezResult Enumerate(const char* path, EnumerateFunc callbackFunc);

private:
  DirEntry* FindDirectory(ezStringBuilder& path);
  DirEntry* AddDirectoryImpl(DirEntry* startDir, ezStringBuilder& path);

private:
  DirEntry m_topLevelDir;
  ezString m_topLevelDirPath;
};

namespace
{
  void EnsureTrailingSlash(ezStringBuilder& builder)
  {
    if (!builder.EndsWith("/"))
    {
      builder.Append("/");
    }
  }

  void RemoveTrailingSlash(ezStringBuilder& builder)
  {
    if (builder.EndsWith("/"))
    {
      builder.Shrink(0, 1);
    }
  }
} // namespace

template <typename T>
ezFileSystemMirror<T>::ezFileSystemMirror() = default;

template <typename T>
ezFileSystemMirror<T>::~ezFileSystemMirror() = default;

template <typename T>
ezResult ezFileSystemMirror<T>::AddDirectory(const char* path, bool* outDirectoryExistsAlready)
{
  ezStringBuilder currentDirAbsPath = path;
  currentDirAbsPath.MakeCleanPath();
  EnsureTrailingSlash(currentDirAbsPath);

  if (m_topLevelDirPath.IsEmpty())
  {
    m_topLevelDirPath = currentDirAbsPath;
    currentDirAbsPath.Shrink(0, 1); // remove trailing /

    DirEntry* currentDir = &m_topLevelDir;

    ezDynamicArray<DirEntry*> m_dirStack;

    ezFileSystemIterator files;
    files.StartSearch(currentDirAbsPath.GetData(), ezFileSystemIteratorFlags::ReportFilesAndFoldersRecursive);
    for (; files.IsValid(); files.Next())
    {
      const ezFileStats& stats = files.GetStats();

      // In case we are done with a directory, move back up
      while (currentDirAbsPath != stats.m_sParentPath)
      {
        EZ_ASSERT_DEV(m_dirStack.GetCount() > 0, "Unexpected file iteration order");
        currentDir = m_dirStack.PeekBack();
        m_dirStack.PopBack();
        currentDirAbsPath.PathParentDirectory();
        RemoveTrailingSlash(currentDirAbsPath);
      }

      if (stats.m_bIsDirectory)
      {
        m_dirStack.PushBack(currentDir);
        ezStringBuilder subdirName = stats.m_sName;
        EnsureTrailingSlash(subdirName);
        auto insertIt = currentDir->m_subDirectories.Insert(subdirName, DirEntry());
        currentDir = &insertIt.Value();
        currentDirAbsPath.AppendPath(stats.m_sName);
      }
      else
      {
        currentDir->m_files.Insert(std::move(stats.m_sName), T{});
      }
    }
    if (outDirectoryExistsAlready != nullptr)
    {
      *outDirectoryExistsAlready = false;
    }
  }
  else
  {
    DirEntry* parentDir = FindDirectory(currentDirAbsPath);
    if (parentDir == nullptr)
    {
      return EZ_FAILURE;
    }

    if (outDirectoryExistsAlready != nullptr)
    {
      *outDirectoryExistsAlready = currentDirAbsPath.IsEmpty();
    }

    while (!currentDirAbsPath.IsEmpty())
    {
      const char* dirEnd = currentDirAbsPath.FindSubString("/");
      ezStringView subdirName(currentDirAbsPath.GetData(), dirEnd + 1);
      auto insertIt = parentDir->m_subDirectories.Insert(subdirName, DirEntry());
      parentDir = &insertIt.Value();
      currentDirAbsPath.Shrink(ezStringUtils::GetCharacterCount(subdirName.GetStartPointer(), subdirName.GetEndPointer()), 0);
    }
  }

  return EZ_SUCCESS;
}

template <typename T>
ezResult ezFileSystemMirror<T>::AddFile(const char* path, const T& value, bool* outFileExistsAlready, T* outOldValue)
{
  ezStringBuilder sPath = path;
  DirEntry* dir = FindDirectory(sPath);
  if (dir == nullptr)
  {
    return EZ_FAILURE; // file not under top level directory
  }

  if (sPath.FindSubString("/") != nullptr)
  {
    do
    {
      const char* dirEnd = sPath.FindSubString("/");
      ezStringView subdirName(sPath.GetData(), dirEnd + 1);
      auto insertIt = dir->m_subDirectories.Insert(subdirName, DirEntry());
      dir = &insertIt.Value();
      sPath.Shrink(ezStringUtils::GetCharacterCount(subdirName.GetStartPointer(), subdirName.GetEndPointer()), 0);
    } while (sPath.FindSubString("/") != nullptr);
  }
  auto it = dir->m_files.Find(sPath);
  // Do not add the file twice
  if (!it.IsValid())
  {
    dir->m_files.Insert(sPath, value);
    if (outFileExistsAlready != nullptr)
    {
      *outFileExistsAlready = false;
    }
  }
  else
  {
    if (outFileExistsAlready != nullptr)
    {
      *outFileExistsAlready = true;
    }
    if (outOldValue != nullptr)
    {
      *outOldValue = it.Value();
    }
    it.Value() = value;
  }
  return EZ_SUCCESS;
}

template <typename T>
ezResult ezFileSystemMirror<T>::RemoveFile(const char* path)
{
  ezStringBuilder sPath = path;
  DirEntry* dir = FindDirectory(sPath);
  if (dir == nullptr)
  {
    return EZ_FAILURE; // file not under top level directory
  }

  if (sPath.FindSubString("/") != nullptr)
  {
    return EZ_FAILURE; // file does not exist
  }

  if (dir->m_files.GetCount() == 0)
  {
    return EZ_FAILURE; // there are no files in this directory
  }

  auto it = dir->m_files.Find(sPath);
  if (!it.IsValid())
  {
    return EZ_FAILURE; // file does not exist
  }

  dir->m_files.Remove(it);
  return EZ_SUCCESS;
}

template <typename T>
ezResult ezFileSystemMirror<T>::RemoveDirectory(const char* path)
{
  ezStringBuilder parentPath = path;
  ezStringBuilder dirName = path;
  parentPath.PathParentDirectory();
  EnsureTrailingSlash(parentPath);
  dirName.Shrink(parentPath.GetCharacterCount(), 0);
  EnsureTrailingSlash(dirName);

  DirEntry* parentDir = FindDirectory(parentPath);
  if (parentDir == nullptr || !parentPath.IsEmpty())
  {
    return EZ_FAILURE;
  }

  if (!parentDir->m_subDirectories.Remove(dirName))
  {
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

template <typename T>
ezResult ezFileSystemMirror<T>::MoveDirectory(const char* fromPath, const char* toPath)
{
  ezStringBuilder sFromPath = fromPath;
  ezStringBuilder sFromName = fromPath;
  sFromPath.PathParentDirectory();
  EnsureTrailingSlash(sFromPath);
  sFromName.Shrink(sFromPath.GetCharacterCount(), 0);
  EnsureTrailingSlash(sFromName);


  ezStringBuilder sToPath = toPath;
  ezStringBuilder sToName = toPath;
  sToPath.PathParentDirectory();
  EnsureTrailingSlash(sToPath);
  sToName.Shrink(sToPath.GetCharacterCount(), 0);
  EnsureTrailingSlash(sToName);

  DirEntry* moveFromDir = FindDirectory(sFromPath);
  if (!moveFromDir)
  {
    return EZ_FAILURE;
  }
  EZ_ASSERT_DEV(sFromPath.IsEmpty(), "move from directory should fully exist");

  DirEntry* moveToDir = FindDirectory(sToPath);
  if (!moveToDir)
  {
    return EZ_FAILURE;
  }

  if (!sToPath.IsEmpty())
  {
    do
    {
      const char* dirEnd = sToPath.FindSubString("/");
      ezStringView subdirName(sToPath.GetData(), dirEnd + 1);
      auto insertIt = moveToDir->m_subDirectories.Insert(subdirName, DirEntry());
      moveToDir = &insertIt.Value();
      sToPath.Shrink(0, ezStringUtils::GetCharacterCount(subdirName.GetStartPointer(), subdirName.GetEndPointer()));
    } while (!sToPath.IsEmpty());
  }

  DirEntry movedDir;
  {
    auto fromIt = moveFromDir->m_subDirectories.Find(sFromName);
    if (!fromIt.IsValid())
    {
      return EZ_FAILURE;
    }

    movedDir = std::move(fromIt.Value());
    moveFromDir->m_subDirectories.Remove(fromIt);
  }

  moveToDir->m_subDirectories.Insert(sToName, std::move(movedDir));

  return EZ_SUCCESS;
}

namespace
{
  template <typename T>
  struct ezDirEnumerateState
  {
    typename ezFileSystemMirror<T>::DirEntry* dir;
    typename ezMap<ezString, typename ezFileSystemMirror<T>::DirEntry>::Iterator subDirIt;
  };
} // namespace

template <typename T>
ezResult ezFileSystemMirror<T>::Enumerate(const char* path, EnumerateFunc callbackFunc)
{
  ezHybridArray<ezDirEnumerateState<T>, 16> dirStack;
  ezStringBuilder sPath = path;
  if (!sPath.EndsWith("/"))
  {
    sPath.Append("/");
  }
  DirEntry* dirToEnumerate = FindDirectory(sPath);
  if (dirToEnumerate == nullptr)
  {
    return EZ_FAILURE;
  }
  if (!sPath.IsEmpty())
  {
    return EZ_FAILURE; // requested folder to enumerate doesn't exist
  }
  DirEntry* currentDir = dirToEnumerate;
  typename ezMap<ezString, ezFileSystemMirror::DirEntry>::Iterator currentSubDirIt = currentDir->m_subDirectories.GetIterator();
  sPath = path;

  while (currentDir != nullptr)
  {
    if (currentSubDirIt.IsValid())
    {
      DirEntry* nextDir = &currentSubDirIt.Value();
      sPath.AppendPath(currentSubDirIt.Key());
      currentSubDirIt.Next();
      dirStack.PushBack({currentDir, currentSubDirIt});
      currentDir = nextDir;
    }
    else
    {
      ezStringBuilder sFilePath;
      for (auto& file : currentDir->m_files)
      {
        sFilePath = sPath;
        sFilePath.AppendPath(file.Key());
        callbackFunc(sFilePath, Type::File);
      }

      if (currentDir != dirToEnumerate)
      {
        if (sPath.EndsWith("/") && sPath.GetElementCount() > 1)
        {
          sPath.Shrink(0, 1);
        }
        callbackFunc(sPath, Type::Directory);
      }

      if (dirStack.IsEmpty())
      {
        currentDir = nullptr;
      }
      else
      {
        currentDir = dirStack.PeekBack().dir;
        currentSubDirIt = dirStack.PeekBack().subDirIt;
        dirStack.PopBack();
        sPath.PathParentDirectory();
        if (sPath.GetElementCount() > 1 && sPath.EndsWith("/"))
        {
          sPath.Shrink(0, 1);
        }
      }
    }
  }

  return EZ_SUCCESS;
}

template <typename T>
typename ezFileSystemMirror<T>::DirEntry* ezFileSystemMirror<T>::FindDirectory(ezStringBuilder& path)
{
  if (!path.StartsWith(m_topLevelDirPath))
  {
    return nullptr;
  }
  path.TrimWordStart(m_topLevelDirPath);

  DirEntry* currentDir = &m_topLevelDir;

  bool found = false;
  do
  {
    found = false;
    for (auto& dir : currentDir->m_subDirectories)
    {
      if (path.StartsWith(dir.Key()))
      {
        currentDir = &dir.Value();
        path.TrimWordStart(dir.Key());
        path.TrimWordStart("/");
        found = true;
        break;
      }
    }
  } while (found);

  return currentDir;
}
