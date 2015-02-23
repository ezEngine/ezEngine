#pragma once

#include <Foundation/IO/DirectoryWatcher.h>

struct ezDirectoryWatcherImpl
{
};

ezDirectoryWatcher::ezDirectoryWatcher()
  : m_bDirectoryOpen(false)
  , m_pImpl(nullptr)
{
}

ezResult ezDirectoryWatcher::OpenDirectory(const ezString& path, ezBitflags<Watch> whatToWatch)
{
  EZ_ASSERT_NOT_IMPLEMENTED
  return EZ_FAILURE;
}

void ezDirectoryWatcher::CloseDirectory()
{
  EZ_ASSERT_NOT_IMPLEMENTED
}

ezDirectoryWatcher::~ezDirectoryWatcher()
{
}

void ezDirectoryWatcher::EnumerateChanges(ezDelegate<void(const char* filename, Action action)> func)
{
  EZ_ASSERT_NOT_IMPLEMENTED
}