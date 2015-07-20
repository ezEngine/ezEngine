#include <PCH.h>
#include <Foundation/Strings/String.h>

template<typename STRING>
void TestConstruction(const STRING& value, const char* szStart, const char* szEnd)
{
  ezStringUtf8 sUtf8(L"A単語F");
  EZ_TEST_BOOL(value.IsEqual(sUtf8.GetData()));
  const bool bEqualForwardItTypes = ezConversionTest<typename STRING::iterator, typename STRING::const_iterator>::sameType == 1;
  EZ_CHECK_AT_COMPILETIME_MSG(bEqualForwardItTypes, "As the string iterator is read-only, both const and non-const versions should be the same type.");
  const bool bEqualReverseItTypes = ezConversionTest<typename STRING::reverse_iterator, typename STRING::const_reverse_iterator>::sameType == 1;
  EZ_CHECK_AT_COMPILETIME_MSG(bEqualReverseItTypes, "As the reverse string iterator is read-only, both const and non-const versions should be the same type.");

  typename STRING::iterator itInvalid;
  EZ_TEST_BOOL(!itInvalid.IsValid());
  typename STRING::reverse_iterator itInvalidR;
  EZ_TEST_BOOL(!itInvalidR.IsValid());

  // Begin
  const typename STRING::iterator itBegin = begin(value);
  EZ_TEST_BOOL(itBegin == value.GetIteratorFront());
  EZ_TEST_BOOL(itBegin.IsValid());
  EZ_TEST_BOOL(itBegin == itBegin);
  EZ_TEST_BOOL(itBegin.GetData() == szStart);
  EZ_TEST_BOOL(itBegin.GetCharacter() == ezUnicodeUtils::ConvertUtf8ToUtf32("A"));
  EZ_TEST_BOOL(*itBegin == ezUnicodeUtils::ConvertUtf8ToUtf32("A"));

  // End
  const typename STRING::iterator itEnd = end(value);
  EZ_TEST_BOOL(!itEnd.IsValid());
  EZ_TEST_BOOL(itEnd == itEnd);
  EZ_TEST_BOOL(itBegin != itEnd);
  EZ_TEST_BOOL(itEnd.GetData() == szEnd);
  EZ_TEST_BOOL(itEnd.GetCharacter() == 0);
  EZ_TEST_BOOL(*itEnd == 0);

  // RBegin
  const typename STRING::reverse_iterator itBeginR = rbegin(value);
  EZ_TEST_BOOL(itBeginR == value.GetIteratorBack());
  EZ_TEST_BOOL(itBeginR.IsValid());
  EZ_TEST_BOOL(itBeginR == itBeginR);
  const char* szEndPrior = szEnd;
  ezUnicodeUtils::MoveToPriorUtf8(szEndPrior);
  EZ_TEST_BOOL(itBeginR.GetData() == szEndPrior);
  EZ_TEST_BOOL(itBeginR.GetCharacter() == ezUnicodeUtils::ConvertUtf8ToUtf32("F"));
  EZ_TEST_BOOL(*itBeginR == ezUnicodeUtils::ConvertUtf8ToUtf32("F"));

  // REnd
  const typename STRING::reverse_iterator itEndR = rend(value);
  EZ_TEST_BOOL(!itEndR.IsValid());
  EZ_TEST_BOOL(itEndR == itEndR);
  EZ_TEST_BOOL(itBeginR != itEndR);
  EZ_TEST_BOOL(itEndR.GetData() == nullptr); // Position before first character is not a valid ptr, so it is set to nullptr.
  EZ_TEST_BOOL(itEndR.GetCharacter() == 0);
  EZ_TEST_BOOL(*itEndR == 0);
}

template<typename STRING, typename IT>
void TestIteratorBegin(const STRING& value, const IT& it)
{
  // It is safe to try to move beyond the iterator's range.
  IT itBegin = it;
  --itBegin;
  itBegin -= 4;
  EZ_TEST_BOOL(itBegin == it);
  EZ_TEST_BOOL(itBegin - 2 == it);

  // Prefix / Postfix
  EZ_TEST_BOOL(itBegin + 2 != it);
  EZ_TEST_BOOL(itBegin++ == it);
  EZ_TEST_BOOL(itBegin-- != it);
  itBegin = it;
  EZ_TEST_BOOL(++itBegin != it);
  EZ_TEST_BOOL(--itBegin == it);

  // Misc
  itBegin = it;
  EZ_TEST_BOOL(it + 2 == ++(++itBegin));
  itBegin -= 1;
  EZ_TEST_BOOL(itBegin == it + 1);
  itBegin -= 0;
  EZ_TEST_BOOL(itBegin == it + 1);
  itBegin += 0;
  EZ_TEST_BOOL(itBegin == it + 1);
  itBegin += -1;
  EZ_TEST_BOOL(itBegin == it);
}

template<typename STRING, typename IT>
void TestIteratorEnd(const STRING& value, const IT& it)
{
  // It is safe to try to move beyond the iterator's range.
  IT itEnd = it;
  ++itEnd;
  itEnd += 4;
  EZ_TEST_BOOL(itEnd == it);
  EZ_TEST_BOOL(itEnd + 2 == it);

  // Prefix / Postfix
  EZ_TEST_BOOL(itEnd - 2 != it);
  EZ_TEST_BOOL(itEnd-- == it);
  EZ_TEST_BOOL(itEnd++ != it);
  itEnd = it;
  EZ_TEST_BOOL(--itEnd != it);
  EZ_TEST_BOOL(++itEnd == it);

  // Misc
  itEnd = it;
  EZ_TEST_BOOL(it - 2 == --(--itEnd))
  itEnd += 1;
  EZ_TEST_BOOL(itEnd == it - 1);
  itEnd += 0;
  EZ_TEST_BOOL(itEnd == it - 1);
  itEnd -= 0;
  EZ_TEST_BOOL(itEnd == it - 1);
  itEnd -= -1;
  EZ_TEST_BOOL(itEnd == it);
}

template<typename STRING>
void TestOperators(const STRING& value, const char* szStart, const char* szEnd)
{
  ezStringUtf8 sUtf8(L"A単語F");
  EZ_TEST_BOOL(value.IsEqual(sUtf8.GetData()));

  // Begin
  typename STRING::iterator itBegin = begin(value);
  TestIteratorBegin(value, itBegin);

  // End
  typename STRING::iterator itEnd = end(value);
  TestIteratorEnd(value, itEnd);

  // RBegin
  typename STRING::reverse_iterator itBeginR = rbegin(value);
  TestIteratorBegin(value, itBeginR);

  // REnd
  typename STRING::reverse_iterator itEndR = rend(value);
  TestIteratorEnd(value, itEndR);
}

template<typename STRING>
void TestLoops(const STRING& value, const char* szStart, const char* szEnd)
{
  ezStringUtf8 sUtf8(L"A単語F");
  ezUInt32 characters[] = {
    ezUnicodeUtils::ConvertUtf8ToUtf32(ezStringUtf8(L"A").GetData()),
    ezUnicodeUtils::ConvertUtf8ToUtf32(ezStringUtf8(L"単").GetData()),
    ezUnicodeUtils::ConvertUtf8ToUtf32(ezStringUtf8(L"語").GetData()),
    ezUnicodeUtils::ConvertUtf8ToUtf32(ezStringUtf8(L"F").GetData())};

  // Forward
  ezInt32 iIndex = 0;
  for (ezUInt32 character : value)
  {
    EZ_TEST_INT(characters[iIndex], character);
    ++iIndex;
  }
  EZ_TEST_INT(iIndex, 4);

  typename STRING::iterator itBegin = begin(value);
  typename STRING::iterator itEnd = end(value);
  iIndex = 0;
  for (auto it = itBegin; it != itEnd; ++it)
  {
    EZ_TEST_BOOL(it.IsValid());
    EZ_TEST_INT(characters[iIndex], it.GetCharacter());
    EZ_TEST_INT(characters[iIndex], *it);
    EZ_TEST_BOOL(it.GetData() >= szStart);
    EZ_TEST_BOOL(it.GetData() < szEnd);
    ++iIndex;
  }
  EZ_TEST_INT(iIndex, 4);

  // Reverse
  typename STRING::reverse_iterator itBeginR = rbegin(value);
  typename STRING::reverse_iterator itEndR = rend(value);
  iIndex = 3;
  for (auto it = itBeginR; it != itEndR; ++it)
  {
    EZ_TEST_BOOL(it.IsValid());
    EZ_TEST_INT(characters[iIndex], it.GetCharacter());
    EZ_TEST_INT(characters[iIndex], *it);
    EZ_TEST_BOOL(it.GetData() >= szStart);
    EZ_TEST_BOOL(it.GetData() < szEnd);
    --iIndex;
  }
  EZ_TEST_INT(iIndex, -1);
}

EZ_CREATE_SIMPLE_TEST(Strings, StringIterator)
{
  ezStringUtf8 sUtf8(L"_A単語F_");
  ezStringBuilder sTestStringBuilder = sUtf8.GetData();
  sTestStringBuilder.Shrink(1, 1);
  ezString sTextString = sTestStringBuilder.GetData();
  
  ezStringView view(sUtf8.GetData());
  view.Shrink(1, 1);

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Construction")
  {
    TestConstruction<ezString>(sTextString, sTextString.GetData(), sTextString.GetData() + sTextString.GetElementCount());
    TestConstruction<ezStringBuilder>(sTestStringBuilder, sTestStringBuilder.GetData(), sTestStringBuilder.GetData() + sTestStringBuilder.GetElementCount());
    TestConstruction<ezStringView>(view, view.GetStartPosition(), view.GetEndPosition());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Operators")
  {
    TestOperators<ezString>(sTextString, sTextString.GetData(), sTextString.GetData() + sTextString.GetElementCount());
    TestOperators<ezStringBuilder>(sTestStringBuilder, sTestStringBuilder.GetData(), sTestStringBuilder.GetData() + sTestStringBuilder.GetElementCount());
    TestOperators<ezStringView>(view, view.GetStartPosition(), view.GetEndPosition());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Loops")
  {
    TestLoops<ezString>(sTextString, sTextString.GetData(), sTextString.GetData() + sTextString.GetElementCount());
    TestLoops<ezStringBuilder>(sTestStringBuilder, sTestStringBuilder.GetData(), sTestStringBuilder.GetData() + sTestStringBuilder.GetElementCount());
    TestLoops<ezStringView>(view, view.GetStartPosition(), view.GetEndPosition());
  }
}

