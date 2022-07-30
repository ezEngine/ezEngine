#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Types/UniquePtr.h>
#include <Foundation/IO/OSFile.h>

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
  ezResult AddDirectory(const char* path);

  // \brief Adds a file. Creates directories if they do not exist.
  ezResult AddFile(const char* path, bool* outFileExistsAlready);

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

  void EnsureTrailingSlash(ezStringBuilder& builder)
  {
    if(!builder.EndsWith("/"))
    {
      builder.Append("/");
    }
  }

  void RemoveTrailingSlash(ezStringBuilder& builder)
  {
    if(builder.EndsWith("/"))
    {
      builder.Shrink(0, 1);
    }
  }
}

ezFileSystemMirror::ezFileSystemMirror() = default;
ezFileSystemMirror::~ezFileSystemMirror() = default;

ezResult ezFileSystemMirror::AddDirectory(const char* path)
{
  ezStringBuilder currentDirAbsPath = path;
  currentDirAbsPath.MakeCleanPath();
  EnsureTrailingSlash(currentDirAbsPath);

  if (m_topLevelDirPath.IsEmpty())
  {
    m_topLevelDirPath = currentDirAbsPath;
    currentDirAbsPath.Shrink(0,1); // remove trailing /

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
        uint32_t insertPos = FindInsertPositionLikeyOrdered(currentDir->m_files, stats.m_sName);
        currentDir->m_files.Insert(std::move(stats.m_sName), insertPos);
      }
    }
  }
  else
  {
    DirEntry* parentDir = FindDirectory(currentDirAbsPath);
    if(parentDir == nullptr)
    {
      return EZ_FAILURE;
    }

    while(!currentDirAbsPath.IsEmpty())
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

ezResult ezFileSystemMirror::AddFile(const char* path, bool* outFileExistsAlready)
{
  ezStringBuilder sPath = path;
  DirEntry* dir = FindDirectory(sPath);
  if(dir == nullptr)
  {
    return EZ_FAILURE; // file not under top level directory
  }

  if(sPath.FindSubString("/") != nullptr)
  {
    do
    {
      const char* dirEnd = sPath.FindSubString("/");
      ezStringView subdirName(sPath.GetData(), dirEnd + 1);
      auto insertIt = dir->m_subDirectories.Insert(subdirName, DirEntry());
      dir = &insertIt.Value();
      sPath.Shrink(ezStringUtils::GetCharacterCount(subdirName.GetStartPointer(), subdirName.GetEndPointer()), 0);
    } while(sPath.FindSubString("/") != nullptr);
  }
  uint32_t insertPos = FindInsertPosition(dir->m_files, sPath);
  // Do not add the file twice
  if(insertPos == 0 || dir->m_files[insertPos - 1] != sPath)
  {
    dir->m_files.Insert(sPath, insertPos);
    if(outFileExistsAlready != nullptr)
    {
      *outFileExistsAlready = false;
    }
  }
  else
  {
    if(outFileExistsAlready != nullptr)
    {
      *outFileExistsAlready = true;
    }
  }
  return EZ_SUCCESS;
}

ezResult ezFileSystemMirror::RemoveFile(const char* path)
{
  ezStringBuilder sPath = path;
  DirEntry* dir = FindDirectory(sPath);
  if(dir == nullptr)
  {
    return EZ_FAILURE; // file not under top level directory
  }

  if(sPath.FindSubString("/") != nullptr)
  {
    return EZ_FAILURE; // file does not exist
  }

  if(dir->m_files.GetCount() == 0)
  {
    return EZ_FAILURE; // there are no files in this directory
  }

  uint32_t shouldBePos = FindInsertPosition(dir->m_files, sPath);
  if(shouldBePos == 0 || dir->m_files[shouldBePos - 1] != sPath)
  {
    return EZ_FAILURE; // file does not exist
  }

  dir->m_files.RemoveAtAndCopy(shouldBePos - 1);
  return EZ_SUCCESS;
}

ezResult ezFileSystemMirror::RemoveDirectory(const char* path)
{
  ezStringBuilder parentPath = path;
  ezStringBuilder dirName = path;
  parentPath.PathParentDirectory();
  EnsureTrailingSlash(parentPath);
  dirName.Shrink(parentPath.GetCharacterCount(), 0);
  EnsureTrailingSlash(dirName);

  DirEntry* parentDir = FindDirectory(parentPath);
  if(parentDir == nullptr || !parentPath.IsEmpty())
  {
    return EZ_FAILURE;
  }

  if(!parentDir->m_subDirectories.Remove(dirName))
  {
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;  
}

ezResult ezFileSystemMirror::MoveDirectory(const char* fromPath, const char* toPath)
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
  if(!moveFromDir)
  {
    return EZ_FAILURE;
  }
  EZ_ASSERT_DEV(sFromPath.IsEmpty(), "move from directory should fully exist");

  DirEntry* moveToDir = FindDirectory(sToPath);
  if(!moveToDir)
  {
    return EZ_FAILURE;
  }

  if(!sToPath.IsEmpty())
  {
    do
    {
      const char* dirEnd = sToPath.FindSubString("/");
      ezStringView subdirName(sToPath.GetData(), dirEnd + 1);
      auto insertIt = moveToDir->m_subDirectories.Insert(subdirName, DirEntry());
      moveToDir = &insertIt.Value();
      sToPath.Shrink(0, ezStringUtils::GetCharacterCount(subdirName.GetStartPointer(), subdirName.GetEndPointer()));
    } while(!sToPath.IsEmpty());   
  }

  DirEntry movedDir;
  {
    auto fromIt = moveFromDir->m_subDirectories.Find(sFromName);
    if(!fromIt.IsValid())
    {
      return EZ_FAILURE;
    }

    movedDir = std::move(fromIt.Value());
    moveFromDir->m_subDirectories.Remove(fromIt);
  }

  moveToDir->m_subDirectories.Insert(sToName, std::move(movedDir));

  return EZ_SUCCESS;
}

namespace {
  struct ezDirEnumerateState
  {
    ezFileSystemMirror::DirEntry* dir;
    ezMap<ezString, ezFileSystemMirror::DirEntry>::Iterator subDirIt;
  };
}

ezResult ezFileSystemMirror::Enumerate(const char* path, EnumerateFunc callbackFunc)
{
  ezHybridArray<ezDirEnumerateState, 16> dirStack;
  ezStringBuilder sPath = path;
  if(!sPath.EndsWith("/"))
  {
    sPath.Append("/");
  }
  DirEntry* dirToEnumerate = FindDirectory(sPath);
  if(dirToEnumerate == nullptr)
  {
    return EZ_FAILURE;
  }
  if(!sPath.IsEmpty())
  {
    return EZ_FAILURE; // requested folder to enumerate doesn't exist
  }
  DirEntry* currentDir = dirToEnumerate;
  ezMap<ezString, ezFileSystemMirror::DirEntry>::Iterator currentSubDirIt = currentDir->m_subDirectories.GetIterator();
  sPath = path;

  while (currentDir != nullptr)
  {
    if(currentSubDirIt.IsValid())
    {
      DirEntry* nextDir = &currentSubDirIt.Value();
      sPath.AppendPath(currentSubDirIt.Key());
      currentSubDirIt.Next();
      dirStack.PushBack({currentDir, currentSubDirIt});
      currentDir = nextDir;
    }
    else
    {
      if(currentDir != dirToEnumerate)
      {
        callbackFunc(sPath, Type::Directory);
      }
      ezStringBuilder sFilePath;
      for(auto& file : currentDir->m_files)
      {
        sFilePath = sPath;
        sFilePath.AppendPath(file);
        callbackFunc(sFilePath, Type::File);
      }
      if(dirStack.IsEmpty())
      {
        currentDir = nullptr;
      }
      else
      {
        currentDir = dirStack.PeekBack().dir;
        currentSubDirIt = dirStack.PeekBack().subDirIt;
        dirStack.PopBack();
        sPath.PathParentDirectory();
        if(sPath.GetElementCount() > 1 && sPath.EndsWith("/"))
        {
          sPath.Shrink(0, 1);
        }
      }
    }
  }

  return EZ_SUCCESS;
}

ezFileSystemMirror::DirEntry* ezFileSystemMirror::FindDirectory(ezStringBuilder& path)
{
  if(!path.StartsWith(m_topLevelDirPath))
  {
    return nullptr;
  }
  path.TrimWordStart(m_topLevelDirPath);

  DirEntry* currentDir = &m_topLevelDir;

  bool found = false;
  do {
    found = false;
    for(auto& dir : currentDir->m_subDirectories)
    {
      if(path.StartsWith(dir.Key()))
      {
        currentDir = &dir.Value();
        path.TrimWordStart(dir.Key());
        path.TrimWordStart("/");
        found = true;
        break;
      }
    }
  } while(found);

  return currentDir;
}