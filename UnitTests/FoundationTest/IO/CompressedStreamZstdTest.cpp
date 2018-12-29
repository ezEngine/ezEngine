#include <PCH.h>

#include <Foundation/IO/CompressedStreamZstd.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/Stream.h>

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT

EZ_CREATE_SIMPLE_TEST(IO, CompressedStreamZstd)
{
  ezDynamicArray<ezUInt32> TestData;

  // create the test data
  // a repetition of a counting sequence that is getting longer and longer, ie:
  // 0, 0,1, 0,1,2, 0,1,2,3, 0,1,2,3,4, ...
  {
    TestData.SetCountUninitialized(1024 * 1024 * 8);

    const ezUInt32 uiItems = TestData.GetCount();
    ezUInt32 uiStartPos = 0;

    for (ezUInt32 uiWrite = 1; uiWrite < uiItems; ++uiWrite)
    {
      uiWrite = ezMath::Min(uiWrite, uiItems - uiStartPos);

      if (uiWrite == 0)
        break;

      for (ezUInt32 i = 0; i < uiWrite; ++i)
      {
        TestData[uiStartPos + i] = i;
      }

      uiStartPos += uiWrite;
    }
  }


  ezMemoryStreamStorage StreamStorage;

  ezMemoryStreamWriter MemoryWriter(&StreamStorage);
  ezMemoryStreamReader MemoryReader(&StreamStorage);

  ezCompressedStreamReaderZstd CompressedReader(&MemoryReader);
  ezCompressedStreamWriterZstd CompressedWriter;

  const float fExpectedCompressionRatio = 900.0f; // this is a guess that is based on the current input data and size

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Compress Data")
  {
    CompressedWriter.SetOutputStream(&MemoryWriter);

    bool bFlush = true;

    ezUInt32 uiWrite = 1;
    for (ezUInt32 i = 0; i < TestData.GetCount();)
    {
      uiWrite = ezMath::Min<ezUInt32>(uiWrite, TestData.GetCount() - i);

      EZ_TEST_BOOL(CompressedWriter.WriteBytes(&TestData[i], sizeof(ezUInt32) * uiWrite) == EZ_SUCCESS);

      if (bFlush)
      {
        // this actually hurts compression rates
        EZ_TEST_BOOL(CompressedWriter.Flush() == EZ_SUCCESS);
      }

      bFlush = !bFlush;

      i += uiWrite;
      uiWrite += 17; // try different sizes to write
    }

    // flush all data
    CompressedWriter.FinishCompressedStream();

    const ezUInt32 uiCompressed = CompressedWriter.GetCompressedSize();
    const ezUInt32 uiUncompressed = CompressedWriter.GetUncompressedSize();

    EZ_TEST_INT(uiUncompressed, TestData.GetCount() * sizeof(ezUInt32));

    const float fRatio = (float)uiUncompressed / (float)uiCompressed;
    EZ_TEST_BOOL(fRatio >= fExpectedCompressionRatio);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Uncompress Data")
  {
    bool bSkip = false;
    ezUInt32 uiStartPos = 0;

    ezDynamicArray<ezUInt32> TestDataRead = TestData; // initialize with identical data, makes comparing the skipped parts easier

    // read the data in blocks that get larger and larger
    for (ezUInt32 iRead = 1; iRead < TestData.GetCount(); ++iRead)
    {
      ezUInt32 iToRead = ezMath::Min(iRead, TestData.GetCount() - uiStartPos);

      if (iToRead == 0)
        break;

      if (bSkip)
      {
        const ezUInt64 uiReadFromStream = CompressedReader.SkipBytes(sizeof(ezUInt32) * iToRead);
        EZ_TEST_BOOL(uiReadFromStream == sizeof(ezUInt32) * iToRead);
      }
      else
      {
        // overwrite part we are going to read from the stream, to make sure it re-reads the correct data
        for (ezUInt32 i = 0; i < iToRead; ++i)
        {
          TestDataRead[uiStartPos + i] = 0;
        }

        const ezUInt64 uiReadFromStream = CompressedReader.ReadBytes(&TestDataRead[uiStartPos], sizeof(ezUInt32) * iToRead);
        EZ_TEST_BOOL(uiReadFromStream == sizeof(ezUInt32) * iToRead);
      }

      bSkip = !bSkip;

      uiStartPos += iToRead;
    }

    EZ_TEST_BOOL(TestData == TestDataRead);

    // test reading after the end of the stream
    for (ezUInt32 i = 0; i < 1000; ++i)
    {
      ezUInt32 uiTemp = 0;
      EZ_TEST_BOOL(CompressedReader.ReadBytes(&uiTemp, sizeof(ezUInt32)) == 0);
    }
  }
}

#endif
