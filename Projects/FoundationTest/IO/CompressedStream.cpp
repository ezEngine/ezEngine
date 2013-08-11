#include <PCH.h>
#include <Foundation/IO/IBinaryStream.h>
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

  EZ_TEST_BLOCK(true, "Compress Data")
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

      EZ_TEST(CompressedWriter.WriteBytes(&Data[uiStartPos], sizeof(ezUInt32) * iWrite) == EZ_SUCCESS);

      uiStartPos += iWrite;
    }

    // flush all data
    CompressedWriter.CloseStream();

    const ezUInt32 uiCompressed = CompressedWriter.GetCompressedSize();
    const ezUInt32 uiUncompressed = CompressedWriter.GetUncompressedSize();

    EZ_TEST_INT(uiUncompressed, uiItems * sizeof(ezUInt32));

    EZ_TEST((float) uiCompressed <= (float) uiUncompressed / fExpectedCompressionRatio);
  }

  EZ_TEST_BLOCK(true, "Uncompress Data")
  {
    

    ezDynamicArray<ezUInt32> Data;
    Data.SetCount(uiItems);

    ezUInt32 uiStartPos = 0;

    // read the data in blocks that get larger and larger
    for (ezUInt32 iRead = 1; iRead < uiItems; ++iRead)
    {
      iRead = ezMath::Min(iRead, uiItems - uiStartPos);

      if (iRead == 0)
        break;

      EZ_TEST(CompressedReader.ReadBytes(&Data[uiStartPos], sizeof(ezUInt32) * iRead) == sizeof(ezUInt32) * iRead);

      uiStartPos += iRead;
    }

    for (ezUInt32 i = 0; i < uiItems; ++i)
      EZ_TEST_INT(Data[i], i);

    // test reading after the end of the stream
    for (ezUInt32 i = 0; i < 1000; ++i)
    {
      ezUInt32 uiTemp = 0;
      EZ_TEST(CompressedReader.ReadBytes(&uiTemp, sizeof(ezUInt32)) == 0);
    }
  }
}

