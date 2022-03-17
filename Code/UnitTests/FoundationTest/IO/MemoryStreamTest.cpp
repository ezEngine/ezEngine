#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/IO/MemoryStream.h>

EZ_CREATE_SIMPLE_TEST_GROUP(IO);

EZ_CREATE_SIMPLE_TEST(IO, MemoryStream)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Memory Stream Reading / Writing")
  {
    ezDefaultMemoryStreamStorage StreamStorage;

    // Create reader
    ezMemoryStreamReader StreamReader(&StreamStorage);

    // Create writer
    ezMemoryStreamWriter StreamWriter(&StreamStorage);

    // Temp read pointer
    ezUInt8* pPointer = reinterpret_cast<ezUInt8*>(0x41); // Should crash when accessed

    // Try reading from an empty stream (should not crash, just return 0 bytes read)
    ezUInt64 uiBytesRead = StreamReader.ReadBytes(pPointer, 128);

    EZ_TEST_BOOL(uiBytesRead == 0);


    // Now try writing data to the stream and reading it back
    ezUInt32 uiData[1024];
    for (ezUInt32 i = 0; i < 1024; i++)
      uiData[i] = rand();

    // Calculate the hash so we can reuse the array
    const ezUInt32 uiHashBeforeWriting = ezHashingUtils::xxHash32(uiData, sizeof(ezUInt32) * 1024);

    // Write the data
    EZ_TEST_BOOL(StreamWriter.WriteBytes(reinterpret_cast<const ezUInt8*>(uiData), sizeof(ezUInt32) * 1024) == EZ_SUCCESS);

    EZ_TEST_BOOL(StreamWriter.GetByteCount64() == sizeof(ezUInt32) * 1024);
    EZ_TEST_BOOL(StreamWriter.GetByteCount64() == StreamReader.GetByteCount64());
    EZ_TEST_BOOL(StreamWriter.GetByteCount64() == StreamStorage.GetStorageSize64());


    // Clear the array for the read back
    ezMemoryUtils::ZeroFill(uiData, 1024);

    uiBytesRead = StreamReader.ReadBytes(reinterpret_cast<ezUInt8*>(uiData), sizeof(ezUInt32) * 1024);

    EZ_TEST_BOOL(uiBytesRead == sizeof(ezUInt32) * 1024);

    const ezUInt32 uiHashAfterReading = ezHashingUtils::xxHash32(uiData, sizeof(ezUInt32) * 1024);

    EZ_TEST_BOOL(uiHashAfterReading == uiHashBeforeWriting);

    // Modify data and test the Rewind() functionality of the writer
    uiData[0] = 0x42;
    uiData[1] = 0x23;

    const ezUInt32 uiHashOfModifiedData = ezHashingUtils::xxHash32(uiData, sizeof(ezUInt32) * 4); // Only test the first 4 elements now

    StreamWriter.SetWritePosition(0);

    StreamWriter.WriteBytes(uiData, sizeof(ezUInt32) * 4).IgnoreResult();

    // Clear the array for the read back
    ezMemoryUtils::ZeroFill(uiData, 4);

    // Test the rewind of the reader as well
    StreamReader.SetReadPosition(0);

    uiBytesRead = StreamReader.ReadBytes(uiData, sizeof(ezUInt32) * 4);

    EZ_TEST_BOOL(uiBytesRead == sizeof(ezUInt32) * 4);

    const ezUInt32 uiHashAfterReadingOfModifiedData = ezHashingUtils::xxHash32(uiData, sizeof(ezUInt32) * 4);

    EZ_TEST_BOOL(uiHashAfterReadingOfModifiedData == uiHashOfModifiedData);

    // Test skipping
    StreamReader.SetReadPosition(0);

    StreamReader.SkipBytes(sizeof(ezUInt32));

    ezUInt32 uiTemp;

    uiBytesRead = StreamReader.ReadBytes(&uiTemp, sizeof(ezUInt32));

    EZ_TEST_BOOL(uiBytesRead == sizeof(ezUInt32));

    // We skipped over the first 0x42 element, so this should be 0x23
    EZ_TEST_BOOL(uiTemp == 0x23);

    // Skip more bytes than available
    ezUInt64 uiBytesSkipped = StreamReader.SkipBytes(0xFFFFFFFFFF);

    EZ_TEST_BOOL(uiBytesSkipped < 0xFFFFFFFFFF);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Raw Memory Stream Reading")
  {
    ezDynamicArray<ezUInt8> OrigStorage;
    OrigStorage.SetCountUninitialized(1000);

    for (ezUInt32 i = 0; i < 1000; ++i)
    {
      OrigStorage[i] = i % 256;
    }

    {
      ezRawMemoryStreamReader reader(OrigStorage);

      ezDynamicArray<ezUInt8> CopyStorage;
      CopyStorage.SetCountUninitialized(static_cast<ezUInt32>(reader.GetByteCount()));
      reader.ReadBytes(CopyStorage.GetData(), reader.GetByteCount());

      EZ_TEST_BOOL(OrigStorage == CopyStorage);
    }

    {
      ezRawMemoryStreamReader reader(OrigStorage.GetData() + 510, 490);

      ezDynamicArray<ezUInt8> CopyStorage;
      CopyStorage.SetCountUninitialized(static_cast<ezUInt32>(reader.GetByteCount()));
      reader.ReadBytes(CopyStorage.GetData(), reader.GetByteCount());

      EZ_TEST_BOOL(OrigStorage != CopyStorage);

      for (ezUInt32 i = 0; i < 490; ++i)
      {
        CopyStorage[i] = (i + 10) % 256;
      }
    }

    {
      ezRawMemoryStreamReader reader(OrigStorage.GetData(), 1000);
      reader.SkipBytes(510);

      ezDynamicArray<ezUInt8> CopyStorage;
      CopyStorage.SetCountUninitialized(490);
      reader.ReadBytes(CopyStorage.GetData(), 490);

      EZ_TEST_BOOL(OrigStorage != CopyStorage);

      for (ezUInt32 i = 0; i < 490; ++i)
      {
        CopyStorage[i] = (i + 10) % 256;
      }
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Raw Memory Stream Writing")
  {
    ezDynamicArray<ezUInt8> OrigStorage;
    OrigStorage.SetCountUninitialized(1000);

    ezRawMemoryStreamWriter writer0;
    EZ_TEST_INT(writer0.GetNumWrittenBytes(), 0);
    EZ_TEST_INT(writer0.GetStorageSize(), 0);

    ezRawMemoryStreamWriter writer(OrigStorage.GetData(), OrigStorage.GetCount());

    for (ezUInt32 i = 0; i < 1000; ++i)
    {
      writer << static_cast<ezUInt8>(i % 256);

      EZ_TEST_INT(writer.GetNumWrittenBytes(), i + 1);
      EZ_TEST_INT(writer.GetStorageSize(), 1000);
    }

    for (ezUInt32 i = 0; i < 1000; ++i)
    {
      EZ_TEST_INT(OrigStorage[i], i % 256);
    }

    {
      ezRawMemoryStreamWriter writer2(OrigStorage);
      EZ_TEST_INT(writer2.GetNumWrittenBytes(), 0);
      EZ_TEST_INT(writer2.GetStorageSize(), 1000);
    }
  }
}

EZ_CREATE_SIMPLE_TEST(IO, LargeMemoryStream)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Large Memory Stream Reading / Writing")
  {
    ezDefaultMemoryStreamStorage storage;
    ezMemoryStreamWriter writer(&storage);
    ezMemoryStreamReader reader(&storage);

    const ezUInt8 pattern[] = {11, 10, 27, 4, 14, 3, 21, 6};

    ezUInt64 uiSize = 0;
    constexpr ezUInt64 bytesToTest = 0x8000000llu; // tested with up to 8 GB, but that just takes too long

    // writes n gigabyte
    for (ezUInt32 n = 0; n < 8; ++n)
    {
      // writes one gigabyte
      for (ezUInt32 gb = 0; gb < 1024; ++gb)
      {
        // writes one megabyte
        for (ezUInt32 mb = 0; mb < 1024 * 1024 / EZ_ARRAY_SIZE(pattern); ++mb)
        {
          writer.WriteBytes(pattern, EZ_ARRAY_SIZE(pattern)).IgnoreResult();
          uiSize += EZ_ARRAY_SIZE(pattern);

          if (uiSize == bytesToTest)
            goto check;
        }
      }
    }

  check:
    EZ_TEST_BOOL(uiSize == bytesToTest);
    EZ_TEST_BOOL(writer.GetWritePosition() == bytesToTest);
    uiSize = 0;

    // reads n gigabyte
    for (ezUInt32 n = 0; n < 8; ++n)
    {
      // reads one gigabyte
      for (ezUInt32 gb = 0; gb < 1024; ++gb)
      {
        // reads one megabyte
        for (ezUInt32 mb = 0; mb < 1024 * 1024 / EZ_ARRAY_SIZE(pattern); ++mb)
        {
          ezUInt8 pattern2[EZ_ARRAY_SIZE(pattern)];

          const ezUInt64 uiRead = reader.ReadBytes(pattern2, EZ_ARRAY_SIZE(pattern));

          if (uiRead != EZ_ARRAY_SIZE(pattern))
          {
            EZ_TEST_BOOL(uiRead == 0);
            EZ_TEST_BOOL(uiSize == bytesToTest);
            goto endTest;
          }

          uiSize += uiRead;

          if (ezMemoryUtils::RawByteCompare(pattern, pattern2, EZ_ARRAY_SIZE(pattern)) != 0)
          {
            EZ_TEST_BOOL_MSG(false, "Memory read comparison failed.");
            goto endTest;
          }
        }
      }
    }

  endTest:;
    EZ_TEST_BOOL(reader.GetReadPosition() == bytesToTest);
  }
}
