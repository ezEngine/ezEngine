#include <ToolsFoundationTest/ToolsFoundationTestPCH.h>

#include <Foundation/Strings/StringBuilder.h>
#include <ToolsFoundation/Utilities/StringAlgorithms.h>

EZ_CREATE_SIMPLE_TEST_GROUP(Utilities);

EZ_CREATE_SIMPLE_TEST(Utilities, StringAlgorithms)
{
  auto Check = [](ezStringView sLeft, ezStringView sRight, ezStringView sExpected)
  {
    ezStringBuilder sResult;
    ezStringAlgorithms::ComputeNameBetween(sLeft, sRight, sResult);
    EZ_TEST_STRING(sResult, sExpected);
  };

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Append")
  {
    Check("1", "", "2");
    Check("3", "", "4");
    Check("-1", "", "0");
    // Root integer is used regardless of sub-levels
    Check("3.5", "", "4");
    Check("3.5.2", "", "4");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Prepend")
  {
    Check("", "1", "0");
    Check("", "3", "2");
    Check("", "0", "-1");
    Check("", "-1", "-2");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IntegerMidpoint")
  {
    // Gap > 1: use integer midpoint
    Check("2", "4", "3");
    Check("1", "9", "5");
    Check("10", "20", "15");
    Check("0", "10", "5");
    Check("-4", "0", "-2");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "AdjacentIntegers")
  {
    // Gap == 1: fall back to first sub-level
    Check("3", "4", "3.5");
    Check("1", "2", "1.5");
    Check("0", "1", "0.5");
    Check("-2", "-1", "-2.5");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SubLevelInsertions")
  {
    // Inserting between a fractional name and the next integer
    Check("3.5", "4", "3.7");   // midpoint of 5 and 10 = 7
    Check("3.7", "4", "3.8");   // midpoint of 7 and 10 = 8
    Check("3.8", "4", "3.9");   // midpoint of 8 and 10 = 9
    Check("3.9", "4", "3.9.5"); // 9 adjacent to 10, go deeper
    Check("3.9.5", "4", "3.9.7");
    Check("3.9.9", "4", "3.9.9.5");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SamePrefix")
  {
    // Both names share a common root prefix
    Check("3.2", "3.4", "3.3");
    Check("3.5", "3.7", "3.6");
    Check("3.5", "3.6", "3.5.5");
    Check("3.2", "3.3", "3.2.5");
    Check("1.1", "1.9", "1.5");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "BothEmpty")
  {
    // No neighbors at all: return the initial name
    Check("", "", "1");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "NonNumericStrings")
  {
    // Fully non-numeric names have an all-text prefix and an empty numeric part (treated as 0).
    // The prefix is copied to the result; the numeric portion follows the usual scheme.
    Check("", "foo", "foo -1");     // prefix "foo" inherited from right; numeric: 0-1 = -1
    Check("foo", "", "foo 1");      // prefix "foo" from left; numeric: 0+1 = 1
    Check("foo", "bar", "foo 0.5"); // both numeric parts empty (→ 0); prefix "foo" from left

    // Mixed: one numeric, one non-numeric
    Check("", "hello", "hello -1"); // prefix "hello" from right; numeric: 0-1 = -1
    Check("hello", "", "hello 1");  // prefix "hello" from left; numeric: 0+1 = 1
    Check("foo", "2", "foo 1");     // left numeric empty (→0), right numeric 2; midpoint=1

    // Dot-notation with non-numeric segments: the entire string becomes the text prefix
    // since there are no digits at all.
    Check("", "a.b.c", "a.b.c -1");
    Check("a.b.c", "", "a.b.c 1");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "WithSameTextPrefix")
  {
    // Both names share a common root prefix
    Check("Node 3.2", "Node 3.4", "Node 3.3");
    Check("Node 3.5", "Node 3.7", "Node 3.6");
    Check("Node 3.5", "Node 3.6", "Node 3.5.5");
    Check("Node 3.2", "Node 3.3", "Node 3.2.5");
    Check("Node 1.1", "Node 1.9", "Node 1.5");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "WithDifferentTextPrefix")
  {
    // Both names share a common root prefix
    Check("Bla 3.2", "Blub 3.4", "Bla 3.3");
    Check("Bla 3.5", "Blub 3.7", "Bla 3.6");
    Check("Bla 3.5", "Blub 3.6", "Bla 3.5.5");
    Check("Bla 3.2", "Blub 3.3", "Bla 3.2.5");
    Check("Bla 1.1", "Blub 1.9", "Bla 1.5");
  }
}
