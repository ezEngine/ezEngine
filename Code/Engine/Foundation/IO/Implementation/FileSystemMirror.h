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
  ezResult AddDirectory(ezStringView sPath, bool* out_pDirectoryExistsAlready = nullptr);

  // \brief Adds a file. Creates directories if they do not exist.
  ezResult AddFile(ezStringView sPath, const T& value, bool* out_pFileExistsAlready, T* out_pOldValue);

  // \brief Removes a file.
  ezResult RemoveFile(ezStringView sPath);

  // \brief Removes a directory. Deletes any files & directories inside.
  ezResult RemoveDirectory(ezStringView sPath);

  // \brief Moves a directory. Any files & folders inside are moved with it.
  ezResult MoveDirectory(ezStringView sFromPath, ezStringView sToPath);

  using EnumerateFunc = ezDelegate<void(const ezStringBuilder& path, Type type)>;

  // \brief Enumerates the files & directories under the given path
  ezResult Enumerate(ezStringView sPath, EnumerateFunc callbackFunc);

  // \brief On success, out_Type will contains the type of the object (file or folder).
  ezResult GetType(ezStringView sPath, Type& out_Type);

private:
  DirEntry* FindDirectory(ezStringBuilder& path);

private:
  DirEntry m_TopLevelDir;
  ezString m_sTopLevelDirPath;
};

namespace
{
  void EnsureTrailingSlash(ezStringBuilder& ref_sBuilder)
  {
    if (!ref_sBuilder.EndsWith("/"))
    {
      ref_sBuilder.Append("/");
    }
  }

  void RemoveTrailingSlash(ezStringBuilder& ref_sBuilder)
  {
    if (ref_sBuilder.EndsWith("/"))
    {
      ref_sBuilder.Shrink(0, 1);
    }
  }
} // namespace

template <typename T>
ezFileSystemMirror<T>::ezFileSystemMirror() = default;

template <typename T>
ezFileSystemMirror<T>::~ezFileSystemMirror() = default;

template <typename T>
ezResult ezFileSystemMirror<T>::AddDirectory(ezStringView sPath, bool* out_pDirectoryExistsAlready)
{
  ezStringBuilder currentDirAbsPath = sPath;
  currentDirAbsPath.MakeCleanPath();
  EnsureTrailingSlash(currentDirAbsPath);

  if (m_sTopLevelDirPath.IsEmpty())
  {
    m_sTopLevelDirPath = currentDirAbsPath;
    currentDirAbsPath.Shrink(0, 1); // remove trailing /

    DirEntry* currentDir = &m_TopLevelDir;

    ezHybridArray<DirEntry*, 16> m_dirStack;

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
    if (out_pDirectoryExistsAlready != nullptr)
    {
      *out_pDirectoryExistsAlready = false;
    }
  }
  else
  {
    DirEntry* parentDir = FindDirectory(currentDirAbsPath);
    if (parentDir == nullptr)
    {
      return EZ_FAILURE;
    }

    if (out_pDirectoryExistsAlready != nullptr)
    {
      *out_pDirectoryExistsAlready = currentDirAbsPath.IsEmpty();
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
ezResult ezFileSystemMirror<T>::AddFile(ezStringView sPath0, const T& value, bool* out_pFileExistsAlready, T* out_pOldValue)
{
  ezStringBuilder sPath = sPath0;
  DirEntry* dir = FindDirectory(sPath);
  if (dir == nullptr)
  {
    return EZ_FAILURE; // file not under top level directory
  }

  const char* szSlashPos = sPath.FindSubString("/");

  while (szSlashPos != nullptr)
  {
    ezStringView subdirName(sPath.GetData(), szSlashPos + 1);
    auto insertIt = dir->m_subDirectories.Insert(subdirName, DirEntry());
    dir = &insertIt.Value();
    sPath.Shrink(ezStringUtils::GetCharacterCount(subdirName.GetStartPointer(), subdirName.GetEndPointer()), 0);
    szSlashPos = sPath.FindSubString("/");
  }

  auto it = dir->m_files.Find(sPath);
  // Do not add the file twice
  if (!it.IsValid())
  {
    dir->m_files.Insert(sPath, value);
    if (out_pFileExistsAlready != nullptr)
    {
      *out_pFileExistsAlready = false;
    }
  }
  else
  {
    if (out_pFileExistsAlready != nullptr)
    {
      *out_pFileExistsAlready = true;
    }
    if (out_pOldValue != nullptr)
    {
      *out_pOldValue = it.Value();
    }
    it.Value() = value;
  }
  return EZ_SUCCESS;
}

template <typename T>
ezResult ezFileSystemMirror<T>::RemoveFile(ezStringView sPath0)
{
  ezStringBuilder sPath = sPath0;
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
ezResult ezFileSystemMirror<T>::RemoveDirectory(ezStringView sPath)
{
  ezStringBuilder parentPath = sPath;
  ezStringBuilder dirName = sPath;
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
ezResult ezFileSystemMirror<T>::MoveDirectory(ezStringView sFromPath0, ezStringView sToPath0)
{
  ezStringBuilder sFromPath = sFromPath0;
  ezStringBuilder sFromName = sFromPath0;
  sFromPath.PathParentDirectory();
  EnsureTrailingSlash(sFromPath);
  sFromName.Shrink(sFromPath.GetCharacterCount(), 0);
  EnsureTrailingSlash(sFromName);


  ezStringBuilder sToPath = sToPath0;
  ezStringBuilder sToName = sToPath0;
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
ezResult ezFileSystemMirror<T>::Enumerate(ezStringView sPath0, EnumerateFunc callbackFunc)
{
  ezHybridArray<ezDirEnumerateState<T>, 16> dirStack;
  ezStringBuilder sPath = sPath0;
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
  sPath = sPath0;

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
typename ezResult ezFileSystemMirror<T>::GetType(ezStringView sPath0, Type& out_Type)
{
  ezStringBuilder sPath = sPath0;
  DirEntry* dir = FindDirectory(sPath);
  if (dir == nullptr)
  {
    return EZ_FAILURE; // file not under top level directory
  }

  auto it = dir->m_files.Find(sPath);
  if (it.IsValid())
  {
    out_Type = ezFileSystemMirror::Type::File;
    return EZ_SUCCESS;
  }

  auto itDir = dir->m_subDirectories.Find(sPath);
  if (itDir.IsValid())
  {
    out_Type = ezFileSystemMirror::Type::Directory;
    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

template <typename T>
typename ezFileSystemMirror<T>::DirEntry* ezFileSystemMirror<T>::FindDirectory(ezStringBuilder& path)
{
  if (!path.StartsWith(m_sTopLevelDirPath))
  {
    return nullptr;
  }
  path.TrimWordStart(m_sTopLevelDirPath);

  DirEntry* currentDir = &m_TopLevelDir;

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
