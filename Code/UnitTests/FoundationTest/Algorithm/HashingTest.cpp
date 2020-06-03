#include <FoundationTestPCH.h>

#include <Foundation/Algorithm/HashableStruct.h>
#include <Foundation/Algorithm/HashHelperString.h>
#include <Foundation/Algorithm/HashStream.h>
#include <Foundation/Strings/HashedString.h>

EZ_CREATE_SIMPLE_TEST_GROUP(Algorithm);

// Warning for overflow in compile time executed static_assert(ezHashingUtils::MurmurHash32...)
// Todo: Why is this not happening elsewhere?
#pragma warning (disable:4307)

EZ_CREATE_SIMPLE_TEST(Algorithm, Hashing)
{
  // check whether compile time hashing gives the same value as runtime hashing
  const char* szString = "This is a test string. 1234";
  const char* szStringLower = "this is a test string. 1234";
  const char* szString2 = "THiS iS A TESt sTrInG. 1234";
  ezStringBuilder sb = szString;

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Hashfunction")
  {
    ezUInt32 uiHashRT = ezHashingUtils::MurmurHash32String(sb.GetData());
    ezUInt32 uiHashCT = ezHashingUtils::MurmurHash32String("This is a test string. 1234");


    EZ_TEST_INT(uiHashRT, 0xb999d6c4);
    EZ_TEST_INT(uiHashRT, uiHashCT);

    // Static assert to ensure this is happening at compile time!
    static_assert(ezHashingUtils::MurmurHash32String("This is a test string. 1234") == static_cast<ezUInt32>(0xb999d6c4), "Error in compile time murmur hash calculation!");

    // check 64bit hashes
    const ezUInt64 uiMurmurHash64 = ezHashingUtils::MurmurHash64(sb.GetData(), sb.GetElementCount());
    EZ_TEST_INT(uiMurmurHash64, 0xf8ebc5e8cb110786);

    // test crc32
    const ezUInt32 uiCrc32 = ezHashingUtils::CRC32Hash(sb.GetData(), sb.GetElementCount());
    EZ_TEST_INT(uiCrc32, 0x73b5e898);

    // 32 Bit xxHash
    const ezUInt32 uiXXHash32 = ezHashingUtils::xxHash32(sb.GetData(), sb.GetElementCount());
    EZ_TEST_INT(uiXXHash32, 0xff35b049);

    // 64 Bit xxHash
    const ezUInt64 uiXXHash64 = ezHashingUtils::xxHash64(sb.GetData(), sb.GetElementCount());
    EZ_TEST_INT(uiXXHash64, 0x141fb89c0bf32020);

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

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "HashHelperString_NoCase")
  {
    const ezUInt32 uiHash = ezHashHelper<const char*>::Hash(szStringLower);
    EZ_TEST_INT(uiHash, 0x756cc03e);
    EZ_TEST_INT(uiHash, ezHashHelperString_NoCase::Hash(szString));
    EZ_TEST_INT(uiHash, ezHashHelperString_NoCase::Hash(szStringLower));
    EZ_TEST_INT(uiHash, ezHashHelperString_NoCase::Hash(szString2));
    EZ_TEST_INT(uiHash, ezHashHelperString_NoCase::Hash(sb));
    ezStringBuilder sb2 = szString2;
    EZ_TEST_INT(uiHash, ezHashHelperString_NoCase::Hash(sb2));
    ezString sL = szStringLower;
    ezString s1 = sb;
    ezString s2 = sb2;
    EZ_TEST_INT(uiHash, ezHashHelperString_NoCase::Hash(s1));
    EZ_TEST_INT(uiHash, ezHashHelperString_NoCase::Hash(s2));
    ezStringView svL = szStringLower;
    ezStringView sv1 = szString;
    ezStringView sv2 = szString2;
    EZ_TEST_INT(uiHash, ezHashHelperString_NoCase::Hash(svL));
    EZ_TEST_INT(uiHash, ezHashHelperString_NoCase::Hash(sv1));
    EZ_TEST_INT(uiHash, ezHashHelperString_NoCase::Hash(sv2));

    EZ_TEST_BOOL(ezHashHelperString_NoCase::Equal(sb, sb2));
    EZ_TEST_BOOL(ezHashHelperString_NoCase::Equal(sb, szString2));
    EZ_TEST_BOOL(ezHashHelperString_NoCase::Equal(sb, sv2));
    EZ_TEST_BOOL(ezHashHelperString_NoCase::Equal(s1, sb2));
    EZ_TEST_BOOL(ezHashHelperString_NoCase::Equal(s1, szString2));
    EZ_TEST_BOOL(ezHashHelperString_NoCase::Equal(s1, sv2));
    EZ_TEST_BOOL(ezHashHelperString_NoCase::Equal(sv1, sb2));
    EZ_TEST_BOOL(ezHashHelperString_NoCase::Equal(sv1, szString2));
    EZ_TEST_BOOL(ezHashHelperString_NoCase::Equal(sv1, sv2));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "HashStream32")
  {
    const char* szTest = "This is a test string. 1234";
    const char* szTestHalf1 = "This is a test";
    const char* szTestHalf2 = " string. 1234";

    auto test = [szTest, szTestHalf1, szTestHalf2](bool flush, ezUInt32* outHash) {
      ezHashStreamWriter32 writer1;
      writer1.WriteBytes(szTest, std::strlen(szTest));
      if (flush) writer1.Flush();
      const ezUInt32 uiHash1 = writer1.GetHashValue();

      ezHashStreamWriter32 writer2;
      writer2.WriteBytes(szTestHalf1, std::strlen(szTestHalf1));
      if (flush) writer2.Flush();
      writer2.WriteBytes(szTestHalf2, std::strlen(szTestHalf2));
      if (flush) writer2.Flush();
      const ezUInt32 uiHash2 = writer2.GetHashValue();

      ezHashStreamWriter32 writer3;
      for (ezUInt64 i = 0; szTest[i] != 0; ++i)
      {
        writer3.WriteBytes(szTest + i, 1);
        if (flush) writer3.Flush();
      }
      const ezUInt32 uiHash3 = writer3.GetHashValue();

      EZ_TEST_INT(uiHash1, uiHash2);
      EZ_TEST_INT(uiHash1, uiHash3);

      *outHash = uiHash1;
    };

    ezUInt32 uiHash1 = 0, uiHash2 = 1;
    test(true, &uiHash1);
    test(false, &uiHash2);
    EZ_TEST_INT(uiHash1, uiHash2);

    const ezUInt64 uiHash3 = ezHashingUtils::xxHash32(szTest, std::strlen(szTest));
    EZ_TEST_INT(uiHash1, uiHash3);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "HashStream64")
  {
    const char* szTest = "This is a test string. 1234";
    const char* szTestHalf1 = "This is a test";
    const char* szTestHalf2 = " string. 1234";

    auto test = [szTest, szTestHalf1, szTestHalf2](bool flush, ezUInt64* outHash) {
      ezHashStreamWriter64 writer1;
      writer1.WriteBytes(szTest, std::strlen(szTest));
      if (flush) writer1.Flush();
      const ezUInt64 uiHash1 = writer1.GetHashValue();

      ezHashStreamWriter64 writer2;
      writer2.WriteBytes(szTestHalf1, std::strlen(szTestHalf1));
      if (flush) writer2.Flush();
      writer2.WriteBytes(szTestHalf2, std::strlen(szTestHalf2));
      if (flush) writer2.Flush();
      const ezUInt64 uiHash2 = writer2.GetHashValue();

      ezHashStreamWriter64 writer3;
      for (ezUInt64 i = 0; szTest[i] != 0; ++i)
      {
        writer3.WriteBytes(szTest + i, 1);
        if (flush) writer3.Flush();
      }
      const ezUInt64 uiHash3 = writer3.GetHashValue();

      EZ_TEST_INT(uiHash1, uiHash2);
      EZ_TEST_INT(uiHash1, uiHash3);

      *outHash = uiHash1;
    };

    ezUInt64 uiHash1 = 0, uiHash2 = 1;
    test(true, &uiHash1);
    test(false, &uiHash2);
    EZ_TEST_INT(uiHash1, uiHash2);

    const ezUInt64 uiHash3 = ezHashingUtils::xxHash64(szTest, std::strlen(szTest));
    EZ_TEST_INT(uiHash1, uiHash3);
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
  ezMemoryUtils::ZeroFill(&NonAutomaticInst, 1);

  EZ_CHECK_AT_COMPILETIME(sizeof(AutomaticInst) == sizeof(NonAutomaticInst));

  EZ_TEST_INT(ezMemoryUtils::Compare<ezUInt8>((ezUInt8*)&AutomaticInst, (ezUInt8*)&NonAutomaticInst, sizeof(AutomaticInst)), 0);

  AutomaticInst.m_uiTestMember2 = 0x42u;
  AutomaticInst.m_uiTestMember3 = 0x23u;

  ezUInt32 uiAutomaticHash = AutomaticInst.CalculateHash();

  NonAutomaticInst.m_uiTestMember2 = 0x42u;
  NonAutomaticInst.m_uiTestMember3 = 0x23u;

  ezUInt32 uiNonAutomaticHash = ezHashingUtils::xxHash32(&NonAutomaticInst, sizeof(NonAutomaticInst));

  EZ_TEST_INT(uiAutomaticHash, uiNonAutomaticHash);

  AutomaticInst.m_uiTestMember1 = 0x5u;
  uiAutomaticHash = AutomaticInst.CalculateHash();

  EZ_TEST_BOOL(uiAutomaticHash != uiNonAutomaticHash);
}
