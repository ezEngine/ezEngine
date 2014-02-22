#include <PCH.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/IO/MemoryStream.h>

EZ_CREATE_SIMPLE_TEST(IO, StreamOperation)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Binary Stream Basic Operations (built-in types)")
  {
    ezMemoryStreamStorage StreamStorage(4096);

    // Create writer
    ezMemoryStreamWriter StreamWriter(&StreamStorage);

    StreamWriter << (ezUInt8)0x42;
    StreamWriter << (ezUInt16)0x4223;
    StreamWriter << (ezUInt32)0x42232342;
    StreamWriter << (ezUInt64)0x4223234242232342;
    StreamWriter << 42.0f;
    StreamWriter << 23.0;
    StreamWriter << (ezInt8)0x23;
    StreamWriter << (ezInt16)0x2342;
    StreamWriter << (ezInt32)0x23422342;
    StreamWriter << (ezInt64)0x2342234242232342;

    // Create reader
    ezMemoryStreamReader StreamReader(&StreamStorage);

    // Read back
    {
      ezUInt8 uiVal; StreamReader >> uiVal; EZ_TEST_BOOL(uiVal == (ezUInt8)0x42);
    }
    {
      ezUInt16 uiVal; StreamReader >> uiVal; EZ_TEST_BOOL(uiVal == (ezUInt16)0x4223);
    }
    {
      ezUInt32 uiVal; StreamReader >> uiVal; EZ_TEST_BOOL(uiVal == (ezUInt32)0x42232342);
    }
    {
      ezUInt64 uiVal; StreamReader >> uiVal; EZ_TEST_BOOL(uiVal == (ezUInt64)0x4223234242232342);
    }

    {
      float fVal; StreamReader >> fVal; EZ_TEST_BOOL(fVal == 42.0f);
    }
    {
      double dVal; StreamReader >> dVal; EZ_TEST_BOOL(dVal == 23.0f);
    }


    {
      ezInt8 iVal; StreamReader >> iVal; EZ_TEST_BOOL(iVal == (ezInt8)0x23);
    }
    {
      ezInt16 iVal; StreamReader >> iVal; EZ_TEST_BOOL(iVal == (ezInt16)0x2342);
    }
    {
      ezInt32 iVal; StreamReader >> iVal; EZ_TEST_BOOL(iVal == (ezInt32)0x23422342);
    }
    {
      ezInt64 iVal; StreamReader >> iVal; EZ_TEST_BOOL(iVal == (ezInt64)0x2342234242232342);
    }
  }

}