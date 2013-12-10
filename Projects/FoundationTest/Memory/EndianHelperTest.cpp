#include <PCH.h>
#include <Foundation/Memory/EndianHelper.h>

namespace
{
  struct TempStruct
  {
    float fVal;
    ezUInt32 uiDVal;
    ezUInt16 uiWVal1; ezUInt16 uiWVal2;
    char pad[4];
  };
  
  struct FloatAndInt
  {
    union
    {
      float fVal;
      ezUInt32 uiVal;
    };
  };
}


EZ_CREATE_SIMPLE_TEST(Memory, Endian)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Basics")
  {
    // Test if the IsBigEndian() delivers the same result as the #define
    #if EZ_ENABLED(EZ_PLATFORM_LITTLE_ENDIAN)
      EZ_TEST_BOOL(!ezEndianHelper::IsBigEndian());
    #elif EZ_ENABLED(EZ_PLATFORM_BIG_ENDIAN)
      EZ_TEST_BOOL(ezEndianHelper::IsBigEndian());
    #endif

    // Test conversion functions for single elements
    EZ_TEST_BOOL(ezEndianHelper::Switch(static_cast<ezUInt16>(0x15FF)) == 0xFF15);
    EZ_TEST_BOOL(ezEndianHelper::Switch(static_cast<ezUInt32>(0x34AA12FF)) == 0xFF12AA34);
    EZ_TEST_BOOL(ezEndianHelper::Switch(static_cast<ezUInt64>(0x34AA12FFABC3421E)) == 0x1E42C3ABFF12AA34);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Switching Arrays")
  {
    ezArrayPtr<ezUInt16> p16BitArray = EZ_DEFAULT_NEW_ARRAY(ezUInt16, 1024);
    ezArrayPtr<ezUInt16> p16BitArrayCopy = EZ_DEFAULT_NEW_ARRAY(ezUInt16, 1024);

    ezArrayPtr<ezUInt32> p32BitArray = EZ_DEFAULT_NEW_ARRAY(ezUInt32, 1024);
    ezArrayPtr<ezUInt32> p32BitArrayCopy = EZ_DEFAULT_NEW_ARRAY(ezUInt32, 1024);

    ezArrayPtr<ezUInt64> p64BitArray = EZ_DEFAULT_NEW_ARRAY(ezUInt64, 1024);
    ezArrayPtr<ezUInt64> p64BitArrayCopy = EZ_DEFAULT_NEW_ARRAY(ezUInt64, 1024);

    for (ezUInt32 i = 0; i < 1024; i++)
    {
      ezInt32 iRand = rand();
      p16BitArray[i] = static_cast<ezUInt16>(iRand);
      p32BitArray[i] = static_cast<ezUInt32>(iRand);
      p64BitArray[i] = static_cast<ezUInt64>(iRand | static_cast<ezUInt64>((iRand % 3)) << 32);
    }

    p16BitArrayCopy.CopyFrom(p16BitArray);
    p32BitArrayCopy.CopyFrom(p32BitArray);
    p64BitArrayCopy.CopyFrom(p64BitArray);

    ezEndianHelper::SwitchWords(p16BitArray.GetPtr(), 1024);
    ezEndianHelper::SwitchDWords(p32BitArray.GetPtr(), 1024);
    ezEndianHelper::SwitchQWords(p64BitArray.GetPtr(), 1024);

    for (ezUInt32 i = 0; i < 1024; i++)
    {
      EZ_TEST_BOOL(p16BitArray[i] == ezEndianHelper::Switch(p16BitArrayCopy[i]));
      EZ_TEST_BOOL(p32BitArray[i] == ezEndianHelper::Switch(p32BitArrayCopy[i]));
      EZ_TEST_BOOL(p64BitArray[i] == ezEndianHelper::Switch(p64BitArrayCopy[i]));

      // Test in place switcher
      ezEndianHelper::SwitchInPlace(&p16BitArrayCopy[i]);
      EZ_TEST_BOOL(p16BitArray[i] == p16BitArrayCopy[i]);

      ezEndianHelper::SwitchInPlace(&p32BitArrayCopy[i]);
      EZ_TEST_BOOL(p32BitArray[i] == p32BitArrayCopy[i]);

      ezEndianHelper::SwitchInPlace(&p64BitArrayCopy[i]);
      EZ_TEST_BOOL(p64BitArray[i] == p64BitArrayCopy[i]);
    }


    EZ_DEFAULT_DELETE_ARRAY(p16BitArray);
    EZ_DEFAULT_DELETE_ARRAY(p16BitArrayCopy);

    EZ_DEFAULT_DELETE_ARRAY(p32BitArray);
    EZ_DEFAULT_DELETE_ARRAY(p32BitArrayCopy);

    EZ_DEFAULT_DELETE_ARRAY(p64BitArray);
    EZ_DEFAULT_DELETE_ARRAY(p64BitArrayCopy);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Switching Structs")
  {
    TempStruct instance = { 42.0f, 0x34AA12FF, 0x15FF, 0x23FF, {'E', 'Z', 'F', 'T'} };

    ezEndianHelper::SwitchStruct(&instance, "ddwwcccc");

    ezIntFloatUnion floatHelper;
    floatHelper.f = 42.0f;

    ezIntFloatUnion floatHelper2;
    floatHelper2.f = instance.fVal;

    EZ_TEST_BOOL(floatHelper2.i == ezEndianHelper::Switch(floatHelper.i));
    EZ_TEST_BOOL(instance.uiDVal == ezEndianHelper::Switch(static_cast<ezUInt32>(0x34AA12FF)));
    EZ_TEST_BOOL(instance.uiWVal1 == ezEndianHelper::Switch(static_cast<ezUInt16>(0x15FF)));
    EZ_TEST_BOOL(instance.uiWVal2 == ezEndianHelper::Switch(static_cast<ezUInt16>(0x23FF)));
    EZ_TEST_BOOL(instance.pad[0] == 'E');
    EZ_TEST_BOOL(instance.pad[1] == 'Z');
    EZ_TEST_BOOL(instance.pad[2] == 'F');
    EZ_TEST_BOOL(instance.pad[3] == 'T');
  }
}

