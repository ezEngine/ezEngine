#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileReader.h>

#include "Misc.h"

namespace DontUse
{
  ezResult ReadCompleteFile(const char* szFile, ezStringBuilder& out_FileContent)
  {
    ezFileReader File;
    if (File.Open(szFile) == EZ_FAILURE)
      return EZ_FAILURE;

    out_FileContent.ReadAll(File);

    return EZ_SUCCESS; // file is automatically closed here
  }
}