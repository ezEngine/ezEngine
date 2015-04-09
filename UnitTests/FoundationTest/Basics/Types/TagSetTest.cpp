#include <PCH.h>
#include <Foundation/Types/Tag.h>
#include <Foundation/Types/TagSet.h>
#include <Foundation/Types/TagRegistry.h>
#include <Foundation/IO/MemoryStream.h>

EZ_CREATE_SIMPLE_TEST(Basics, TagSet)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Basic Tag Tests")
  {
    ezTagRegistry TempTestRegistry;

    ezTag TestTag;
    EZ_TEST_BOOL(!TestTag.IsValid());

    ezHashedString TagName;
    TagName.Assign("BASIC_TAG_TEST");

    ezTag SecondInstance;

    TempTestRegistry.RegisterTag(TagName, &SecondInstance);

    EZ_TEST_BOOL(SecondInstance.IsValid());

    EZ_TEST_BOOL(TestTag != SecondInstance);

    EZ_TEST_BOOL(TempTestRegistry.GetTag("BASIC_TAG_TEST", TestTag).Succeeded());

    EZ_TEST_BOOL(TestTag == SecondInstance);

    EZ_TEST_STRING(TestTag.GetTagString(), "BASIC_TAG_TEST");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Basic Tag Registration")
  {
    ezTagRegistry TempTestRegistry;

    ezTag TestTag;

    EZ_TEST_BOOL(!TestTag.IsValid());

    EZ_TEST_BOOL(TempTestRegistry.GetTag("TEST_TAG1", TestTag).Failed());

    TempTestRegistry.RegisterTag("TEST_TAG1", &TestTag);

    EZ_TEST_BOOL(TestTag.IsValid());

    EZ_TEST_BOOL(TempTestRegistry.GetTag("TEST_TAG1", TestTag).Succeeded());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Basic Tag Work")
  {
    ezTagRegistry TempTestRegistry;

    TempTestRegistry.RegisterTag("TEST_TAG1");

    ezTag TestTag1;
    EZ_TEST_BOOL(TempTestRegistry.GetTag("TEST_TAG1", TestTag1).Succeeded());

    ezTag TestTag2;
    TempTestRegistry.RegisterTag("TEST_TAG2", &TestTag2);
    
    EZ_TEST_BOOL(TestTag2.IsValid());

    ezTagSet TagSet;
    
    EZ_TEST_BOOL(TagSet.IsSet(TestTag1) == false);
    EZ_TEST_BOOL(TagSet.IsSet(TestTag2) == false);

    TagSet.Set(TestTag2);

    EZ_TEST_BOOL(TagSet.IsSet(TestTag1) == false);
    EZ_TEST_BOOL(TagSet.IsSet(TestTag2) == true);

    TagSet.Set(TestTag1);

    EZ_TEST_BOOL(TagSet.IsSet(TestTag1) == true);
    EZ_TEST_BOOL(TagSet.IsSet(TestTag2) == true);

    TagSet.Clear(TestTag1);

    EZ_TEST_BOOL(TagSet.IsSet(TestTag1) == false);
    EZ_TEST_BOOL(TagSet.IsSet(TestTag2) == true);
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
      TagName.Format("TEST_TAG%u", i);

      TempTestRegistry.RegisterTag(TagName.GetData(), &RegisteredTags[i]);

      EZ_TEST_BOOL(RegisteredTags[i].IsValid());
    }

    // Set all tags
    ezTagSet BigTagSet;

    BigTagSet.Set(RegisteredTags[0]);
    BigTagSet.Set(RegisteredTags[64]);

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
      BigTagSet.Clear(RegisteredTags[i]);
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
      TagName.Format("TEST_TAG%u", i);

      TempTestRegistry.RegisterTag(TagName.GetData(), &RegisteredTags[i]);

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

    SimpleSingleTagBlock1.Clear(RegisteredTags[0]);
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
}

