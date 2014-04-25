#include <PCH.h>

#include <Foundation/Algorithm/HashableStruct.h>
#include <Foundation/Strings/HashedString.h>

EZ_CREATE_SIMPLE_TEST_GROUP(Algorithm);

EZ_CREATE_SIMPLE_TEST(Algorithm, Hashing)
{
  // check whether compile time hashing gives the same value as runtime hashing
  ezStringBuilder sb = "This is a test string. 1234";

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Hashfunction")
  {
    ezUInt32 uiHashRT = ezHashing::MurmurHash(sb.GetData());
    ezUInt32 uiHashCT = ezHashing::MurmurHash("This is a test string. 1234");

    EZ_TEST_INT(uiHashRT, 0xb999d6c4);
    EZ_TEST_INT(uiHashRT, uiHashCT);

    // check 64bit hashes
    ezUInt64 uiHash64 = ezHashing::MurmurHash64(sb.GetData(), sb.GetElementCount());
    EZ_TEST_INT(uiHash64, 0xf8ebc5e8cb110786);

    // test crc32
    ezUInt32 uiCrc = ezHashing::CRC32Hash(sb.GetData(), sb.GetElementCount());
    EZ_TEST_INT(uiCrc, 0x73b5e898);
  }
  
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "HashHelper")
  {
    ezUInt32 uiHash = ezHashHelper<ezStringBuilder>::Hash(sb);
    EZ_TEST_INT(uiHash, 0xb999d6c4);

    const char* szTest = "This is a test string. 1234";
    uiHash = ezHashHelper<const char*>::Hash(szTest);
    EZ_TEST_INT(uiHash, 0xb999d6c4);
    EZ_TEST_BOOL(ezHashHelper<const char*>::Equal(szTest, sb.GetData()));

    ezHashedString hs;
    hs.Assign(szTest);
    uiHash = ezHashHelper<ezHashedString>::Hash(hs);
    EZ_TEST_INT(uiHash, 0xb999d6c4);

    ezTempHashedString ths(szTest);
    uiHash = ezHashHelper<ezHashedString>::Hash(ths);
    EZ_TEST_INT(uiHash, 0xb999d6c4);
    EZ_TEST_BOOL(ezHashHelper<ezHashedString>::Equal(hs, ths));
  }
}

struct SimpleHashableStruct : public ezHashableStruct<SimpleHashableStruct>
{
  ezUInt32 m_uiTestMember1;
  ezUInt8 m_uiTestMember2;
  ezUInt64 m_uiTestMember3;
};

struct SimpleStruct
{
  ezUInt32 m_uiTestMember1;
  ezUInt8 m_uiTestMember2;
  ezUInt64 m_uiTestMember3;
};

EZ_CREATE_SIMPLE_TEST(Algorithm, HashableStruct)
{
  SimpleHashableStruct AutomaticInst;
  EZ_TEST_INT(AutomaticInst.m_uiTestMember1, 0);
  EZ_TEST_INT(AutomaticInst.m_uiTestMember2, 0);
  EZ_TEST_INT(AutomaticInst.m_uiTestMember3, 0);

  SimpleStruct NonAutomaticInst;
  ezMemoryUtils::ZeroFill(&NonAutomaticInst);

  EZ_CHECK_AT_COMPILETIME(sizeof(AutomaticInst) == sizeof(NonAutomaticInst));

  EZ_TEST_INT(ezMemoryUtils::ByteCompare<ezUInt8>((ezUInt8*)&AutomaticInst, (ezUInt8*)&NonAutomaticInst, sizeof(AutomaticInst)), 0);

  AutomaticInst.m_uiTestMember2 = 0x42u;
  AutomaticInst.m_uiTestMember3 = 0x23u;

  ezUInt32 uiAutomaticHash = AutomaticInst.CalculateHash();

  NonAutomaticInst.m_uiTestMember2 = 0x42u;
  NonAutomaticInst.m_uiTestMember3 = 0x23u;

  ezUInt32 uiNonAutomaticHash = ezHashing::CRC32Hash(&NonAutomaticInst, sizeof(NonAutomaticInst));

  EZ_TEST_INT(uiAutomaticHash, uiNonAutomaticHash);

  AutomaticInst.m_uiTestMember1 = 0x5u;
  uiAutomaticHash = AutomaticInst.CalculateHash();

  EZ_TEST_BOOL(uiAutomaticHash != uiNonAutomaticHash);
}