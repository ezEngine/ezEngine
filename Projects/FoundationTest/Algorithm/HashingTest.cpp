#include <PCH.h>

#include <Foundation/Algorithm/HashableStruct.h>

EZ_CREATE_SIMPLE_TEST_GROUP(Algorithm);

EZ_CREATE_SIMPLE_TEST(Algorithm, Hashing)
{
  // check whether compile time hashing gives the same value as runtime hashing
  const char* szTest = "This is a test string. 1234";

  ezUInt32 uiHashRT = ezHashing::MurmurHash(szTest);
  ezUInt32 uiHashCT = ezHashing::MurmurHash("This is a test string. 1234");

  EZ_TEST_INT(uiHashRT, 0xb999d6c4);
  EZ_TEST_INT(uiHashRT, uiHashCT);

  // check 64bit hashes
  ezUInt64 uiHash64 = ezHashing::MurmurHash64(szTest, std::strlen(szTest));
  EZ_TEST_INT(uiHash64, 0xf8ebc5e8cb110786);

  // test crc32
  ezUInt32 uiCrc = ezHashing::CRC32Hash(szTest, std::strlen(szTest));
  EZ_TEST_INT(uiCrc, 0x73b5e898);
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