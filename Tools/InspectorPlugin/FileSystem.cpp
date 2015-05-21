#include <PCH.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/IO/FileSystem/FileSystem.h>

static ezInt32 s_iDataDirCounter = 0;
static ezMap<ezString, ezInt32, ezCompareHelper<ezString>, ezStaticAllocatorWrapper> s_KnownDataDirs;

static void FileSystemEventHandler(const ezFileSystem::FileEvent& e)
{
  switch (e.m_EventType)
  {
  case ezFileSystem::FileEventType::AddDataDirectorySucceeded:
    {
      bool bExisted = false;
      auto it = s_KnownDataDirs.FindOrAdd(e.m_szFileOrDirectory, &bExisted);

      if (!bExisted)
      {
        it.Value() = s_iDataDirCounter;
        ++s_iDataDirCounter;
      }

      ezStringBuilder sName;
      sName.Format("IO/DataDirs/Dir%02i", it.Value());

      ezStats::SetStat(sName.GetData(), e.m_szFileOrDirectory);
    }
    break;

  case ezFileSystem::FileEventType::RemoveDataDirectory:
    {
      auto it = s_KnownDataDirs.Find(e.m_szFileOrDirectory);

      if (!it.IsValid())
        break;

      ezStringBuilder sName;
      sName.Format("IO/DataDirs/Dir%02i", it.Value());

      ezStats::RemoveStat(sName.GetData());
    }
    break;

  default:
    break;
  }
}

void AddFileSystemEventHandler()
{
  ezFileSystem::RegisterEventHandler(FileSystemEventHandler);
}

void RemoveFileSystemEventHandler()
{
  ezFileSystem::UnregisterEventHandler(FileSystemEventHandler);
}



