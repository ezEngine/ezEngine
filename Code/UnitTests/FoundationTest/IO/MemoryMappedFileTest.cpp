#include <FoundationTestPCH.h>

#include <Foundation/IO/MemoryMappedFile.h>
#include <Foundation/IO/OSFile.h>

#if EZ_ENABLED(EZ_SUPPORTS_MEMORY_MAPPED_FILE)

EZ_CREATE_SIMPLE_TEST(IO, MemoryMappedFile)
{
  ezStringBuilder sOutputFile = ezTestFramework::GetInstance()->GetAbsOutputPath();
  sOutputFile.MakeCleanPath();
  sOutputFile.AppendPath("IO");
  sOutputFile.AppendPath("MemoryMappedFile.dat");

  const ezUInt32 uiFileSize = 1024 * 1024 * 16; // * 4

  // generate test data
  {
    ezOSFile file;
    if (EZ_TEST_BOOL_MSG(file.Open(sOutputFile, ezFileOpenMode::Write).Succeeded(), "File for memory mapping could not be created").Failed())
      return;

    ezDynamicArray<ezUInt32> data;
    data.SetCountUninitialized(uiFileSize);

    for (ezUInt32 i = 0; i < uiFileSize; ++i)
    {
      data[i] = i;
    }

    file.Write(data.GetData(), data.GetCount() * sizeof(ezUInt32));
    file.Close();
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Memory map for writing")
  {
    ezMemoryMappedFile memFile;

    if (EZ_TEST_BOOL_MSG(memFile.Open(sOutputFile, ezMemoryMappedFile::Mode::ReadWrite).Succeeded(), "Memory mapping a file failed")
          .Failed())
      return;

    EZ_TEST_BOOL(memFile.GetWritePointer() != nullptr);
    EZ_TEST_INT(memFile.GetFileSize(), uiFileSize * sizeof(ezUInt32));

    ezUInt32* ptr = static_cast<ezUInt32*>(memFile.GetWritePointer());

    for (ezUInt32 i = 0; i < uiFileSize; ++i)
    {
      EZ_TEST_INT(ptr[i], i);
      ptr[i] = ptr[i] + 1;
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Memory map for reading")
  {
    ezMemoryMappedFile memFile;

    if (EZ_TEST_BOOL_MSG(memFile.Open(sOutputFile, ezMemoryMappedFile::Mode::ReadOnly).Succeeded(), "Memory mapping a file failed")
          .Failed())
      return;

    EZ_TEST_BOOL(memFile.GetReadPointer() != nullptr);
    EZ_TEST_INT(memFile.GetFileSize(), uiFileSize * sizeof(ezUInt32));

    const ezUInt32* ptr = static_cast<const ezUInt32*>(memFile.GetReadPointer());

    for (ezUInt32 i = 0; i < uiFileSize; ++i)
    {
      EZ_TEST_INT(ptr[i], i + 1);
    }
  }

  ezOSFile::DeleteFile(sOutputFile);
}
#endif
