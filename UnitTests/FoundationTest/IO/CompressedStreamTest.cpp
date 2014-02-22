#include <PCH.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/IO/CompressedStream.h>
#include <Foundation/IO/MemoryStream.h>

EZ_CREATE_SIMPLE_TEST(IO, CompressedStream)
{
  ezMemoryStreamStorage StreamStorage;

  ezMemoryStreamWriter MemoryWriter(&StreamStorage);
  ezMemoryStreamReader MemoryReader(&StreamStorage);

  ezCompressedStreamReader CompressedReader(MemoryReader);
  ezCompressedStreamWriter CompressedWriter(MemoryWriter, ezCompressedStreamWriter::Fastest);
 
  const ezUInt32 uiItems = 1024 * 1024 * 5; // if you change this, you might need to adjust the expected compression ratio
  const float fExpectedCompressionRatio = 2.5f; // this is a guess that is based on the current input data and size

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Compress Data")
  {
    ezDynamicArray<ezUInt32> Data;
    Data.SetCount(uiItems);

    for (ezUInt32 i = 0; i < uiItems; ++i)
      Data[i] = i;

    ezUInt32 uiStartPos = 0;

    // write the data in blocks that get larger and larger
    for (ezUInt32 iWrite = 1; iWrite < uiItems; ++iWrite)
    {
      iWrite = ezMath::Min(iWrite, uiItems - uiStartPos);

      if (iWrite == 0)
        break;

      EZ_TEST_BOOL(CompressedWriter.WriteBytes(&Data[uiStartPos], sizeof(ezUInt32) * iWrite) == EZ_SUCCESS);

      uiStartPos += iWrite;
    }

    // flush all data
    CompressedWriter.CloseStream();

    const ezUInt32 uiCompressed = CompressedWriter.GetCompressedSize();
    const ezUInt32 uiUncompressed = CompressedWriter.GetUncompressedSize();

    EZ_TEST_INT(uiUncompressed, uiItems * sizeof(ezUInt32));

    EZ_TEST_BOOL((float) uiCompressed <= (float) uiUncompressed / fExpectedCompressionRatio);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Uncompress Data")
  {
    

    ezDynamicArray<ezUInt32> Data;
    Data.SetCount(uiItems);

    for (ezUInt32 i = 0; i < uiItems; ++i)
      Data[i] = 0;

    ezUInt32 uiStartPos = 0;
    bool bSkip = false;

    // read the data in blocks that get larger and larger
    for (ezUInt32 iRead = 1; iRead < uiItems; ++iRead)
    {
      ezUInt32 iToRead = ezMath::Min(iRead, uiItems - uiStartPos);

      if (iToRead == 0)
        break;

      if (bSkip)
      {
        const ezUInt64 uiReadFromStream = CompressedReader.SkipBytes(sizeof(ezUInt32) * iToRead);
        EZ_TEST_BOOL(uiReadFromStream == sizeof(ezUInt32) * iToRead);
      }
      else
      {
        const ezUInt64 uiReadFromStream = CompressedReader.ReadBytes(&Data[uiStartPos], sizeof(ezUInt32) * iToRead);
        EZ_TEST_BOOL(uiReadFromStream == sizeof(ezUInt32) * iToRead);
      }

      bSkip = !bSkip;

      uiStartPos += iToRead;
    }


    bSkip = false;
    uiStartPos = 0;

    // read the data in blocks that get larger and larger
    for (ezUInt32 iRead = 1; iRead < uiItems; ++iRead)
    {
      ezUInt32 iToRead = ezMath::Min(iRead, uiItems - uiStartPos);

      if (iToRead == 0)
        break;

      if (bSkip)
      {
        for (ezUInt32 i = 0; i < iToRead; ++i)
        {
          EZ_TEST_INT(Data[uiStartPos + i], 0);
        }
      }
      else
      {
        for (ezUInt32 i = 0; i < iToRead; ++i)
        {
          EZ_TEST_INT(Data[uiStartPos + i], uiStartPos + i);
        }
      }

      bSkip = !bSkip;
      uiStartPos += iToRead;
    }

    // test reading after the end of the stream
    for (ezUInt32 i = 0; i < 1000; ++i)
    {
      ezUInt32 uiTemp = 0;
      EZ_TEST_BOOL(CompressedReader.ReadBytes(&uiTemp, sizeof(ezUInt32)) == 0);
    }
  }
}

