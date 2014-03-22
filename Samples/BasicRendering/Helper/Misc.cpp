#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileReader.h>

#include "Misc.h"

namespace DontUse
{
  ezResult ReadCompleteFile(const char* szFile, ezDynamicArray<ezUInt8>& out_FileContent)
  {
    out_FileContent.Clear();

    ezFileReader File;
    if (File.Open(szFile) == EZ_FAILURE)
      return EZ_FAILURE;

    ezUInt8 uiTemp[1024];
    while (true)
    {
      const ezUInt64 uiRead = File.ReadBytes(uiTemp, 1023);

      if (uiRead == 0)
        return EZ_SUCCESS; // file is automatically closed here

      out_FileContent.PushBackRange(ezArrayPtr<ezUInt8>(uiTemp, (ezUInt32)uiRead));
    }

    return EZ_SUCCESS; // file is automatically closed here
  }
}