#include <PCH.h>
#include <Foundation/Strings/String.h>

EZ_CREATE_SIMPLE_TEST(Strings, StringBase)
{
  // These tests need not be very through, as ezStringBase only passes through to ezStringUtil
  // which has been tested elsewhere already.
  // Here it is only assured that ezStringBases passes its own pointers properly through,
  // such that the ezStringUtil functions are called correctly.

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsEmpty")
  {
    ezStringView it (nullptr);
    EZ_TEST_BOOL(it.IsEmpty());

    ezStringView it2("");
    EZ_TEST_BOOL(it2.IsEmpty());

    ezStringView it3(nullptr, nullptr);
    EZ_TEST_BOOL(it3.IsEmpty());

    const char* sz = "abcdef";

    ezStringView it4(sz, sz);
    EZ_TEST_BOOL(it4.IsEmpty());

    ezStringView it5(sz, sz + 1);
    EZ_TEST_BOOL(!it5.IsEmpty());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "StartsWith")
  {
    const char* sz = "abcdef";
    ezStringView it(sz);

    EZ_TEST_BOOL(it.StartsWith("abc"));
    EZ_TEST_BOOL(it.StartsWith("abcdef"));
    EZ_TEST_BOOL(it.StartsWith("")); // empty strings always return true

    ezStringView it2(sz + 3);

    EZ_TEST_BOOL(it2.StartsWith("def"));
    EZ_TEST_BOOL(it2.StartsWith(""));

    ezStringView it3(sz + 2, sz + 4);
    it3.SetCurrentPosition(sz + 3);

    EZ_TEST_BOOL(it3.StartsWith("d"));
    EZ_TEST_BOOL(!it3.StartsWith("de"));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "StartsWith_NoCase")
  {
    const char* sz = "abcdef";
    ezStringView it(sz);

    EZ_TEST_BOOL(it.StartsWith_NoCase("ABC"));
    EZ_TEST_BOOL(it.StartsWith_NoCase("abcDEF"));
    EZ_TEST_BOOL(it.StartsWith_NoCase("")); // empty strings always return true

    ezStringView it2(sz + 3);

    EZ_TEST_BOOL(it2.StartsWith_NoCase("DEF"));
    EZ_TEST_BOOL(it2.StartsWith_NoCase(""));

    ezStringView it3(sz + 2, sz + 4);
    it3.SetCurrentPosition(sz + 3);

    EZ_TEST_BOOL(it3.StartsWith_NoCase("D"));
    EZ_TEST_BOOL(!it3.StartsWith_NoCase("DE"));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "EndsWith")
  {
    const char* sz = "abcdef";
    ezStringView it(sz);

    EZ_TEST_BOOL(it.EndsWith("def"));
    EZ_TEST_BOOL(it.EndsWith("abcdef"));
    EZ_TEST_BOOL(it.EndsWith("")); // empty strings always return true

    ezStringView it2(sz + 3);

    EZ_TEST_BOOL(it2.EndsWith("def"));
    EZ_TEST_BOOL(it2.EndsWith(""));

    ezStringView it3(sz + 2, sz + 4);
    it3.SetCurrentPosition(sz + 3);

    EZ_TEST_BOOL(it3.EndsWith("d"));
    EZ_TEST_BOOL(!it3.EndsWith("cd"));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "EndsWith_NoCase")
  {
    const char* sz = "ABCDEF";
    ezStringView it(sz);

    EZ_TEST_BOOL(it.EndsWith_NoCase("def"));
    EZ_TEST_BOOL(it.EndsWith_NoCase("abcdef"));
    EZ_TEST_BOOL(it.EndsWith_NoCase("")); // empty strings always return true

    ezStringView it2(sz + 3);

    EZ_TEST_BOOL(it2.EndsWith_NoCase("def"));
    EZ_TEST_BOOL(it2.EndsWith_NoCase(""));

    ezStringView it3(sz + 2, sz + 4);
    it3.SetCurrentPosition(sz + 3);

    EZ_TEST_BOOL(it3.EndsWith_NoCase("d"));
    EZ_TEST_BOOL(!it3.EndsWith_NoCase("cd"));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "FindSubString")
  {
    const char* sz = "abcdef";
    ezStringView it(sz);

    EZ_TEST_BOOL(it.FindSubString("abcdef") == sz);
    EZ_TEST_BOOL(it.FindSubString("abc") == sz);
    EZ_TEST_BOOL(it.FindSubString("def") == sz + 3);
    EZ_TEST_BOOL(it.FindSubString("cd") == sz + 2);
    EZ_TEST_BOOL(it.FindSubString("") == nullptr);
    EZ_TEST_BOOL(it.FindSubString(nullptr) == nullptr);
    EZ_TEST_BOOL(it.FindSubString("g") == nullptr);

    EZ_TEST_BOOL(it.FindSubString("abcdef", sz) == sz);
    EZ_TEST_BOOL(it.FindSubString("abcdef", sz + 1) == nullptr);
    EZ_TEST_BOOL(it.FindSubString("def", sz + 2) == sz + 3);
    EZ_TEST_BOOL(it.FindSubString("def", sz + 3) == sz + 3);
    EZ_TEST_BOOL(it.FindSubString("def", sz + 4) == nullptr);
    EZ_TEST_BOOL(it.FindSubString("", sz + 3) == nullptr);

    ezStringView it2(sz + 1, sz + 5);
    it2.SetCurrentPosition(sz + 2);

    EZ_TEST_BOOL(it2.FindSubString("abcdef") == nullptr);
    EZ_TEST_BOOL(it2.FindSubString("abc") == nullptr);
    EZ_TEST_BOOL(it2.FindSubString("de") == sz + 3);
    EZ_TEST_BOOL(it2.FindSubString("cd") == sz + 2);
    EZ_TEST_BOOL(it2.FindSubString("") == nullptr);
    EZ_TEST_BOOL(it2.FindSubString(nullptr) == nullptr);
    EZ_TEST_BOOL(it2.FindSubString("g") == nullptr);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "FindSubString_NoCase")
  {
    const char* sz = "ABCDEF";
    ezStringView it(sz);

    EZ_TEST_BOOL(it.FindSubString_NoCase("abcdef") == sz);
    EZ_TEST_BOOL(it.FindSubString_NoCase("abc") == sz);
    EZ_TEST_BOOL(it.FindSubString_NoCase("def") == sz + 3);
    EZ_TEST_BOOL(it.FindSubString_NoCase("cd") == sz + 2);
    EZ_TEST_BOOL(it.FindSubString_NoCase("") == nullptr);
    EZ_TEST_BOOL(it.FindSubString_NoCase(nullptr) == nullptr);
    EZ_TEST_BOOL(it.FindSubString_NoCase("g") == nullptr);

    EZ_TEST_BOOL(it.FindSubString_NoCase("abcdef", sz) == sz);
    EZ_TEST_BOOL(it.FindSubString_NoCase("abcdef", sz + 1) == nullptr);
    EZ_TEST_BOOL(it.FindSubString_NoCase("def", sz + 2) == sz + 3);
    EZ_TEST_BOOL(it.FindSubString_NoCase("def", sz + 3) == sz + 3);
    EZ_TEST_BOOL(it.FindSubString_NoCase("def", sz + 4) == nullptr);
    EZ_TEST_BOOL(it.FindSubString_NoCase("", sz + 3) == nullptr);


    ezStringView it2(sz + 1, sz + 5);
    it2.SetCurrentPosition(sz + 2);

    EZ_TEST_BOOL(it2.FindSubString_NoCase("abcdef") == nullptr);
    EZ_TEST_BOOL(it2.FindSubString_NoCase("abc") == nullptr);
    EZ_TEST_BOOL(it2.FindSubString_NoCase("de") == sz + 3);
    EZ_TEST_BOOL(it2.FindSubString_NoCase("cd") == sz + 2);
    EZ_TEST_BOOL(it2.FindSubString_NoCase("") == nullptr);
    EZ_TEST_BOOL(it2.FindSubString_NoCase(nullptr) == nullptr);
    EZ_TEST_BOOL(it2.FindSubString_NoCase("g") == nullptr);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "FindLastSubString")
  {
    const char* sz = "abcdef";
    ezStringView it(sz);

    EZ_TEST_BOOL(it.FindLastSubString("abcdef") == sz);
    EZ_TEST_BOOL(it.FindLastSubString("abc") == sz);
    EZ_TEST_BOOL(it.FindLastSubString("def") == sz + 3);
    EZ_TEST_BOOL(it.FindLastSubString("cd") == sz + 2);
    EZ_TEST_BOOL(it.FindLastSubString("") == nullptr);
    EZ_TEST_BOOL(it.FindLastSubString(nullptr) == nullptr);
    EZ_TEST_BOOL(it.FindLastSubString("g") == nullptr);

    ezStringView it2(sz + 1, sz + 5);
    it2.SetCurrentPosition(sz + 2);

    EZ_TEST_BOOL(it2.FindLastSubString("abcdef") == nullptr);
    EZ_TEST_BOOL(it2.FindLastSubString("abc") == nullptr);
    EZ_TEST_BOOL(it2.FindLastSubString("de") == sz + 3);
    EZ_TEST_BOOL(it2.FindLastSubString("cd") == sz + 2);
    EZ_TEST_BOOL(it2.FindLastSubString("") == nullptr);
    EZ_TEST_BOOL(it2.FindLastSubString(nullptr) == nullptr);
    EZ_TEST_BOOL(it2.FindLastSubString("g") == nullptr);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "FindLastSubString_NoCase")
  {
    const char* sz = "ABCDEF";
    ezStringView it(sz);

    EZ_TEST_BOOL(it.FindLastSubString_NoCase("abcdef") == sz);
    EZ_TEST_BOOL(it.FindLastSubString_NoCase("abc") == sz);
    EZ_TEST_BOOL(it.FindLastSubString_NoCase("def") == sz + 3);
    EZ_TEST_BOOL(it.FindLastSubString_NoCase("cd") == sz + 2);
    EZ_TEST_BOOL(it.FindLastSubString_NoCase("") == nullptr);
    EZ_TEST_BOOL(it.FindLastSubString_NoCase(nullptr) == nullptr);
    EZ_TEST_BOOL(it.FindLastSubString_NoCase("g") == nullptr);

    ezStringView it2(sz + 1, sz + 5);
    it2.SetCurrentPosition(sz + 2);

    EZ_TEST_BOOL(it2.FindLastSubString_NoCase("abcdef") == nullptr);
    EZ_TEST_BOOL(it2.FindLastSubString_NoCase("abc") == nullptr);
    EZ_TEST_BOOL(it2.FindLastSubString_NoCase("de") == sz + 3);
    EZ_TEST_BOOL(it2.FindLastSubString_NoCase("cd") == sz + 2);
    EZ_TEST_BOOL(it2.FindLastSubString_NoCase("") == nullptr);
    EZ_TEST_BOOL(it2.FindLastSubString_NoCase(nullptr) == nullptr);
    EZ_TEST_BOOL(it2.FindLastSubString_NoCase("g") == nullptr);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Compare")
  {
    const char* sz = "abcdef";
    ezStringView it(sz);

    EZ_TEST_BOOL(it.Compare("abcdef") == 0);
    EZ_TEST_BOOL(it.Compare("abcde") > 0);
    EZ_TEST_BOOL(it.Compare("abcdefg") < 0);

    ezStringView it2(sz + 2, sz + 5);
    it2.SetCurrentPosition(sz + 3);

    EZ_TEST_BOOL(it2.Compare("de") == 0);
    EZ_TEST_BOOL(it2.Compare("def") < 0);
    EZ_TEST_BOOL(it2.Compare("d") > 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Compare_NoCase")
  {
    const char* sz = "ABCDEF";
    ezStringView it(sz);

    EZ_TEST_BOOL(it.Compare_NoCase("abcdef") == 0);
    EZ_TEST_BOOL(it.Compare_NoCase("abcde") > 0);
    EZ_TEST_BOOL(it.Compare_NoCase("abcdefg") < 0);

    ezStringView it2(sz + 2, sz + 5);
    it2.SetCurrentPosition(sz + 3);

    EZ_TEST_BOOL(it2.Compare_NoCase("de") == 0);
    EZ_TEST_BOOL(it2.Compare_NoCase("def") < 0);
    EZ_TEST_BOOL(it2.Compare_NoCase("d") > 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "CompareN")
  {
    const char* sz = "abcdef";
    ezStringView it(sz);

    EZ_TEST_BOOL(it.CompareN("abc", 3) == 0);
    EZ_TEST_BOOL(it.CompareN("abcde", 6) > 0);
    EZ_TEST_BOOL(it.CompareN("abcg", 3) == 0);

    ezStringView it2(sz + 2, sz + 5);

    EZ_TEST_BOOL(it2.CompareN("cd", 2) == 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "CompareN_NoCase")
  {
    const char* sz = "ABCDEF";
    ezStringView it(sz);

    EZ_TEST_BOOL(it.CompareN_NoCase("abc", 3) == 0);
    EZ_TEST_BOOL(it.CompareN_NoCase("abcde", 6) > 0);
    EZ_TEST_BOOL(it.CompareN_NoCase("abcg", 3) == 0);

    ezStringView it2(sz + 2, sz + 5);

    EZ_TEST_BOOL(it2.CompareN_NoCase("cd", 2) == 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsEqual")
  {
    const char* sz = "abcdef";
    ezStringView it(sz);

    EZ_TEST_BOOL(it.IsEqual("abcdef"));
    EZ_TEST_BOOL(!it.IsEqual("abcde"));
    EZ_TEST_BOOL(!it.IsEqual("abcdefg"));

    ezStringView it2(sz + 1, sz + 5);
    it2.SetCurrentPosition(sz + 2);

    EZ_TEST_BOOL(it2.IsEqual("cde"));
    EZ_TEST_BOOL(!it2.IsEqual("bcde"));
    EZ_TEST_BOOL(!it2.IsEqual("cdef"));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsEqual_NoCase")
  {
    const char* sz = "ABCDEF";
    ezStringView it(sz);

    EZ_TEST_BOOL(it.IsEqual_NoCase("abcdef"));
    EZ_TEST_BOOL(!it.IsEqual_NoCase("abcde"));
    EZ_TEST_BOOL(!it.IsEqual_NoCase("abcdefg"));

    ezStringView it2(sz + 1, sz + 5);
    it2.SetCurrentPosition(sz + 2);

    EZ_TEST_BOOL(it2.IsEqual_NoCase("cde"));
    EZ_TEST_BOOL(!it2.IsEqual_NoCase("bcde"));
    EZ_TEST_BOOL(!it2.IsEqual_NoCase("cdef"));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsEqualN")
  {
    const char* sz = "abcdef";
    ezStringView it(sz);

    EZ_TEST_BOOL(it.IsEqualN("abcGHI", 3));
    EZ_TEST_BOOL(!it.IsEqualN("abcGHI", 4));

    ezStringView it2(sz + 1, sz + 5);
    it2.SetCurrentPosition(sz + 2);

    EZ_TEST_BOOL(it2.IsEqualN("cdeZX", 3));
    EZ_TEST_BOOL(!it2.IsEqualN("cdeZX", 4));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsEqualN_NoCase")
  {
    const char* sz = "ABCDEF";
    ezStringView it(sz);

    EZ_TEST_BOOL(it.IsEqualN_NoCase("abcGHI", 3));
    EZ_TEST_BOOL(!it.IsEqualN_NoCase("abcGHI", 4));

    ezStringView it2(sz + 1, sz + 5);
    it2.SetCurrentPosition(sz + 2);

    EZ_TEST_BOOL(it2.IsEqualN_NoCase("cdeZX", 3));
    EZ_TEST_BOOL(!it2.IsEqualN_NoCase("cdeZX", 4));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator==/!=")
  {
    const char* sz = "abcdef";
    const char* sz2 = "blabla";
    ezStringView it(sz);
    ezStringView it2(sz);
    ezStringView it3(sz2);

    EZ_TEST_BOOL(it == sz);
    EZ_TEST_BOOL(sz == it);
    EZ_TEST_BOOL(it == "abcdef");
    EZ_TEST_BOOL("abcdef" == it);
    EZ_TEST_BOOL(it == it);
    EZ_TEST_BOOL(it == it2);

    EZ_TEST_BOOL(it != sz2);
    EZ_TEST_BOOL(sz2 != it);
    EZ_TEST_BOOL(it != "blabla");
    EZ_TEST_BOOL("blabla" != it);
    EZ_TEST_BOOL(it != it3);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "substring operator ==/!=/</>/<=/>=")
  {

    const char* sz1 = "aaabbbcccddd";
    const char* sz2 = "aaabbbdddeee";

    ezStringView it1(sz1 + 3, sz1 + 6);
    ezStringView it2(sz2 + 3, sz2 + 6);

    EZ_TEST_BOOL(it1 == it1);
    EZ_TEST_BOOL(it2 == it2);

    EZ_TEST_BOOL(it1 == it2);
    EZ_TEST_BOOL(!(it1 != it2));
    EZ_TEST_BOOL(!(it1 < it2));
    EZ_TEST_BOOL(!(it1 > it2));
    EZ_TEST_BOOL(it1 <= it2);
    EZ_TEST_BOOL(it1 >= it2);

    it1 = ezStringView(sz1 + 3, sz1 + 7);
    it2 = ezStringView(sz2 + 3, sz2 + 7);

    EZ_TEST_BOOL(it1 == it1);
    EZ_TEST_BOOL(it2 == it2);

    EZ_TEST_BOOL(it1 != it2);
    EZ_TEST_BOOL(!(it1 == it2));

    EZ_TEST_BOOL(it1 < it2);
    EZ_TEST_BOOL(!(it1 > it2));
    EZ_TEST_BOOL(it1 <= it2);
    EZ_TEST_BOOL(!(it1 >= it2));

    EZ_TEST_BOOL(it2 > it1);
    EZ_TEST_BOOL(!(it2 < it1));
    EZ_TEST_BOOL(it2 >= it1);
    EZ_TEST_BOOL(!(it2 <= it1));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator</>")
  {
    const char* sz = "abcdef";
    const char* sz2 = "abcdefg";
    ezStringView it(sz);
    ezStringView it2(sz2);

    EZ_TEST_BOOL(it < sz2);
    EZ_TEST_BOOL(sz < it2);
    EZ_TEST_BOOL(it < it2);

    EZ_TEST_BOOL(sz2 > it);
    EZ_TEST_BOOL(it2 > sz);
    EZ_TEST_BOOL(it2 > it);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator<=/>=")
  {
    {
      const char* sz = "abcdef";
      const char* sz2 = "abcdefg";
      ezStringView it(sz);
      ezStringView it2(sz2);

      EZ_TEST_BOOL(it <= sz2);
      EZ_TEST_BOOL(sz <= it2);
      EZ_TEST_BOOL(it <= it2);

      EZ_TEST_BOOL(sz2 >= it);
      EZ_TEST_BOOL(it2 >= sz);
      EZ_TEST_BOOL(it2 >= it);
    }

    {
      const char* sz = "abcdef";
      const char* sz2 = "abcdef";
      ezStringView it(sz);
      ezStringView it2(sz2);

      EZ_TEST_BOOL(it <= sz2);
      EZ_TEST_BOOL(sz <= it2);
      EZ_TEST_BOOL(it <= it2);

      EZ_TEST_BOOL(sz2 >= it);
      EZ_TEST_BOOL(it2 >= sz);
      EZ_TEST_BOOL(it2 >= it);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "FindWholeWord")
  {
    ezStringUtf8 s(L"abc def mompfhüßß ßßß öäü abcdef abc def abc def");
    ezStringView it(s.GetData() + 8, s.GetData() + s.GetElementCount() - 8);
    ezStringView it2(s.GetData() + 8, s.GetData() + s.GetElementCount());

    EZ_TEST_BOOL(it.FindWholeWord("abc", ezStringUtils::IsWordDelimiter_English) == &it.GetData()[34]);
    EZ_TEST_BOOL(it.FindWholeWord("def", ezStringUtils::IsWordDelimiter_English) == &it.GetData()[38]);
    EZ_TEST_BOOL(it.FindWholeWord("mompfh", ezStringUtils::IsWordDelimiter_English) == &it.GetData()[0]); // ü is not english (thus a delimiter)

    EZ_TEST_BOOL(it.FindWholeWord("abc", ezStringUtils::IsWordDelimiter_English, it.GetData() + 34) == &it.GetData()[34]);
    EZ_TEST_BOOL(it.FindWholeWord("abc", ezStringUtils::IsWordDelimiter_English, it.GetData() + 35) == nullptr);

    EZ_TEST_BOOL(it2.FindWholeWord("abc", ezStringUtils::IsWordDelimiter_English, it.GetData() + 34) == &it.GetData()[34]);
    EZ_TEST_BOOL(it2.FindWholeWord("abc", ezStringUtils::IsWordDelimiter_English, it.GetData() + 35) == &it.GetData()[42]);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "FindWholeWord_NoCase")
  {
    ezStringUtf8 s(L"abc def mompfhüßß ßßß öäü abcdef abc def abc def");
    ezStringView it(s.GetData() + 8, s.GetData() + s.GetElementCount() - 8);
    ezStringView it2(s.GetData() + 8, s.GetData() + s.GetElementCount());

    EZ_TEST_BOOL(it.FindWholeWord_NoCase("ABC", ezStringUtils::IsWordDelimiter_English) == &it.GetData()[34]);
    EZ_TEST_BOOL(it.FindWholeWord_NoCase("DEF", ezStringUtils::IsWordDelimiter_English) == &it.GetData()[38]);
    EZ_TEST_BOOL(it.FindWholeWord_NoCase("momPFH", ezStringUtils::IsWordDelimiter_English) == &it.GetData()[0]);

    EZ_TEST_BOOL(it.FindWholeWord_NoCase("ABc", ezStringUtils::IsWordDelimiter_English, it.GetData() + 34) == &it.GetData()[34]);
    EZ_TEST_BOOL(it.FindWholeWord_NoCase("ABc", ezStringUtils::IsWordDelimiter_English, it.GetData() + 35) == nullptr);

    EZ_TEST_BOOL(it2.FindWholeWord_NoCase("ABc", ezStringUtils::IsWordDelimiter_English, it.GetData() + 34) == &it.GetData()[34]);
    EZ_TEST_BOOL(it2.FindWholeWord_NoCase("ABc", ezStringUtils::IsWordDelimiter_English, it.GetData() + 35) == &it.GetData()[42]);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ComputeCharacterPosition")
  {
    wchar_t* sz = L"mompfhüßß ßßß öäü abcdef abc def abc def";
    ezStringBuilder s(sz);

    EZ_TEST_STRING(s.ComputeCharacterPosition(14), ezStringUtf8(L"öäü abcdef abc def abc def").GetData());
  }
}

