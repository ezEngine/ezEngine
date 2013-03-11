#include <TestFramework/Framework/TestFramework.h>
#include <Foundation/Strings/StringConversion.h>
#include <Foundation/Strings/StringIterator.h>

EZ_CREATE_SIMPLE_TEST(Strings, StringBase)
{
  // These tests need not be very through, as ezStringBase only passes through to ezStringUtil
  // which has been tested elsewhere already.
  // Here it is only assured that ezStringBases passes its own pointers properly through,
  // such that the ezStringUtil functions are called correctly.

  EZ_TEST_BLOCK(true, "IsEmpty")
  {
    ezStringIterator it (NULL);
    EZ_TEST(it.IsEmpty());

    ezStringIterator it2("");
    EZ_TEST(it2.IsEmpty());

    ezStringIterator it3(NULL, NULL, NULL);
    EZ_TEST(it3.IsEmpty());

    const char* sz = "abcdef";

    ezStringIterator it4(sz, sz, sz);
    EZ_TEST(it4.IsEmpty());

    ezStringIterator it5(sz, sz + 1, sz);
    EZ_TEST(!it5.IsEmpty());
  }

  EZ_TEST_BLOCK(true, "StartsWith")
  {
    const char* sz = "abcdef";
    ezStringIterator it(sz);

    EZ_TEST(it.StartsWith("abc"));
    EZ_TEST(it.StartsWith("abcdef"));
    EZ_TEST(it.StartsWith("")); // empty strings always return true

    ezStringIterator it2(sz + 3);

    EZ_TEST(it2.StartsWith("def"));
    EZ_TEST(it2.StartsWith(""));

    ezStringIterator it3(sz + 2, sz + 4, sz + 3);

    EZ_TEST(it3.StartsWith("d"));
    EZ_TEST(!it3.StartsWith("de"));
  }

  EZ_TEST_BLOCK(true, "StartsWith_NoCase")
  {
    const char* sz = "abcdef";
    ezStringIterator it(sz);

    EZ_TEST(it.StartsWith_NoCase("ABC"));
    EZ_TEST(it.StartsWith_NoCase("abcDEF"));
    EZ_TEST(it.StartsWith_NoCase("")); // empty strings always return true

    ezStringIterator it2(sz + 3);

    EZ_TEST(it2.StartsWith_NoCase("DEF"));
    EZ_TEST(it2.StartsWith_NoCase(""));

    ezStringIterator it3(sz + 2, sz + 4, sz + 3);

    EZ_TEST(it3.StartsWith_NoCase("D"));
    EZ_TEST(!it3.StartsWith_NoCase("DE"));
  }

  EZ_TEST_BLOCK(true, "EndsWith")
  {
    const char* sz = "abcdef";
    ezStringIterator it(sz);

    EZ_TEST(it.EndsWith("def"));
    EZ_TEST(it.EndsWith("abcdef"));
    EZ_TEST(it.EndsWith("")); // empty strings always return true

    ezStringIterator it2(sz + 3);

    EZ_TEST(it2.EndsWith("def"));
    EZ_TEST(it2.EndsWith(""));

    ezStringIterator it3(sz + 2, sz + 4, sz + 3);

    EZ_TEST(it3.EndsWith("d"));
    EZ_TEST(!it3.EndsWith("cd"));
  }

  EZ_TEST_BLOCK(true, "EndsWith_NoCase")
  {
    const char* sz = "ABCDEF";
    ezStringIterator it(sz);

    EZ_TEST(it.EndsWith_NoCase("def"));
    EZ_TEST(it.EndsWith_NoCase("abcdef"));
    EZ_TEST(it.EndsWith_NoCase("")); // empty strings always return true

    ezStringIterator it2(sz + 3);

    EZ_TEST(it2.EndsWith_NoCase("def"));
    EZ_TEST(it2.EndsWith_NoCase(""));

    ezStringIterator it3(sz + 2, sz + 4, sz + 3);

    EZ_TEST(it3.EndsWith_NoCase("d"));
    EZ_TEST(!it3.EndsWith_NoCase("cd"));
  }

  EZ_TEST_BLOCK(true, "FindSubString")
  {
    const char* sz = "abcdef";
    ezStringIterator it(sz);

    EZ_TEST(it.FindSubString("abcdef") == sz);
    EZ_TEST(it.FindSubString("abc") == sz);
    EZ_TEST(it.FindSubString("def") == sz + 3);
    EZ_TEST(it.FindSubString("cd") == sz + 2);
    EZ_TEST(it.FindSubString("") == NULL);
    EZ_TEST(it.FindSubString(NULL) == NULL);
    EZ_TEST(it.FindSubString("g") == NULL);

    ezStringIterator it2(sz + 1, sz + 5, sz + 2);

    EZ_TEST(it2.FindSubString("abcdef") == NULL);
    EZ_TEST(it2.FindSubString("abc") == NULL);
    EZ_TEST(it2.FindSubString("de") == sz + 3);
    EZ_TEST(it2.FindSubString("cd") == sz + 2);
    EZ_TEST(it2.FindSubString("") == NULL);
    EZ_TEST(it2.FindSubString(NULL) == NULL);
    EZ_TEST(it2.FindSubString("g") == NULL);
  }

  EZ_TEST_BLOCK(true, "FindSubString_NoCase")
  {
    const char* sz = "ABCDEF";
    ezStringIterator it(sz);

    EZ_TEST(it.FindSubString_NoCase("abcdef") == sz);
    EZ_TEST(it.FindSubString_NoCase("abc") == sz);
    EZ_TEST(it.FindSubString_NoCase("def") == sz + 3);
    EZ_TEST(it.FindSubString_NoCase("cd") == sz + 2);
    EZ_TEST(it.FindSubString_NoCase("") == NULL);
    EZ_TEST(it.FindSubString_NoCase(NULL) == NULL);
    EZ_TEST(it.FindSubString_NoCase("g") == NULL);

    ezStringIterator it2(sz + 1, sz + 5, sz + 2);

    EZ_TEST(it2.FindSubString_NoCase("abcdef") == NULL);
    EZ_TEST(it2.FindSubString_NoCase("abc") == NULL);
    EZ_TEST(it2.FindSubString_NoCase("de") == sz + 3);
    EZ_TEST(it2.FindSubString_NoCase("cd") == sz + 2);
    EZ_TEST(it2.FindSubString_NoCase("") == NULL);
    EZ_TEST(it2.FindSubString_NoCase(NULL) == NULL);
    EZ_TEST(it2.FindSubString_NoCase("g") == NULL);
  }

  EZ_TEST_BLOCK(true, "FindLastSubString")
  {
    const char* sz = "abcdef";
    ezStringIterator it(sz);

    EZ_TEST(it.FindLastSubString("abcdef") == sz);
    EZ_TEST(it.FindLastSubString("abc") == sz);
    EZ_TEST(it.FindLastSubString("def") == sz + 3);
    EZ_TEST(it.FindLastSubString("cd") == sz + 2);
    EZ_TEST(it.FindLastSubString("") == NULL);
    EZ_TEST(it.FindLastSubString(NULL) == NULL);
    EZ_TEST(it.FindLastSubString("g") == NULL);

    ezStringIterator it2(sz + 1, sz + 5, sz + 2);

    EZ_TEST(it2.FindLastSubString("abcdef") == NULL);
    EZ_TEST(it2.FindLastSubString("abc") == NULL);
    EZ_TEST(it2.FindLastSubString("de") == sz + 3);
    EZ_TEST(it2.FindLastSubString("cd") == sz + 2);
    EZ_TEST(it2.FindLastSubString("") == NULL);
    EZ_TEST(it2.FindLastSubString(NULL) == NULL);
    EZ_TEST(it2.FindLastSubString("g") == NULL);
  }

  EZ_TEST_BLOCK(true, "FindLastSubString_NoCase")
  {
    const char* sz = "ABCDEF";
    ezStringIterator it(sz);

    EZ_TEST(it.FindLastSubString_NoCase("abcdef") == sz);
    EZ_TEST(it.FindLastSubString_NoCase("abc") == sz);
    EZ_TEST(it.FindLastSubString_NoCase("def") == sz + 3);
    EZ_TEST(it.FindLastSubString_NoCase("cd") == sz + 2);
    EZ_TEST(it.FindLastSubString_NoCase("") == NULL);
    EZ_TEST(it.FindLastSubString_NoCase(NULL) == NULL);
    EZ_TEST(it.FindLastSubString_NoCase("g") == NULL);

    ezStringIterator it2(sz + 1, sz + 5, sz + 2);

    EZ_TEST(it2.FindLastSubString_NoCase("abcdef") == NULL);
    EZ_TEST(it2.FindLastSubString_NoCase("abc") == NULL);
    EZ_TEST(it2.FindLastSubString_NoCase("de") == sz + 3);
    EZ_TEST(it2.FindLastSubString_NoCase("cd") == sz + 2);
    EZ_TEST(it2.FindLastSubString_NoCase("") == NULL);
    EZ_TEST(it2.FindLastSubString_NoCase(NULL) == NULL);
    EZ_TEST(it2.FindLastSubString_NoCase("g") == NULL);
  }

  EZ_TEST_BLOCK(true, "Compare")
  {
    const char* sz = "abcdef";
    ezStringIterator it(sz);

    EZ_TEST(it.Compare("abcdef") == 0);
    EZ_TEST(it.Compare("abcde") > 0);
    EZ_TEST(it.Compare("abcdefg") < 0);

    ezStringIterator it2(sz + 2, sz + 5, sz + 3);

    EZ_TEST(it2.Compare("de") == 0);
    EZ_TEST(it2.Compare("def") < 0);
    EZ_TEST(it2.Compare("d") > 0);
  }

  EZ_TEST_BLOCK(true, "Compare_NoCase")
  {
    const char* sz = "ABCDEF";
    ezStringIterator it(sz);

    EZ_TEST(it.Compare_NoCase("abcdef") == 0);
    EZ_TEST(it.Compare_NoCase("abcde") > 0);
    EZ_TEST(it.Compare_NoCase("abcdefg") < 0);

    ezStringIterator it2(sz + 2, sz + 5, sz + 3);

    EZ_TEST(it2.Compare_NoCase("de") == 0);
    EZ_TEST(it2.Compare_NoCase("def") < 0);
    EZ_TEST(it2.Compare_NoCase("d") > 0);
  }

  EZ_TEST_BLOCK(true, "CompareN")
  {
    const char* sz = "abcdef";
    ezStringIterator it(sz);

    EZ_TEST(it.CompareN("abc", 3) == 0);
    EZ_TEST(it.CompareN("abcde", 6) > 0);
    EZ_TEST(it.CompareN("abcg", 3) == 0);

    ezStringIterator it2(sz + 2, sz + 5, sz + 2);

    EZ_TEST(it2.CompareN("cd", 2) == 0);
  }

  EZ_TEST_BLOCK(true, "CompareN_NoCase")
  {
    const char* sz = "ABCDEF";
    ezStringIterator it(sz);

    EZ_TEST(it.CompareN_NoCase("abc", 3) == 0);
    EZ_TEST(it.CompareN_NoCase("abcde", 6) > 0);
    EZ_TEST(it.CompareN_NoCase("abcg", 3) == 0);

    ezStringIterator it2(sz + 2, sz + 5, sz + 2);

    EZ_TEST(it2.CompareN_NoCase("cd", 2) == 0);
  }

  EZ_TEST_BLOCK(true, "IsEqual")
  {
    const char* sz = "abcdef";
    ezStringIterator it(sz);

    EZ_TEST(it.IsEqual("abcdef"));
    EZ_TEST(!it.IsEqual("abcde"));
    EZ_TEST(!it.IsEqual("abcdefg"));

    ezStringIterator it2(sz + 1, sz + 5, sz + 2);

    EZ_TEST(it2.IsEqual("cde"));
    EZ_TEST(!it2.IsEqual("bcde"));
    EZ_TEST(!it2.IsEqual("cdef"));
  }

  EZ_TEST_BLOCK(true, "IsEqual_NoCase")
  {
    const char* sz = "ABCDEF";
    ezStringIterator it(sz);

    EZ_TEST(it.IsEqual_NoCase("abcdef"));
    EZ_TEST(!it.IsEqual_NoCase("abcde"));
    EZ_TEST(!it.IsEqual_NoCase("abcdefg"));

    ezStringIterator it2(sz + 1, sz + 5, sz + 2);

    EZ_TEST(it2.IsEqual_NoCase("cde"));
    EZ_TEST(!it2.IsEqual_NoCase("bcde"));
    EZ_TEST(!it2.IsEqual_NoCase("cdef"));
  }

  EZ_TEST_BLOCK(true, "IsEqualN")
  {
    const char* sz = "abcdef";
    ezStringIterator it(sz);

    EZ_TEST(it.IsEqualN("abcGHI", 3));
    EZ_TEST(!it.IsEqualN("abcGHI", 4));

    ezStringIterator it2(sz + 1, sz + 5, sz + 2);

    EZ_TEST(it2.IsEqualN("cdeZX", 3));
    EZ_TEST(!it2.IsEqualN("cdeZX", 4));
  }

  EZ_TEST_BLOCK(true, "IsEqualN_NoCase")
  {
    const char* sz = "ABCDEF";
    ezStringIterator it(sz);

    EZ_TEST(it.IsEqualN_NoCase("abcGHI", 3));
    EZ_TEST(!it.IsEqualN_NoCase("abcGHI", 4));

    ezStringIterator it2(sz + 1, sz + 5, sz + 2);

    EZ_TEST(it2.IsEqualN_NoCase("cdeZX", 3));
    EZ_TEST(!it2.IsEqualN_NoCase("cdeZX", 4));
  }

  EZ_TEST_BLOCK(true, "operator==/!=")
  {
    const char* sz = "abcdef";
    const char* sz2 = "blabla";
    ezStringIterator it(sz);
    ezStringIterator it2(sz);
    ezStringIterator it3(sz2);

    EZ_TEST(it == sz);
    EZ_TEST(sz == it);
    EZ_TEST(it == "abcdef");
    EZ_TEST("abcdef" == it);
    EZ_TEST(it == it);
    EZ_TEST(it == it2);

    EZ_TEST(it != sz2);
    EZ_TEST(sz2 != it);
    EZ_TEST(it != "blabla");
    EZ_TEST("blabla" != it);
    EZ_TEST(it != it3);
  }

  EZ_TEST_BLOCK(true, "operator</>")
  {
    const char* sz = "abcdef";
    const char* sz2 = "abcdefg";
    ezStringIterator it(sz);
    ezStringIterator it2(sz2);

    EZ_TEST(it < sz2);
    EZ_TEST(sz < it2);
    EZ_TEST(it < it2);

    EZ_TEST(sz2 > it);
    EZ_TEST(it2 > sz);
    EZ_TEST(it2 > it);
  }

  EZ_TEST_BLOCK(true, "operator<=/>=")
  {
    {
      const char* sz = "abcdef";
      const char* sz2 = "abcdefg";
      ezStringIterator it(sz);
      ezStringIterator it2(sz2);

      EZ_TEST(it <= sz2);
      EZ_TEST(sz <= it2);
      EZ_TEST(it <= it2);

      EZ_TEST(sz2 >= it);
      EZ_TEST(it2 >= sz);
      EZ_TEST(it2 >= it);
    }

    {
      const char* sz = "abcdef";
      const char* sz2 = "abcdef";
      ezStringIterator it(sz);
      ezStringIterator it2(sz2);

      EZ_TEST(it <= sz2);
      EZ_TEST(sz <= it2);
      EZ_TEST(it <= it2);

      EZ_TEST(sz2 >= it);
      EZ_TEST(it2 >= sz);
      EZ_TEST(it2 >= it);
    }
  }

  EZ_TEST_BLOCK(true, "FindWholeWord")
  {
    ezStringUtf8 s = L"abc def mompfhüßß ßßß öäü abcdef abc def abc def";
    ezStringIterator it(s.GetData() + 8, s.GetData() + s.GetElementCount() - 8, s.GetData() + 8);

    EZ_TEST(it.FindWholeWord("abc", ezStringUtils::IsWordDelimiter_English) == &it.GetData()[34]);
    EZ_TEST(it.FindWholeWord("def", ezStringUtils::IsWordDelimiter_English) == &it.GetData()[38]);
    EZ_TEST(it.FindWholeWord("mompfh", ezStringUtils::IsWordDelimiter_English) == &it.GetData()[0]); // ü is not english
  }

  EZ_TEST_BLOCK(true, "FindWholeWord_NoCase")
  {
    ezStringUtf8 s = L"abc def mompfhüßß ßßß öäü abcdef abc def abc def";
    ezStringIterator it(s.GetData() + 8, s.GetData() + s.GetElementCount() - 8, s.GetData() + 8);

    EZ_TEST(it.FindWholeWord_NoCase("ABC", ezStringUtils::IsWordDelimiter_English) == &it.GetData()[34]);
    EZ_TEST(it.FindWholeWord_NoCase("DEF", ezStringUtils::IsWordDelimiter_English) == &it.GetData()[38]);
    EZ_TEST(it.FindWholeWord_NoCase("momPFH", ezStringUtils::IsWordDelimiter_English) == &it.GetData()[0]);
  }
}

