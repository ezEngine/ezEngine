#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Types/Uuid.h>


EZ_CREATE_SIMPLE_TEST(Basics, Uuid)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Uuid Generation")
  {
    ezUuid ShouldBeInvalid;

    EZ_TEST_BOOL(ShouldBeInvalid.IsValid() == false);

    ezUuid FirstGenerated = ezUuid::MakeUuid();
    EZ_TEST_BOOL(FirstGenerated.IsValid());

    ezUuid SecondGenerated = ezUuid::MakeUuid();
    EZ_TEST_BOOL(SecondGenerated.IsValid());

    EZ_TEST_BOOL(!(FirstGenerated == SecondGenerated));
    EZ_TEST_BOOL(FirstGenerated != SecondGenerated);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Uuid Serialization")
  {
    ezUuid Uuid;
    EZ_TEST_BOOL(Uuid.IsValid() == false);

    Uuid = ezUuid::MakeUuid();
    EZ_TEST_BOOL(Uuid.IsValid());

    ezDefaultMemoryStreamStorage StreamStorage;

    // Create reader
    ezMemoryStreamReader StreamReader(&StreamStorage);

    // Create writer
    ezMemoryStreamWriter StreamWriter(&StreamStorage);

    StreamWriter << Uuid;

    ezUuid ReadBack;
    EZ_TEST_BOOL(ReadBack.IsValid() == false);

    StreamReader >> ReadBack;

    EZ_TEST_BOOL(ReadBack == Uuid);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Stable Uuid From String")
  {
    ezUuid uuid1 = ezUuid::MakeStableUuidFromString("TEST 1");
    ezUuid uuid2 = ezUuid::MakeStableUuidFromString("TEST 2");
    ezUuid uuid3 = ezUuid::MakeStableUuidFromString("TEST 1");

    EZ_TEST_BOOL(uuid1 == uuid3);
    EZ_TEST_BOOL(uuid1 != uuid2);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Uuid Combine")
  {
    ezUuid uuid1 = ezUuid::MakeUuid();
    ezUuid uuid2 = ezUuid::MakeUuid();
    ezUuid combined = uuid1;
    combined.CombineWithSeed(uuid2);
    EZ_TEST_BOOL(combined != uuid1);
    EZ_TEST_BOOL(combined != uuid2);
    combined.RevertCombinationWithSeed(uuid2);
    EZ_TEST_BOOL(combined == uuid1);

    ezUuid hashA = uuid1;
    hashA.HashCombine(uuid2);
    ezUuid hashB = uuid2;
    hashA.HashCombine(uuid1);
    EZ_TEST_BOOL(hashA != hashB);
  }
}
