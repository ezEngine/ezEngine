#include <CoreTestPCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <Foundation/IO/MemoryStream.h>

EZ_CREATE_SIMPLE_TEST_GROUP(Assets);

EZ_CREATE_SIMPLE_TEST(Assets, AssetFileHeader)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetSerializedSize")
  {
    // Serialize a dummy header to get the size it takes to store it.
    ezAssetFileHeader temp;
    temp.SetFileHashAndVersion(123, 1);
    ezMemoryStreamStorage storage;
    ezMemoryStreamWriter writer(&storage);
    temp.Write(writer);
    const ezUInt16 uiRequiredSize = (ezUInt16)storage.GetStorageSize();

    EZ_TEST_INT(uiRequiredSize, ezAssetFileHeader::GetSerializedSize());
  }
}
