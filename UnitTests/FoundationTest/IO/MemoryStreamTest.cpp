#include <PCH.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Algorithm/Hashing.h>

EZ_CREATE_SIMPLE_TEST_GROUP(IO);

EZ_CREATE_SIMPLE_TEST(IO, MemoryStream)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Memory Stream Reading / Writing")
  {
    ezMemoryStreamStorage StreamStorage;

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
    const ezUInt32 uiHashBeforeWriting = ezHashing::CRC32Hash(uiData, sizeof(ezUInt32) * 1024);

    // Write the data
    EZ_TEST_BOOL(StreamWriter.WriteBytes(reinterpret_cast<const ezUInt8*>(uiData), sizeof(ezUInt32) * 1024) == EZ_SUCCESS);

    EZ_TEST_BOOL(StreamWriter.GetByteCount() == sizeof(ezUInt32) * 1024);
    EZ_TEST_BOOL(StreamWriter.GetByteCount() == StreamReader.GetByteCount());


    // Clear the array for the read back
    ezMemoryUtils::ZeroFill(uiData, 1024);

    uiBytesRead = StreamReader.ReadBytes(reinterpret_cast<ezUInt8*>(uiData), sizeof(ezUInt32) * 1024);

    EZ_TEST_BOOL(uiBytesRead == sizeof(ezUInt32) * 1024);
    
    const ezUInt32 uiHashAfterReading = ezHashing::CRC32Hash(uiData, sizeof(ezUInt32) * 1024);

    EZ_TEST_BOOL(uiHashAfterReading == uiHashBeforeWriting);

    // Modify data and test the Rewind() functionality of the writer
    uiData[0] = 0x42;
    uiData[1] = 0x23;

    const ezUInt32 uiHashOfModifiedData = ezHashing::CRC32Hash(uiData, sizeof(ezUInt32) * 4); // Only test the first 4 elements now

    StreamWriter.SetWritePosition(0);

    StreamWriter.WriteBytes(uiData, sizeof(ezUInt32) * 4);

    // Clear the array for the read back
    ezMemoryUtils::ZeroFill(uiData, 4);

    // Test the rewind of the reader as well
    StreamReader.SetReadPosition(0);

    uiBytesRead = StreamReader.ReadBytes(uiData, sizeof(ezUInt32) * 4);

    EZ_TEST_BOOL(uiBytesRead == sizeof(ezUInt32) * 4);

    const ezUInt32 uiHashAfterReadingOfModifiedData = ezHashing::CRC32Hash(uiData, sizeof(ezUInt32) * 4);

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
}
