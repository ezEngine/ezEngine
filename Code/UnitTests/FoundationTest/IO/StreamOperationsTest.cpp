#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Containers/StaticArray.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/Stream.h>

#include <Foundation/IO/StringDeduplicationContext.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Time/Stopwatch.h>

namespace
{
  struct SerializableStructWithMethods
  {

    EZ_DECLARE_POD_TYPE();

    ezResult Serialize(ezStreamWriter& inout_stream) const
    {
      inout_stream << m_uiMember1;
      inout_stream << m_uiMember2;

      return EZ_SUCCESS;
    }

    ezResult Deserialize(ezStreamReader& inout_stream)
    {
      inout_stream >> m_uiMember1;
      inout_stream >> m_uiMember2;

      return EZ_SUCCESS;
    }

    ezInt32 m_uiMember1 = 0x42;
    ezInt32 m_uiMember2 = 0x23;
  };
} // namespace

EZ_CREATE_SIMPLE_TEST(IO, StreamOperation)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Binary Stream Basic Operations (built-in types)")
  {
    ezDefaultMemoryStreamStorage StreamStorage(4096);

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

    // Arrays
    {
      ezDynamicArray<ezUInt32> DynamicArray;
      DynamicArray.PushBack(42);
      DynamicArray.PushBack(23);
      DynamicArray.PushBack(13);
      DynamicArray.PushBack(5);
      DynamicArray.PushBack(0);

      StreamWriter.WriteArray(DynamicArray).IgnoreResult();
    }

    // Create reader
    ezMemoryStreamReader StreamReader(&StreamStorage);

    // Read back
    {
      ezUInt8 uiVal;
      StreamReader >> uiVal;
      EZ_TEST_BOOL(uiVal == (ezUInt8)0x42);
    }
    {
      ezUInt16 uiVal;
      StreamReader >> uiVal;
      EZ_TEST_BOOL(uiVal == (ezUInt16)0x4223);
    }
    {
      ezUInt32 uiVal;
      StreamReader >> uiVal;
      EZ_TEST_BOOL(uiVal == (ezUInt32)0x42232342);
    }
    {
      ezUInt64 uiVal;
      StreamReader >> uiVal;
      EZ_TEST_BOOL(uiVal == (ezUInt64)0x4223234242232342);
    }

    {
      float fVal;
      StreamReader >> fVal;
      EZ_TEST_BOOL(fVal == 42.0f);
    }
    {
      double dVal;
      StreamReader >> dVal;
      EZ_TEST_BOOL(dVal == 23.0f);
    }


    {
      ezInt8 iVal;
      StreamReader >> iVal;
      EZ_TEST_BOOL(iVal == (ezInt8)0x23);
    }
    {
      ezInt16 iVal;
      StreamReader >> iVal;
      EZ_TEST_BOOL(iVal == (ezInt16)0x2342);
    }
    {
      ezInt32 iVal;
      StreamReader >> iVal;
      EZ_TEST_BOOL(iVal == (ezInt32)0x23422342);
    }
    {
      ezInt64 iVal;
      StreamReader >> iVal;
      EZ_TEST_BOOL(iVal == (ezInt64)0x2342234242232342);
    }

    {
      ezDynamicArray<ezUInt32> ReadBackDynamicArray;

      // This element will be removed by the ReadArray function
      ReadBackDynamicArray.PushBack(0xAAu);

      StreamReader.ReadArray(ReadBackDynamicArray).IgnoreResult();

      EZ_TEST_INT(ReadBackDynamicArray.GetCount(), 5);

      EZ_TEST_INT(ReadBackDynamicArray[0], 42);
      EZ_TEST_INT(ReadBackDynamicArray[1], 23);
      EZ_TEST_INT(ReadBackDynamicArray[2], 13);
      EZ_TEST_INT(ReadBackDynamicArray[3], 5);
      EZ_TEST_INT(ReadBackDynamicArray[4], 0);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Binary Stream Arrays of Structs")
  {
    ezDefaultMemoryStreamStorage StreamStorage(4096);

    // Create writer
    ezMemoryStreamWriter StreamWriter(&StreamStorage);

    // Write out a couple of the structs
    {
      ezStaticArray<SerializableStructWithMethods, 16> WriteArray;
      WriteArray.ExpandAndGetRef().m_uiMember1 = 0x5;
      WriteArray.ExpandAndGetRef().m_uiMember1 = 0x6;

      StreamWriter.WriteArray(WriteArray).IgnoreResult();
    }

    // Read back in
    {
      // Create reader
      ezMemoryStreamReader StreamReader(&StreamStorage);

      // This intentionally uses a different array type for the read back
      // to verify that it is a) compatible and b) all arrays are somewhat tested
      ezHybridArray<SerializableStructWithMethods, 1> ReadArray;

      StreamReader.ReadArray(ReadArray).IgnoreResult();

      EZ_TEST_INT(ReadArray.GetCount(), 2);

      EZ_TEST_INT(ReadArray[0].m_uiMember1, 0x5);
      EZ_TEST_INT(ReadArray[0].m_uiMember2, 0x23);

      EZ_TEST_INT(ReadArray[1].m_uiMember1, 0x6);
      EZ_TEST_INT(ReadArray[1].m_uiMember2, 0x23);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezSet Stream Operators")
  {
    ezDefaultMemoryStreamStorage StreamStorage(4096);

    // Create writer
    ezMemoryStreamWriter StreamWriter(&StreamStorage);

    ezSet<ezString> TestSet;
    TestSet.Insert("Hello");
    TestSet.Insert("World");
    TestSet.Insert("!");

    StreamWriter.WriteSet(TestSet).IgnoreResult();

    ezSet<ezString> TestSetReadBack;

    TestSetReadBack.Insert("Shouldn't be there after deserialization.");

    ezMemoryStreamReader StreamReader(&StreamStorage);

    StreamReader.ReadSet(TestSetReadBack).IgnoreResult();

    EZ_TEST_INT(TestSetReadBack.GetCount(), 3);

    EZ_TEST_BOOL(TestSetReadBack.Contains("Hello"));
    EZ_TEST_BOOL(TestSetReadBack.Contains("!"));
    EZ_TEST_BOOL(TestSetReadBack.Contains("World"));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezMap Stream Operators")
  {
    ezDefaultMemoryStreamStorage StreamStorage(4096);

    // Create writer
    ezMemoryStreamWriter StreamWriter(&StreamStorage);

    ezMap<ezUInt64, ezString> TestMap;
    TestMap.Insert(42, "Hello");
    TestMap.Insert(23, "World");
    TestMap.Insert(5, "!");

    StreamWriter.WriteMap(TestMap).IgnoreResult();

    ezMap<ezUInt64, ezString> TestMapReadBack;

    TestMapReadBack.Insert(1, "Shouldn't be there after deserialization.");

    ezMemoryStreamReader StreamReader(&StreamStorage);

    StreamReader.ReadMap(TestMapReadBack).IgnoreResult();

    EZ_TEST_INT(TestMapReadBack.GetCount(), 3);

    EZ_TEST_BOOL(TestMapReadBack.Contains(42));
    EZ_TEST_BOOL(TestMapReadBack.Contains(5));
    EZ_TEST_BOOL(TestMapReadBack.Contains(23));

    EZ_TEST_BOOL(TestMapReadBack.GetValue(42)->IsEqual("Hello"));
    EZ_TEST_BOOL(TestMapReadBack.GetValue(5)->IsEqual("!"));
    EZ_TEST_BOOL(TestMapReadBack.GetValue(23)->IsEqual("World"));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezHashTable Stream Operators")
  {
    ezDefaultMemoryStreamStorage StreamStorage(4096);

    // Create writer
    ezMemoryStreamWriter StreamWriter(&StreamStorage);

    ezHashTable<ezUInt64, ezString> TestHashTable;
    TestHashTable.Insert(42, "Hello");
    TestHashTable.Insert(23, "World");
    TestHashTable.Insert(5, "!");

    StreamWriter.WriteHashTable(TestHashTable).IgnoreResult();

    ezMap<ezUInt64, ezString> TestHashTableReadBack;

    TestHashTableReadBack.Insert(1, "Shouldn't be there after deserialization.");

    ezMemoryStreamReader StreamReader(&StreamStorage);

    StreamReader.ReadMap(TestHashTableReadBack).IgnoreResult();

    EZ_TEST_INT(TestHashTableReadBack.GetCount(), 3);

    EZ_TEST_BOOL(TestHashTableReadBack.Contains(42));
    EZ_TEST_BOOL(TestHashTableReadBack.Contains(5));
    EZ_TEST_BOOL(TestHashTableReadBack.Contains(23));

    EZ_TEST_BOOL(TestHashTableReadBack.GetValue(42)->IsEqual("Hello"));
    EZ_TEST_BOOL(TestHashTableReadBack.GetValue(5)->IsEqual("!"));
    EZ_TEST_BOOL(TestHashTableReadBack.GetValue(23)->IsEqual("World"));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "String Deduplication")
  {
    ezDefaultMemoryStreamStorage StreamStorageNonDeduplicated(4096);
    ezDefaultMemoryStreamStorage StreamStorageDeduplicated(4096);

    ezHybridString<4> str1 = "Hello World";
    ezDynamicString str2 = "Hello World 2";
    ezStringBuilder str3 = "Hello Schlumpf";

    // Non deduplicated serialization
    {
      ezMemoryStreamWriter StreamWriter(&StreamStorageNonDeduplicated);

      StreamWriter << str1;
      StreamWriter << str2;
      StreamWriter << str1;
      StreamWriter << str3;
      StreamWriter << str1;
      StreamWriter << str2;
    }

    // Deduplicated serialization
    {
      ezMemoryStreamWriter StreamWriter(&StreamStorageDeduplicated);

      ezStringDeduplicationWriteContext StringDeduplicationContext(StreamWriter);
      auto& DeduplicationWriter = StringDeduplicationContext.Begin();

      DeduplicationWriter << str1;
      DeduplicationWriter << str2;
      DeduplicationWriter << str1;
      DeduplicationWriter << str3;
      DeduplicationWriter << str1;
      DeduplicationWriter << str2;

      StringDeduplicationContext.End().IgnoreResult();

      EZ_TEST_INT(StringDeduplicationContext.GetUniqueStringCount(), 3);
    }

    EZ_TEST_BOOL(StreamStorageDeduplicated.GetStorageSize64() < StreamStorageNonDeduplicated.GetStorageSize64());

    // Read the deduplicated strings back
    {
      ezMemoryStreamReader StreamReader(&StreamStorageDeduplicated);

      ezStringDeduplicationReadContext StringDeduplicationReadContext(StreamReader);

      ezHybridString<16> szRead0, szRead1, szRead2;
      ezStringBuilder szRead3, szRead4, szRead5;

      StreamReader >> szRead0;
      StreamReader >> szRead1;
      StreamReader >> szRead2;
      StreamReader >> szRead3;
      StreamReader >> szRead4;
      StreamReader >> szRead5;

      EZ_TEST_STRING(szRead0, szRead2);
      EZ_TEST_STRING(szRead0, szRead4);
      EZ_TEST_STRING(szRead1, szRead5);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Array Serialization Performance (bytes)")
  {
    constexpr ezUInt32 uiCount = 1024 * 1024 * 10;

    ezContiguousMemoryStreamStorage storage(uiCount + 16);

    ezMemoryStreamWriter writer(&storage);
    ezMemoryStreamReader reader(&storage);

    ezDynamicArray<ezUInt8> DynamicArray;
    DynamicArray.SetCountUninitialized(uiCount);

    for (ezUInt32 i = 0; i < uiCount; ++i)
    {
      DynamicArray[i] = i & 0xFF;
    }

    {
      ezStopwatch sw;

      writer.WriteArray(DynamicArray).AssertSuccess();

      ezTime t = sw.GetRunningTotal();
      ezStringBuilder s;
      s.SetFormat("Write {} byte array: {}", ezArgFileSize(uiCount), t);
      ezTestFramework::Output(ezTestOutput::Details, s);
    }

    {
      ezStopwatch sw;

      reader.ReadArray(DynamicArray).IgnoreResult();

      ezTime t = sw.GetRunningTotal();
      ezStringBuilder s;
      s.SetFormat("Read {} byte array: {}", ezArgFileSize(uiCount), t);
      ezTestFramework::Output(ezTestOutput::Details, s);
    }

    for (ezUInt32 i = 0; i < uiCount; ++i)
    {
      EZ_TEST_INT(DynamicArray[i], i & 0xFF);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Array Serialization Performance (ezVec3)")
  {
    constexpr ezUInt32 uiCount = 1024 * 1024 * 10;

    ezContiguousMemoryStreamStorage storage(uiCount * sizeof(ezVec3) + 16);

    ezMemoryStreamWriter writer(&storage);
    ezMemoryStreamReader reader(&storage);

    ezDynamicArray<ezVec3> DynamicArray;
    DynamicArray.SetCountUninitialized(uiCount);

    for (ezUInt32 i = 0; i < uiCount; ++i)
    {
      DynamicArray[i].Set(i, i + 1, i + 2);
    }

    {
      ezStopwatch sw;

      writer.WriteArray(DynamicArray).AssertSuccess();

      ezTime t = sw.GetRunningTotal();
      ezStringBuilder s;
      s.SetFormat("Write {} vec3 array: {}", ezArgFileSize(uiCount * sizeof(ezVec3)), t);
      ezTestFramework::Output(ezTestOutput::Details, s);
    }

    {
      ezStopwatch sw;

      reader.ReadArray(DynamicArray).AssertSuccess();

      ezTime t = sw.GetRunningTotal();
      ezStringBuilder s;
      s.SetFormat("Read {} vec3 array: {}", ezArgFileSize(uiCount * sizeof(ezVec3)), t);
      ezTestFramework::Output(ezTestOutput::Details, s);
    }

    for (ezUInt32 i = 0; i < uiCount; ++i)
    {
      EZ_TEST_VEC3(DynamicArray[i], ezVec3(i, i + 1, i + 2), 0.01f);
    }
  }
}
