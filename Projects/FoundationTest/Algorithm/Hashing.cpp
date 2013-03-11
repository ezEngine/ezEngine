#include <TestFramework/Framework/TestFramework.h>
#include <Foundation/Algorithm/Hashing.h>

EZ_CREATE_SIMPLE_TEST_GROUP(Algorithm);

EZ_CREATE_SIMPLE_TEST(Algorithm, Hashing)
{
  // check whether compile time hashing gives the same value as runtime hashing
  const char* szTest = "This is a test string. 1234";

  ezUInt32 uiHashRT = ezHashing::MurmurHash(szTest);
  ezUInt32 uiHashCT = ezHashing::MurmurHash("This is a test string. 1234");

  EZ_TEST_INT(uiHashRT, uiHashCT);

  // check 64bit hashes
  ezUInt64 uiHash64 = ezHashing::MurmurHash64(szTest, std::strlen(szTest));
  EZ_TEST_INT(uiHash64, 0xf8ebc5e8cb110786);
}
