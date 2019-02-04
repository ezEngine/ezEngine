#pragma once

#include <Foundation/IO/DirectoryWatcher.h>

ezDirectoryWatcher::ezDirectoryWatcher()
    : m_pImpl(nullptr)
{
}

ezResult ezDirectoryWatcher::OpenDirectory(const ezString& absolutePath, ezBitflags<Watch> whatToWatch)
{
  return EZ_FAILURE;
}

void ezDirectoryWatcher::CloseDirectory()
{
}

ezDirectoryWatcher::~ezDirectoryWatcher()
{
  CloseDirectory();
}

void ezDirectoryWatcher::EnumerateChanges(ezDelegate<void(const char* filename, ezDirectoryWatcherAction action)> func)
{
}

