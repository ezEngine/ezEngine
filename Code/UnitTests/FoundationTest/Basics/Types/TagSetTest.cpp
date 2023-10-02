#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Types/Tag.h>
#include <Foundation/Types/TagRegistry.h>
#include <Foundation/Types/TagSet.h>

static_assert(sizeof(ezTagSet) == 16);

#if EZ_ENABLED(EZ_PLATFORM_64BIT)
static_assert(sizeof(ezTag) == 16);
#else
static_assert(sizeof(ezTag) == 12);
#endif

EZ_CREATE_SIMPLE_TEST(Basics, TagSet)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Basic Tag Tests")
  {
    ezTagRegistry TempTestRegistry;

    {
      ezTag TestTag;
      EZ_TEST_BOOL(!TestTag.IsValid());
    }

    ezHashedString TagName;
    TagName.Assign("BASIC_TAG_TEST");

    const ezTag& SecondInstance = TempTestRegistry.RegisterTag(TagName);
    EZ_TEST_BOOL(SecondInstance.IsValid());

    const ezTag* SecondInstance2 = TempTestRegistry.GetTagByName("BASIC_TAG_TEST");

    if (EZ_TEST_BOOL(SecondInstance2 != nullptr))
    {
      EZ_ANALYSIS_ASSUME(SecondInstance2 != nullptr);
      EZ_TEST_BOOL(SecondInstance2->IsValid());

      EZ_TEST_BOOL(&SecondInstance == SecondInstance2);

      EZ_TEST_STRING(SecondInstance2->GetTagString(), "BASIC_TAG_TEST");
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Basic Tag Registration")
  {
    ezTagRegistry TempTestRegistry;

    ezTag TestTag;

    EZ_TEST_BOOL(!TestTag.IsValid());

    EZ_TEST_BOOL(TempTestRegistry.GetTagByName("TEST_TAG1") == nullptr);

    TestTag = TempTestRegistry.RegisterTag("TEST_TAG1");

    EZ_TEST_BOOL(TestTag.IsValid());

    EZ_TEST_BOOL(TempTestRegistry.GetTagByName("TEST_TAG1") != nullptr);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Basic Tag Work")
  {
    ezTagRegistry TempTestRegistry;

    TempTestRegistry.RegisterTag("TEST_TAG1");

    const ezTag* TestTag1 = TempTestRegistry.GetTagByName("TEST_TAG1");
    if (EZ_TEST_BOOL(TestTag1 != nullptr))
    {
      EZ_ANALYSIS_ASSUME(TestTag1 != nullptr);

      const ezTag& TestTag2 = TempTestRegistry.RegisterTag("TEST_TAG2");

      EZ_TEST_BOOL(TestTag2.IsValid());

      ezTagSet tagSet;

      EZ_TEST_BOOL(tagSet.IsSet(*TestTag1) == false);
      EZ_TEST_BOOL(tagSet.IsSet(TestTag2) == false);

      tagSet.Set(TestTag2);

      EZ_TEST_BOOL(tagSet.IsSet(*TestTag1) == false);
      EZ_TEST_BOOL(tagSet.IsSet(TestTag2) == true);
      EZ_TEST_INT(tagSet.GetNumTagsSet(), 1);

      tagSet.Set(*TestTag1);

      EZ_TEST_BOOL(tagSet.IsSet(*TestTag1) == true);
      EZ_TEST_BOOL(tagSet.IsSet(TestTag2) == true);
      EZ_TEST_INT(tagSet.GetNumTagsSet(), 2);

      tagSet.Remove(*TestTag1);

      EZ_TEST_BOOL(tagSet.IsSet(*TestTag1) == false);
      EZ_TEST_BOOL(tagSet.IsSet(TestTag2) == true);
      EZ_TEST_INT(tagSet.GetNumTagsSet(), 1);

      ezTagSet tagSet2 = tagSet;
      EZ_TEST_BOOL(tagSet2.IsSet(*TestTag1) == false);
      EZ_TEST_BOOL(tagSet2.IsSet(TestTag2) == true);
      EZ_TEST_INT(tagSet2.GetNumTagsSet(), 1);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Many Tags")
  {
    ezTagRegistry TempTestRegistry;

    // TagSets have local storage for 1 block (64 tags)
    // Allocate enough tags so the storage overflows (or doesn't start at block 0)
    // for these tests

    ezTag RegisteredTags[250];

    // Pre register some tags
    TempTestRegistry.RegisterTag("TEST_TAG1");
    TempTestRegistry.RegisterTag("TEST_TAG2");

    for (ezUInt32 i = 0; i < 250; ++i)
    {
      ezStringBuilder TagName;
      TagName.SetFormat("TEST_TAG{0}", i);

      RegisteredTags[i] = TempTestRegistry.RegisterTag(TagName.GetData());

      EZ_TEST_BOOL(RegisteredTags[i].IsValid());
    }

    EZ_TEST_INT(TempTestRegistry.GetNumTags(), 250);

    // Set all tags
    ezTagSet BigTagSet;

    BigTagSet.Set(RegisteredTags[128]);
    BigTagSet.Set(RegisteredTags[64]);
    BigTagSet.Set(RegisteredTags[0]);

    EZ_TEST_BOOL(BigTagSet.IsSet(RegisteredTags[0]));
    EZ_TEST_BOOL(BigTagSet.IsSet(RegisteredTags[64]));
    EZ_TEST_BOOL(BigTagSet.IsSet(RegisteredTags[128]));

    for (ezUInt32 i = 0; i < 250; ++i)
    {
      BigTagSet.Set(RegisteredTags[i]);
    }

    for (ezUInt32 i = 0; i < 250; ++i)
    {
      EZ_TEST_BOOL(BigTagSet.IsSet(RegisteredTags[i]));
    }

    for (ezUInt32 i = 10; i < 60; ++i)
    {
      BigTagSet.Remove(RegisteredTags[i]);
    }

    for (ezUInt32 i = 0; i < 10; ++i)
    {
      EZ_TEST_BOOL(BigTagSet.IsSet(RegisteredTags[i]));
    }

    for (ezUInt32 i = 10; i < 60; ++i)
    {
      EZ_TEST_BOOL(!BigTagSet.IsSet(RegisteredTags[i]));
    }

    for (ezUInt32 i = 60; i < 250; ++i)
    {
      EZ_TEST_BOOL(BigTagSet.IsSet(RegisteredTags[i]));
    }

    // Set tags, but starting outside block 0. This should do no allocation
    ezTagSet Non0BlockStartSet;
    Non0BlockStartSet.Set(RegisteredTags[100]);
    EZ_TEST_BOOL(Non0BlockStartSet.IsSet(RegisteredTags[100]));
    EZ_TEST_BOOL(!Non0BlockStartSet.IsSet(RegisteredTags[0]));

    ezTagSet Non0BlockStartSet2 = Non0BlockStartSet;
    EZ_TEST_BOOL(Non0BlockStartSet2.IsSet(RegisteredTags[100]));
    EZ_TEST_INT(Non0BlockStartSet2.GetNumTagsSet(), Non0BlockStartSet.GetNumTagsSet());

    // Also test allocating a tag in an earlier block than the first tag allocated in the set
    Non0BlockStartSet.Set(RegisteredTags[0]);
    EZ_TEST_BOOL(Non0BlockStartSet.IsSet(RegisteredTags[100]));
    EZ_TEST_BOOL(Non0BlockStartSet.IsSet(RegisteredTags[0]));

    // Copying a tag set should work as well
    ezTagSet SecondTagSet = BigTagSet;

    for (ezUInt32 i = 60; i < 250; ++i)
    {
      EZ_TEST_BOOL(SecondTagSet.IsSet(RegisteredTags[i]));
    }

    for (ezUInt32 i = 10; i < 60; ++i)
    {
      EZ_TEST_BOOL(!SecondTagSet.IsSet(RegisteredTags[i]));
    }

    EZ_TEST_INT(SecondTagSet.GetNumTagsSet(), BigTagSet.GetNumTagsSet());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsAnySet")
  {
    ezTagRegistry TempTestRegistry;

    // TagSets have local storage for 1 block (64 tags)
    // Allocate enough tags so the storage overflows (or doesn't start at block 0)
    // for these tests

    ezTag RegisteredTags[250];

    for (ezUInt32 i = 0; i < 250; ++i)
    {
      ezStringBuilder TagName;
      TagName.SetFormat("TEST_TAG{0}", i);

      RegisteredTags[i] = TempTestRegistry.RegisterTag(TagName.GetData());

      EZ_TEST_BOOL(RegisteredTags[i].IsValid());
    }

    ezTagSet EmptyTagSet;
    ezTagSet SecondEmptyTagSet;

    EZ_TEST_BOOL(!EmptyTagSet.IsAnySet(SecondEmptyTagSet));
    EZ_TEST_BOOL(!SecondEmptyTagSet.IsAnySet(EmptyTagSet));


    ezTagSet SimpleSingleTagBlock0;
    SimpleSingleTagBlock0.Set(RegisteredTags[0]);

    ezTagSet SimpleSingleTagBlock1;
    SimpleSingleTagBlock1.Set(RegisteredTags[0]);

    EZ_TEST_BOOL(!SecondEmptyTagSet.IsAnySet(SimpleSingleTagBlock0));

    EZ_TEST_BOOL(SimpleSingleTagBlock0.IsAnySet(SimpleSingleTagBlock0));
    EZ_TEST_BOOL(SimpleSingleTagBlock0.IsAnySet(SimpleSingleTagBlock1));

    SimpleSingleTagBlock1.Remove(RegisteredTags[0]);
    EZ_TEST_BOOL(!SimpleSingleTagBlock1.IsAnySet(SimpleSingleTagBlock0));

    // Try with different block sizes/offsets (but same bit index)
    SimpleSingleTagBlock1.Set(RegisteredTags[64]);

    EZ_TEST_BOOL(!SimpleSingleTagBlock1.IsAnySet(SimpleSingleTagBlock0));
    EZ_TEST_BOOL(!SimpleSingleTagBlock0.IsAnySet(SimpleSingleTagBlock1));

    SimpleSingleTagBlock0.Set(RegisteredTags[65]);
    EZ_TEST_BOOL(!SimpleSingleTagBlock1.IsAnySet(SimpleSingleTagBlock0));
    EZ_TEST_BOOL(!SimpleSingleTagBlock0.IsAnySet(SimpleSingleTagBlock1));

    SimpleSingleTagBlock0.Set(RegisteredTags[64]);
    EZ_TEST_BOOL(SimpleSingleTagBlock1.IsAnySet(SimpleSingleTagBlock0));
    EZ_TEST_BOOL(SimpleSingleTagBlock0.IsAnySet(SimpleSingleTagBlock1));

    ezTagSet OffsetBlock;
    OffsetBlock.Set(RegisteredTags[65]);
    EZ_TEST_BOOL(OffsetBlock.IsAnySet(SimpleSingleTagBlock0));
    EZ_TEST_BOOL(SimpleSingleTagBlock0.IsAnySet(OffsetBlock));

    ezTagSet OffsetBlock2;
    OffsetBlock2.Set(RegisteredTags[66]);
    EZ_TEST_BOOL(!OffsetBlock.IsAnySet(OffsetBlock2));
    EZ_TEST_BOOL(!OffsetBlock2.IsAnySet(OffsetBlock));

    OffsetBlock2.Set(RegisteredTags[65]);
    EZ_TEST_BOOL(OffsetBlock.IsAnySet(OffsetBlock2));
    EZ_TEST_BOOL(OffsetBlock2.IsAnySet(OffsetBlock));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Add / Remove / IsEmpty / Clear")
  {
    ezTagRegistry TempTestRegistry;

    TempTestRegistry.RegisterTag("TEST_TAG1");

    const ezTag* TestTag1 = TempTestRegistry.GetTagByName("TEST_TAG1");
    EZ_TEST_BOOL(TestTag1 != nullptr);

    const ezTag& TestTag2 = TempTestRegistry.RegisterTag("TEST_TAG2");

    EZ_TEST_BOOL(TestTag2.IsValid());

    ezTagSet tagSet;

    EZ_TEST_BOOL(tagSet.IsEmpty());
    EZ_TEST_BOOL(tagSet.IsSet(*TestTag1) == false);
    EZ_TEST_BOOL(tagSet.IsSet(TestTag2) == false);

    tagSet.Clear();

    EZ_TEST_BOOL(tagSet.IsEmpty());
    EZ_TEST_BOOL(tagSet.IsSet(*TestTag1) == false);
    EZ_TEST_BOOL(tagSet.IsSet(TestTag2) == false);

    tagSet.Set(TestTag2);

    EZ_TEST_BOOL(!tagSet.IsEmpty());
    EZ_TEST_BOOL(tagSet.IsSet(*TestTag1) == false);
    EZ_TEST_BOOL(tagSet.IsSet(TestTag2) == true);

    tagSet.Remove(TestTag2);

    EZ_TEST_BOOL(tagSet.IsEmpty());
    EZ_TEST_BOOL(tagSet.IsSet(*TestTag1) == false);
    EZ_TEST_BOOL(tagSet.IsSet(TestTag2) == false);

    tagSet.Set(*TestTag1);
    tagSet.Set(TestTag2);

    EZ_TEST_BOOL(!tagSet.IsEmpty());
    EZ_TEST_BOOL(tagSet.IsSet(*TestTag1) == true);
    EZ_TEST_BOOL(tagSet.IsSet(TestTag2) == true);

    tagSet.Remove(*TestTag1);
    tagSet.Remove(TestTag2);

    EZ_TEST_BOOL(tagSet.IsEmpty());
    EZ_TEST_BOOL(tagSet.IsSet(*TestTag1) == false);
    EZ_TEST_BOOL(tagSet.IsSet(TestTag2) == false);

    tagSet.Set(*TestTag1);
    tagSet.Set(TestTag2);

    EZ_TEST_BOOL(!tagSet.IsEmpty());
    EZ_TEST_BOOL(tagSet.IsSet(*TestTag1) == true);
    EZ_TEST_BOOL(tagSet.IsSet(TestTag2) == true);

    tagSet.Clear();

    EZ_TEST_BOOL(tagSet.IsEmpty());
    EZ_TEST_BOOL(tagSet.IsSet(*TestTag1) == false);
    EZ_TEST_BOOL(tagSet.IsSet(TestTag2) == false);
  }
}
