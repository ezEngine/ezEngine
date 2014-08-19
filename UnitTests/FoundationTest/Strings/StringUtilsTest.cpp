#include <PCH.h>
#include <Foundation/Strings/String.h>

EZ_CREATE_SIMPLE_TEST_GROUP(Strings);

EZ_CREATE_SIMPLE_TEST(Strings, StringUtils)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsNullOrEmpty")
  {
    EZ_TEST_BOOL(ezStringUtils::IsNullOrEmpty((char*) nullptr) == true);
    EZ_TEST_BOOL(ezStringUtils::IsNullOrEmpty("") == true);

    // all other characters are not empty
    for (ezUInt8 c = 1; c < 255; c++)
      EZ_TEST_BOOL(ezStringUtils::IsNullOrEmpty(&c) == false);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetStringElementCount")
  {
    EZ_TEST_INT(ezStringUtils::GetStringElementCount((char*) nullptr), 0);

    // Counts the Bytes
    EZ_TEST_INT(ezStringUtils::GetStringElementCount(""), 0);
    EZ_TEST_INT(ezStringUtils::GetStringElementCount("a"), 1);
    EZ_TEST_INT(ezStringUtils::GetStringElementCount("ab"), 2);
    EZ_TEST_INT(ezStringUtils::GetStringElementCount("abc"), 3);

    // Counts the number of wchar_t's
    EZ_TEST_INT(ezStringUtils::GetStringElementCount(L""), 0);
    EZ_TEST_INT(ezStringUtils::GetStringElementCount(L"a"), 1);
    EZ_TEST_INT(ezStringUtils::GetStringElementCount(L"ab"), 2);
    EZ_TEST_INT(ezStringUtils::GetStringElementCount(L"abc"), 3);

    // test with a sub-string
    const char* sz = "abc def ghi";
    EZ_TEST_INT(ezStringUtils::GetStringElementCount(sz, sz + 0), 0);
    EZ_TEST_INT(ezStringUtils::GetStringElementCount(sz, sz + 3), 3);
    EZ_TEST_INT(ezStringUtils::GetStringElementCount(sz, sz + 6), 6);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "UpdateStringEnd")
  {
    const char* sz = "Test test";
    const char* szEnd = ezMaxStringEnd;

    ezStringUtils::UpdateStringEnd(sz, szEnd);
    EZ_TEST_BOOL(szEnd == sz + ezStringUtils::GetStringElementCount(sz));

    ezStringUtils::UpdateStringEnd(sz, szEnd);
    EZ_TEST_BOOL(szEnd == sz + ezStringUtils::GetStringElementCount(sz));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetCharacterCount")
  {
    EZ_TEST_INT(ezStringUtils::GetCharacterCount(nullptr), 0);
    EZ_TEST_INT(ezStringUtils::GetCharacterCount(""), 0);
    EZ_TEST_INT(ezStringUtils::GetCharacterCount("a"), 1);
    EZ_TEST_INT(ezStringUtils::GetCharacterCount("abc"), 3);

    ezStringUtf8 s(L"äöü"); // 6 Bytes

    EZ_TEST_INT(ezStringUtils::GetStringElementCount(s.GetData()), 6);
    EZ_TEST_INT(ezStringUtils::GetCharacterCount(s.GetData()), 3);

    // test with a sub-string
    const char* sz = "abc def ghi";
    EZ_TEST_INT(ezStringUtils::GetCharacterCount(sz, sz + 0), 0);
    EZ_TEST_INT(ezStringUtils::GetCharacterCount(sz, sz + 3), 3);
    EZ_TEST_INT(ezStringUtils::GetCharacterCount(sz, sz + 6), 6);

    EZ_TEST_INT(ezStringUtils::GetCharacterCount(s.GetData(), s.GetData() + 0), 0);
    EZ_TEST_INT(ezStringUtils::GetCharacterCount(s.GetData(), s.GetData() + 2), 1);
    EZ_TEST_INT(ezStringUtils::GetCharacterCount(s.GetData(), s.GetData() + 4), 2);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetCharacterAndElementCount")
  {
    ezUInt32 uiCC, uiEC;

    ezStringUtils::GetCharacterAndElementCount(nullptr, uiCC, uiEC);
    EZ_TEST_INT(uiCC, 0);
    EZ_TEST_INT(uiEC, 0);

    ezStringUtils::GetCharacterAndElementCount("", uiCC, uiEC);
    EZ_TEST_INT(uiCC, 0);
    EZ_TEST_INT(uiEC, 0);

    ezStringUtils::GetCharacterAndElementCount("a", uiCC, uiEC);
    EZ_TEST_INT(uiCC, 1);
    EZ_TEST_INT(uiEC, 1);

    ezStringUtils::GetCharacterAndElementCount("abc", uiCC, uiEC);
    EZ_TEST_INT(uiCC, 3);
    EZ_TEST_INT(uiEC, 3);

    ezStringUtf8 s(L"äöü"); // 6 Bytes

    ezStringUtils::GetCharacterAndElementCount(s.GetData(), uiCC, uiEC);
    EZ_TEST_INT(uiCC, 3);
    EZ_TEST_INT(uiEC, 6);

    ezStringUtils::GetCharacterAndElementCount(s.GetData(), uiCC, uiEC, s.GetData() + 0);
    EZ_TEST_INT(uiCC, 0);
    EZ_TEST_INT(uiEC, 0);

    ezStringUtils::GetCharacterAndElementCount(s.GetData(), uiCC, uiEC, s.GetData() + 4);
    EZ_TEST_INT(uiCC, 2);
    EZ_TEST_INT(uiEC, 4);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Copy (full)")
  {
    char szDest[256] = "";

    // large enough
    EZ_TEST_INT(ezStringUtils::Copy(szDest, 256, "Test ABC"), 8);
    EZ_TEST_BOOL(ezStringUtils::IsEqual(szDest, "Test ABC"));

    // exactly fitting
    EZ_TEST_INT(ezStringUtils::Copy(szDest, 13, "Humpf, humpf"), 12);
    EZ_TEST_BOOL(ezStringUtils::IsEqual(szDest, "Humpf, humpf"));

    // too small
    EZ_TEST_INT(ezStringUtils::Copy(szDest, 8, "Test ABC"), 7);
    EZ_TEST_BOOL(ezStringUtils::IsEqual(szDest, "Test AB"));

    const char* szUTF8 = "ABC \xe6\x97\xa5\xd1\x88"; // contains 'ABC ' + two UTF-8 chars (first is three bytes, second is two bytes)

    // large enough
    EZ_TEST_INT(ezStringUtils::Copy(szDest, 256, szUTF8), 9);
    EZ_TEST_BOOL(ezStringUtils::IsEqual(szDest, szUTF8));

    // exactly fitting
    EZ_TEST_INT(ezStringUtils::Copy(szDest, 10, szUTF8), 9);
    EZ_TEST_BOOL(ezStringUtils::IsEqual(szDest, szUTF8));

    // too small 1
    EZ_TEST_INT(ezStringUtils::Copy(szDest, 9, szUTF8), 7);
    EZ_TEST_BOOL(ezStringUtils::IsEqualN(szDest, szUTF8, 5)); // one character less

    // too small 2
    EZ_TEST_INT(ezStringUtils::Copy(szDest, 7, szUTF8), 4);
    EZ_TEST_BOOL(ezStringUtils::IsEqualN(szDest, szUTF8, 4)); // two characters less


    // copy only from a subset
    EZ_TEST_INT(ezStringUtils::Copy(szDest, 256, szUTF8, szUTF8 + 7), 7);
    EZ_TEST_BOOL(ezStringUtils::IsEqualN(szDest, szUTF8, 5)); // two characters less
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "CopyN")
  {
    char szDest[256] = "";

    EZ_TEST_INT(ezStringUtils::CopyN(szDest, 256, "Test ABC", 4), 4);
    EZ_TEST_BOOL(ezStringUtils::IsEqual(szDest, "Test"));

    const char* szUTF8 = "ABC \xe6\x97\xa5\xd1\x88"; // contains 'ABC ' + two UTF-8 chars (first is three bytes, second is two bytes)

    EZ_TEST_INT(ezStringUtils::CopyN(szDest, 256, szUTF8, 6), 9);
    EZ_TEST_BOOL(ezStringUtils::IsEqualN(szDest, szUTF8, 6));

    EZ_TEST_INT(ezStringUtils::CopyN(szDest, 256, szUTF8, 5), 7);
    EZ_TEST_BOOL(ezStringUtils::IsEqualN(szDest, szUTF8, 5));

    EZ_TEST_INT(ezStringUtils::CopyN(szDest, 256, szUTF8, 4), 4);
    EZ_TEST_BOOL(ezStringUtils::IsEqualN(szDest, szUTF8, 4));

    EZ_TEST_INT(ezStringUtils::CopyN(szDest, 256, szUTF8, 1), 1);
    EZ_TEST_BOOL(ezStringUtils::IsEqualN(szDest, szUTF8, 1));

    EZ_TEST_INT(ezStringUtils::CopyN(szDest, 256, szUTF8, 0), 0);
    EZ_TEST_BOOL(ezStringUtils::IsEqual(szDest, ""));

    // copy only from a subset
    EZ_TEST_INT(ezStringUtils::CopyN(szDest, 256, szUTF8, 6, szUTF8 + 7), 7);
    EZ_TEST_BOOL(ezStringUtils::IsEqualN(szDest, szUTF8, 5));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ToUpperChar")
  {
    // this only tests the ASCII range
    for (ezInt32 i = 0; i < 128; ++i)
      EZ_TEST_INT(ezStringUtils::ToUpperChar(i), toupper(i));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ToLowerChar")
  {
    // this only tests the ASCII range
    for (ezInt32 i = 0; i < 128; ++i)
      EZ_TEST_INT(ezStringUtils::ToLowerChar(i), tolower(i));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ToUpperString")
  {
    ezStringUtf8 sL(L"abc öäü ß €");
    ezStringUtf8 sU(L"ABC ÖÄÜ ß €");

    char szCopy[256];
    ezStringUtils::Copy(szCopy, 256, sL.GetData());

    ezStringUtils::ToUpperString(szCopy);

    EZ_TEST_BOOL(ezStringUtils::IsEqual(szCopy, sU.GetData()));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ToLowerString")
  {
    ezStringUtf8 sL(L"abc öäü ß €");
    ezStringUtf8 sU(L"ABC ÖÄÜ ß €");

    char szCopy[256];
    ezStringUtils::Copy(szCopy, 256, sU.GetData());

    ezStringUtils::ToLowerString(szCopy);

    EZ_TEST_BOOL(ezStringUtils::IsEqual(szCopy, sL.GetData()));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "CompareChars")
  {
    EZ_TEST_BOOL(ezStringUtils::CompareChars('a', 'a') == 0); // make sure the order is right
    EZ_TEST_BOOL(ezStringUtils::CompareChars('a', 'b') < 0);  // a smaller than b -> negative
    EZ_TEST_BOOL(ezStringUtils::CompareChars('b', 'a') > 0);  // b bigger than a  -> positive
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "CompareChars(utf8)")
  {
    EZ_TEST_BOOL(ezStringUtils::CompareChars("a", "a") == 0); // make sure the order is right
    EZ_TEST_BOOL(ezStringUtils::CompareChars("a", "b") < 0);  // a smaller than b -> negative
    EZ_TEST_BOOL(ezStringUtils::CompareChars("b", "a") > 0);  // b bigger than a  -> positive
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "CompareChars_NoCase")
  {
    EZ_TEST_BOOL(ezStringUtils::CompareChars_NoCase('a', 'A') == 0);
    EZ_TEST_BOOL(ezStringUtils::CompareChars_NoCase('a', 'B') < 0);
    EZ_TEST_BOOL(ezStringUtils::CompareChars_NoCase('B', 'a') > 0);

    EZ_TEST_BOOL(ezStringUtils::CompareChars_NoCase('A', 'a') == 0);
    EZ_TEST_BOOL(ezStringUtils::CompareChars_NoCase('A', 'b') < 0);
    EZ_TEST_BOOL(ezStringUtils::CompareChars_NoCase('b', 'A') > 0);

    EZ_TEST_BOOL(ezStringUtils::CompareChars_NoCase(L'ä', L'Ä') == 0);
    EZ_TEST_BOOL(ezStringUtils::CompareChars_NoCase(L'ä', L'Ö') < 0);
    EZ_TEST_BOOL(ezStringUtils::CompareChars_NoCase(L'ö', L'Ä') > 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "CompareChars_NoCase(utf8)")
  {
    EZ_TEST_BOOL(ezStringUtils::CompareChars_NoCase("a", "A") == 0);
    EZ_TEST_BOOL(ezStringUtils::CompareChars_NoCase("a", "B") < 0);
    EZ_TEST_BOOL(ezStringUtils::CompareChars_NoCase("B", "a") > 0);

    EZ_TEST_BOOL(ezStringUtils::CompareChars_NoCase("A", "a") == 0);
    EZ_TEST_BOOL(ezStringUtils::CompareChars_NoCase("A", "b") < 0);
    EZ_TEST_BOOL(ezStringUtils::CompareChars_NoCase("b", "A") > 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsEqual")
  {
    EZ_TEST_BOOL(ezStringUtils::IsEqual(nullptr, nullptr) == true);
    EZ_TEST_BOOL(ezStringUtils::IsEqual(nullptr, "") == true);
    EZ_TEST_BOOL(ezStringUtils::IsEqual("", nullptr) == true);
    EZ_TEST_BOOL(ezStringUtils::IsEqual("", "") == true);

    EZ_TEST_BOOL(ezStringUtils::IsEqual("abc", "abc") == true);
    EZ_TEST_BOOL(ezStringUtils::IsEqual("abc", "abcd") == false);
    EZ_TEST_BOOL(ezStringUtils::IsEqual("abcd", "abc") == false);

    EZ_TEST_BOOL(ezStringUtils::IsEqual("a", nullptr) == false);
    EZ_TEST_BOOL(ezStringUtils::IsEqual(nullptr, "a") == false);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsEqualN")
  {
    EZ_TEST_BOOL(ezStringUtils::IsEqualN(nullptr, nullptr, 1) == true);
    EZ_TEST_BOOL(ezStringUtils::IsEqualN(nullptr, "", 1) == true);
    EZ_TEST_BOOL(ezStringUtils::IsEqualN("", nullptr, 1) == true);
    EZ_TEST_BOOL(ezStringUtils::IsEqualN("", "", 1) == true);

    // as long as we compare 'nothing' the strings must be equal
    EZ_TEST_BOOL(ezStringUtils::IsEqualN("abc", nullptr, 0) == true);
    EZ_TEST_BOOL(ezStringUtils::IsEqualN("abc", "", 0) == true);
    EZ_TEST_BOOL(ezStringUtils::IsEqualN(nullptr, "abc", 0) == true);
    EZ_TEST_BOOL(ezStringUtils::IsEqualN("", "abc", 0) == true);

    EZ_TEST_BOOL(ezStringUtils::IsEqualN("abc", "abcdef", 1) == true);
    EZ_TEST_BOOL(ezStringUtils::IsEqualN("abc", "abcdef", 2) == true);
    EZ_TEST_BOOL(ezStringUtils::IsEqualN("abc", "abcdef", 3) == true);
    EZ_TEST_BOOL(ezStringUtils::IsEqualN("abc", "abcdef", 4) == false);

    EZ_TEST_BOOL(ezStringUtils::IsEqualN("abcdef", "abc", 1) == true);
    EZ_TEST_BOOL(ezStringUtils::IsEqualN("abcdef", "abc", 2) == true);
    EZ_TEST_BOOL(ezStringUtils::IsEqualN("abcdef", "abc", 3) == true);
    EZ_TEST_BOOL(ezStringUtils::IsEqualN("abcdef", "abc", 4) == false);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsEqual_NoCase")
  {
    EZ_TEST_BOOL(ezStringUtils::IsEqual_NoCase(nullptr, nullptr) == true);
    EZ_TEST_BOOL(ezStringUtils::IsEqual_NoCase(nullptr, "") == true);
    EZ_TEST_BOOL(ezStringUtils::IsEqual_NoCase("", nullptr) == true);
    EZ_TEST_BOOL(ezStringUtils::IsEqual_NoCase("", "") == true);


    ezStringUtf8 sL(L"abc öäü ß €");
    ezStringUtf8 sU(L"ABC ÖÄÜ ß €");
    ezStringUtf8 sU2(L"ABC ÖÄÜ ß € ");

    EZ_TEST_BOOL(ezStringUtils::IsEqual_NoCase(sL.GetData(), sU.GetData()) == true);
    EZ_TEST_BOOL(ezStringUtils::IsEqual_NoCase(sL.GetData(), sU2.GetData()) == false);
    EZ_TEST_BOOL(ezStringUtils::IsEqual_NoCase(sU2.GetData(), sL.GetData()) == false);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsEqualN_NoCase")
  {
    EZ_TEST_BOOL(ezStringUtils::IsEqualN_NoCase(nullptr, nullptr, 1) == true);
    EZ_TEST_BOOL(ezStringUtils::IsEqualN_NoCase(nullptr, "", 1) == true);
    EZ_TEST_BOOL(ezStringUtils::IsEqualN_NoCase("", nullptr, 1) == true);
    EZ_TEST_BOOL(ezStringUtils::IsEqualN_NoCase("", "", 1) == true);

    // as long as we compare 'nothing' the strings must be equal
    EZ_TEST_BOOL(ezStringUtils::IsEqualN_NoCase("abc", nullptr, 0) == true);
    EZ_TEST_BOOL(ezStringUtils::IsEqualN_NoCase("abc", "", 0) == true);
    EZ_TEST_BOOL(ezStringUtils::IsEqualN_NoCase(nullptr, "abc", 0) == true);
    EZ_TEST_BOOL(ezStringUtils::IsEqualN_NoCase("", "abc", 0) == true);

    ezStringUtf8 sL(L"abc öäü ß €");
    ezStringUtf8 sU(L"ABC ÖÄÜ ß € moep");

    for (ezInt32 i = 0; i < 12; ++i)
      EZ_TEST_BOOL(ezStringUtils::IsEqualN_NoCase(sL.GetData(), sU.GetData(), i) == true);
    EZ_TEST_BOOL(ezStringUtils::IsEqualN_NoCase(sL.GetData(), sU.GetData(), 12) == false);

    for (ezInt32 i = 0; i < 12; ++i)
      EZ_TEST_BOOL(ezStringUtils::IsEqualN_NoCase(sU.GetData(), sL.GetData(), i) == true);
    EZ_TEST_BOOL(ezStringUtils::IsEqualN_NoCase(sU.GetData(), sL.GetData(), 12) == false);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Compare")
  {
    EZ_TEST_BOOL(ezStringUtils::Compare(nullptr, nullptr) == 0);
    EZ_TEST_BOOL(ezStringUtils::Compare(nullptr, "") == 0);
    EZ_TEST_BOOL(ezStringUtils::Compare("", nullptr) == 0);
    EZ_TEST_BOOL(ezStringUtils::Compare("", "") == 0);

    EZ_TEST_BOOL(ezStringUtils::Compare("abc", "abc") == 0);
    EZ_TEST_BOOL(ezStringUtils::Compare("abc", "abcd") < 0);
    EZ_TEST_BOOL(ezStringUtils::Compare("abcd", "abc") > 0);

    EZ_TEST_BOOL(ezStringUtils::Compare("a", nullptr) > 0);
    EZ_TEST_BOOL(ezStringUtils::Compare(nullptr, "a") < 0);

    // substring compare
    const char* sz = "abc def ghi bla";
    EZ_TEST_BOOL(ezStringUtils::Compare(sz, "abc", sz + 3) == 0);
    EZ_TEST_BOOL(ezStringUtils::Compare(sz, "abc def", sz + 7) == 0);
    EZ_TEST_BOOL(ezStringUtils::Compare(sz, sz, sz + 7, sz + 7) == 0);
    EZ_TEST_BOOL(ezStringUtils::Compare(sz, sz, sz + 7, sz + 6) > 0);
    EZ_TEST_BOOL(ezStringUtils::Compare(sz, sz, sz + 7, sz + 8) < 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "CompareN")
  {
    EZ_TEST_BOOL(ezStringUtils::CompareN(nullptr, nullptr, 1) == 0);
    EZ_TEST_BOOL(ezStringUtils::CompareN(nullptr, "", 1) == 0);
    EZ_TEST_BOOL(ezStringUtils::CompareN("", nullptr, 1) == 0);
    EZ_TEST_BOOL(ezStringUtils::CompareN("", "", 1) == 0);

    // as long as we compare 'nothing' the strings must be equal
    EZ_TEST_BOOL(ezStringUtils::CompareN("abc", nullptr, 0) == 0);
    EZ_TEST_BOOL(ezStringUtils::CompareN("abc", "", 0) == 0);
    EZ_TEST_BOOL(ezStringUtils::CompareN(nullptr, "abc", 0) == 0);
    EZ_TEST_BOOL(ezStringUtils::CompareN("", "abc", 0) == 0);

    EZ_TEST_BOOL(ezStringUtils::CompareN("abc", "abcdef", 1) == 0);
    EZ_TEST_BOOL(ezStringUtils::CompareN("abc", "abcdef", 2) == 0);
    EZ_TEST_BOOL(ezStringUtils::CompareN("abc", "abcdef", 3) == 0);
    EZ_TEST_BOOL(ezStringUtils::CompareN("abc", "abcdef", 4) < 0);

    EZ_TEST_BOOL(ezStringUtils::CompareN("abcdef", "abc", 1) == 0);
    EZ_TEST_BOOL(ezStringUtils::CompareN("abcdef", "abc", 2) == 0);
    EZ_TEST_BOOL(ezStringUtils::CompareN("abcdef", "abc", 3) == 0);
    EZ_TEST_BOOL(ezStringUtils::CompareN("abcdef", "abc", 4) > 0);

    // substring compare
    const char* sz = "abc def ghi bla";
    EZ_TEST_BOOL(ezStringUtils::CompareN(sz, "abc", 10, sz + 3) == 0);
    EZ_TEST_BOOL(ezStringUtils::CompareN(sz, "abc def", 10, sz + 7) == 0);
    EZ_TEST_BOOL(ezStringUtils::CompareN(sz, sz, 10, sz + 7, sz + 7) == 0);
    EZ_TEST_BOOL(ezStringUtils::CompareN(sz, sz, 10, sz + 7, sz + 6) > 0);
    EZ_TEST_BOOL(ezStringUtils::CompareN(sz, sz, 10, sz + 7, sz + 8) < 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Compare_NoCase")
  {
    EZ_TEST_BOOL(ezStringUtils::Compare_NoCase(nullptr, nullptr) == 0);
    EZ_TEST_BOOL(ezStringUtils::Compare_NoCase(nullptr, "") == 0);
    EZ_TEST_BOOL(ezStringUtils::Compare_NoCase("", nullptr) == 0);
    EZ_TEST_BOOL(ezStringUtils::Compare_NoCase("", "") == 0);

    EZ_TEST_BOOL(ezStringUtils::Compare_NoCase("abc", "aBc") == 0);
    EZ_TEST_BOOL(ezStringUtils::Compare_NoCase("ABC", "abcd") < 0);
    EZ_TEST_BOOL(ezStringUtils::Compare_NoCase("abcd", "ABC") > 0);

    EZ_TEST_BOOL(ezStringUtils::Compare_NoCase("a", nullptr) > 0);
    EZ_TEST_BOOL(ezStringUtils::Compare_NoCase(nullptr, "a") < 0);

    // substring compare
    const char* sz = "abc def ghi bla";
    EZ_TEST_BOOL(ezStringUtils::Compare_NoCase(sz, "ABC", sz + 3) == 0);
    EZ_TEST_BOOL(ezStringUtils::Compare_NoCase(sz, "ABC def", sz + 7) == 0);
    EZ_TEST_BOOL(ezStringUtils::Compare_NoCase(sz, sz, sz + 7, sz + 7) == 0);
    EZ_TEST_BOOL(ezStringUtils::Compare_NoCase(sz, sz, sz + 7, sz + 6) > 0);
    EZ_TEST_BOOL(ezStringUtils::Compare_NoCase(sz, sz, sz + 7, sz + 8) < 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "CompareN_NoCase")
  {
    EZ_TEST_BOOL(ezStringUtils::CompareN_NoCase(nullptr, nullptr, 1) == 0);
    EZ_TEST_BOOL(ezStringUtils::CompareN_NoCase(nullptr, "", 1) == 0);
    EZ_TEST_BOOL(ezStringUtils::CompareN_NoCase("", nullptr, 1) == 0);
    EZ_TEST_BOOL(ezStringUtils::CompareN_NoCase("", "", 1) == 0);

    // as long as we compare 'nothing' the strings must be equal
    EZ_TEST_BOOL(ezStringUtils::CompareN_NoCase("abc", nullptr, 0) == 0);
    EZ_TEST_BOOL(ezStringUtils::CompareN_NoCase("abc", "", 0) == 0);
    EZ_TEST_BOOL(ezStringUtils::CompareN_NoCase(nullptr, "abc", 0) == 0);
    EZ_TEST_BOOL(ezStringUtils::CompareN_NoCase("", "abc", 0) == 0);

    EZ_TEST_BOOL(ezStringUtils::CompareN_NoCase("aBc", "abcdef", 1) == 0);
    EZ_TEST_BOOL(ezStringUtils::CompareN_NoCase("aBc", "abcdef", 2) == 0);
    EZ_TEST_BOOL(ezStringUtils::CompareN_NoCase("aBc", "abcdef", 3) == 0);
    EZ_TEST_BOOL(ezStringUtils::CompareN_NoCase("aBc", "abcdef", 4) < 0);

    EZ_TEST_BOOL(ezStringUtils::CompareN_NoCase("abcdef", "Abc", 1) == 0);
    EZ_TEST_BOOL(ezStringUtils::CompareN_NoCase("abcdef", "Abc", 2) == 0);
    EZ_TEST_BOOL(ezStringUtils::CompareN_NoCase("abcdef", "Abc", 3) == 0);
    EZ_TEST_BOOL(ezStringUtils::CompareN_NoCase("abcdef", "Abc", 4) > 0);

    // substring compare
    const char* sz = "abc def ghi bla";
    EZ_TEST_BOOL(ezStringUtils::CompareN_NoCase(sz, "ABC", 10, sz + 3) == 0);
    EZ_TEST_BOOL(ezStringUtils::CompareN_NoCase(sz, "ABC def", 10, sz + 7) == 0);
    EZ_TEST_BOOL(ezStringUtils::CompareN_NoCase(sz, sz, 10, sz + 7, sz + 7) == 0);
    EZ_TEST_BOOL(ezStringUtils::CompareN_NoCase(sz, sz, 10, sz + 7, sz + 6) > 0);
    EZ_TEST_BOOL(ezStringUtils::CompareN_NoCase(sz, sz, 10, sz + 7, sz + 8) < 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "snprintf")
  {
    // This function has been tested to death during its implementation.
    // That test-code would require several pages, if one would try to test it properly.
    // I am not going to do that here, I am quite confident the function works as expected with pure ASCII strings.
    // So I'm only testing a bit of Utf8 stuff.

    ezStringUtf8 s(L"Abc %s äöü ß %i %s %.4f");
    ezStringUtf8 s2(L"ÄÖÜ");

    char sz[256];
    ezStringUtils::snprintf(sz, 256, s.GetData(), "ASCII", 42, s2.GetData(), 23.31415);

    ezStringUtf8 sC(L"Abc ASCII äöü ß 42 ÄÖÜ 23.3142"); // notice the correct float rounding ;-)

    EZ_TEST_STRING(sz, sC.GetData());


    // NaN and Infinity
    ezStringUtils::snprintf(sz, 256, "NaN Value: %.2f", ezMath::BasicType<float>::GetNaN());
    EZ_TEST_STRING(sz, "NaN Value: NaN");

    ezStringUtils::snprintf(sz, 256, "Inf Value: %.2f", +ezMath::BasicType<float>::GetInfinity());
    EZ_TEST_STRING(sz, "Inf Value: Infinity");

    ezStringUtils::snprintf(sz, 256, "Inf Value: %.2f", -ezMath::BasicType<float>::GetInfinity());
    EZ_TEST_STRING(sz, "Inf Value: -Infinity");

    ezStringUtils::snprintf(sz, 256, "NaN Value: %.2e", ezMath::BasicType<float>::GetNaN());
    EZ_TEST_STRING(sz, "NaN Value: NaN");

    ezStringUtils::snprintf(sz, 256, "Inf Value: %.2e", +ezMath::BasicType<float>::GetInfinity());
    EZ_TEST_STRING(sz, "Inf Value: Infinity");

    ezStringUtils::snprintf(sz, 256, "Inf Value: %.2e", -ezMath::BasicType<float>::GetInfinity());
    EZ_TEST_STRING(sz, "Inf Value: -Infinity");

    ezStringUtils::snprintf(sz, 256, "NaN Value: %+10.2f", ezMath::BasicType<float>::GetNaN());
    EZ_TEST_STRING(sz, "NaN Value:       +NaN");

    ezStringUtils::snprintf(sz, 256, "Inf Value: %+10.2f", +ezMath::BasicType<float>::GetInfinity());
    EZ_TEST_STRING(sz, "Inf Value:  +Infinity");

    ezStringUtils::snprintf(sz, 256, "Inf Value: %+10.2f", -ezMath::BasicType<float>::GetInfinity());
    EZ_TEST_STRING(sz, "Inf Value:  -Infinity");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "StartsWith")
  {
    EZ_TEST_BOOL(ezStringUtils::StartsWith(nullptr, nullptr) == true);
    EZ_TEST_BOOL(ezStringUtils::StartsWith(nullptr, "") == true);
    EZ_TEST_BOOL(ezStringUtils::StartsWith("", nullptr) == true);
    EZ_TEST_BOOL(ezStringUtils::StartsWith("", "") == true);

    EZ_TEST_BOOL(ezStringUtils::StartsWith("abc", nullptr) == true);
    EZ_TEST_BOOL(ezStringUtils::StartsWith("abc", "") == true);
    EZ_TEST_BOOL(ezStringUtils::StartsWith(nullptr, "abc") == false);
    EZ_TEST_BOOL(ezStringUtils::StartsWith("", "abc") == false);

    EZ_TEST_BOOL(ezStringUtils::StartsWith("abc", "abc") == true);
    EZ_TEST_BOOL(ezStringUtils::StartsWith("abcdef", "abc") == true);
    EZ_TEST_BOOL(ezStringUtils::StartsWith("abcdef", "Abc") == false);

    // substring test
    const char* sz = "abc def ghi";
    EZ_TEST_BOOL(ezStringUtils::StartsWith(sz, "abc", sz + 3) == true);
    EZ_TEST_BOOL(ezStringUtils::StartsWith(sz, "abc", sz + 2) == false);
    EZ_TEST_BOOL(ezStringUtils::StartsWith(sz, "abc", sz + 0) == false);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "StartsWith_NoCase")
  {
    ezStringUtf8 sL(L"äöü");
    ezStringUtf8 sU(L"ÄÖÜ");

    EZ_TEST_BOOL(ezStringUtils::StartsWith_NoCase(nullptr, nullptr) == true);
    EZ_TEST_BOOL(ezStringUtils::StartsWith_NoCase(nullptr, "") == true);
    EZ_TEST_BOOL(ezStringUtils::StartsWith_NoCase("", nullptr) == true);
    EZ_TEST_BOOL(ezStringUtils::StartsWith_NoCase("", "") == true);

    EZ_TEST_BOOL(ezStringUtils::StartsWith_NoCase("abc", nullptr) == true);
    EZ_TEST_BOOL(ezStringUtils::StartsWith_NoCase("abc", "") == true);
    EZ_TEST_BOOL(ezStringUtils::StartsWith_NoCase(nullptr, "abc") == false);
    EZ_TEST_BOOL(ezStringUtils::StartsWith_NoCase("", "abc") == false);

    EZ_TEST_BOOL(ezStringUtils::StartsWith_NoCase("abc", "ABC") == true);
    EZ_TEST_BOOL(ezStringUtils::StartsWith_NoCase("aBCdef", "abc") == true);
    EZ_TEST_BOOL(ezStringUtils::StartsWith_NoCase("aBCdef", "bc") == false);

    EZ_TEST_BOOL(ezStringUtils::StartsWith_NoCase(sL.GetData(), sU.GetData()) == true);

    // substring test
    const char* sz = "abc def ghi";
    EZ_TEST_BOOL(ezStringUtils::StartsWith_NoCase(sz, "ABC", sz + 3) == true);
    EZ_TEST_BOOL(ezStringUtils::StartsWith_NoCase(sz, "ABC", sz + 2) == false);
    EZ_TEST_BOOL(ezStringUtils::StartsWith_NoCase(sz, "ABC", sz + 0) == false);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "EndsWith")
  {
    EZ_TEST_BOOL(ezStringUtils::EndsWith(nullptr, nullptr) == true);
    EZ_TEST_BOOL(ezStringUtils::EndsWith(nullptr, "") == true);
    EZ_TEST_BOOL(ezStringUtils::EndsWith("", nullptr) == true);
    EZ_TEST_BOOL(ezStringUtils::EndsWith("", "") == true);

    EZ_TEST_BOOL(ezStringUtils::EndsWith("abc", nullptr) == true);
    EZ_TEST_BOOL(ezStringUtils::EndsWith("abc", "") == true);
    EZ_TEST_BOOL(ezStringUtils::EndsWith(nullptr, "abc") == false);
    EZ_TEST_BOOL(ezStringUtils::EndsWith("", "abc") == false);

    EZ_TEST_BOOL(ezStringUtils::EndsWith("abc", "abc") == true);
    EZ_TEST_BOOL(ezStringUtils::EndsWith("abcdef", "def") == true);
    EZ_TEST_BOOL(ezStringUtils::EndsWith("abcdef", "Def") == false);
    EZ_TEST_BOOL(ezStringUtils::EndsWith("def", "abcdef") == false);

    // substring test
    const char* sz = "abc def ghi";
    EZ_TEST_BOOL(ezStringUtils::EndsWith(sz, "abc", sz + 3) == true);
    EZ_TEST_BOOL(ezStringUtils::EndsWith(sz, "def", sz + 7) == true);
    EZ_TEST_BOOL(ezStringUtils::EndsWith(sz, "def", sz + 8) == false);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "EndsWith_NoCase")
  {
    ezStringUtf8 sL(L"äöü");
    ezStringUtf8 sU(L"ÄÖÜ");

    EZ_TEST_BOOL(ezStringUtils::EndsWith_NoCase(nullptr, nullptr) == true);
    EZ_TEST_BOOL(ezStringUtils::EndsWith_NoCase(nullptr, "") == true);
    EZ_TEST_BOOL(ezStringUtils::EndsWith_NoCase("", nullptr) == true);
    EZ_TEST_BOOL(ezStringUtils::EndsWith_NoCase("", "") == true);

    EZ_TEST_BOOL(ezStringUtils::EndsWith_NoCase("abc", nullptr) == true);
    EZ_TEST_BOOL(ezStringUtils::EndsWith_NoCase("abc", "") == true);
    EZ_TEST_BOOL(ezStringUtils::EndsWith_NoCase(nullptr, "abc") == false);
    EZ_TEST_BOOL(ezStringUtils::EndsWith_NoCase("", "abc") == false);

    EZ_TEST_BOOL(ezStringUtils::EndsWith_NoCase("abc", "abc") == true);
    EZ_TEST_BOOL(ezStringUtils::EndsWith_NoCase("abcdef", "def") == true);
    EZ_TEST_BOOL(ezStringUtils::EndsWith_NoCase("abcdef", "Def") == true);

    EZ_TEST_BOOL(ezStringUtils::EndsWith_NoCase("def", "abcdef") == false);

    EZ_TEST_BOOL(ezStringUtils::EndsWith_NoCase(sL.GetData(), sU.GetData()) == true);

    // substring test
    const char* sz = "abc def ghi";
    EZ_TEST_BOOL(ezStringUtils::EndsWith_NoCase(sz, "ABC", sz + 3) == true);
    EZ_TEST_BOOL(ezStringUtils::EndsWith_NoCase(sz, "DEF", sz + 7) == true);
    EZ_TEST_BOOL(ezStringUtils::EndsWith_NoCase(sz, "DEF", sz + 8) == false);  
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "FindSubString")
  {
    ezStringUtf8 s(L"abc def ghi äöü jkl ßßß abc2 def2 ghi2 äöü2 ß");
    ezStringUtf8 s2(L"äöü");
    ezStringUtf8 s3(L"äöü2");

    const char* szABC = "abc";

    EZ_TEST_BOOL(ezStringUtils::FindSubString(szABC, szABC) == szABC);
    EZ_TEST_BOOL(ezStringUtils::FindSubString("abc", "") == nullptr);
    EZ_TEST_BOOL(ezStringUtils::FindSubString("abc", nullptr) == nullptr);
    EZ_TEST_BOOL(ezStringUtils::FindSubString(nullptr, "abc") == nullptr);
    EZ_TEST_BOOL(ezStringUtils::FindSubString("", "abc") == nullptr);

    EZ_TEST_BOOL(ezStringUtils::FindSubString(s.GetData(), "abc") == s.GetData());
    EZ_TEST_BOOL(ezStringUtils::FindSubString(s.GetData(), "def") == &s.GetData()[4]);
    EZ_TEST_BOOL(ezStringUtils::FindSubString(s.GetData(), "ghi") == &s.GetData()[8]);
    EZ_TEST_BOOL(ezStringUtils::FindSubString(s.GetData(), s2.GetData()) == &s.GetData()[12]);

    EZ_TEST_BOOL(ezStringUtils::FindSubString(s.GetData(), "abc2") == &s.GetData()[30]);
    EZ_TEST_BOOL(ezStringUtils::FindSubString(s.GetData(), "def2") == &s.GetData()[35]);
    EZ_TEST_BOOL(ezStringUtils::FindSubString(s.GetData(), "ghi2") == &s.GetData()[40]);
    EZ_TEST_BOOL(ezStringUtils::FindSubString(s.GetData(), s3.GetData()) == &s.GetData()[45]);

    // substring test
    EZ_TEST_BOOL(ezStringUtils::FindSubString(s.GetData(), "abc2", s.GetData() + 34) == &s.GetData()[30]);
    EZ_TEST_BOOL(ezStringUtils::FindSubString(s.GetData(), "abc2", s.GetData() + 33) == nullptr);
  }
  
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "FindSubString_NoCase")
  {
    ezStringUtf8 s(L"abc def ghi äöü jkl ßßß abc2 def2 ghi2 äöü2 ß");
    ezStringUtf8 s2(L"äÖü");
    ezStringUtf8 s3(L"ÄöÜ2");

    const char* szABC = "abc";

    EZ_TEST_BOOL(ezStringUtils::FindSubString_NoCase(szABC, "aBc") == szABC);
    EZ_TEST_BOOL(ezStringUtils::FindSubString_NoCase("abc", "") == nullptr);
    EZ_TEST_BOOL(ezStringUtils::FindSubString_NoCase("abc", nullptr) == nullptr);
    EZ_TEST_BOOL(ezStringUtils::FindSubString_NoCase(nullptr, "abc") == nullptr);
    EZ_TEST_BOOL(ezStringUtils::FindSubString_NoCase("", "abc") == nullptr);

    EZ_TEST_BOOL(ezStringUtils::FindSubString_NoCase(s.GetData(), "Abc") == s.GetData());
    EZ_TEST_BOOL(ezStringUtils::FindSubString_NoCase(s.GetData(), "dEf") == &s.GetData()[4]);
    EZ_TEST_BOOL(ezStringUtils::FindSubString_NoCase(s.GetData(), "ghI") == &s.GetData()[8]);
    EZ_TEST_BOOL(ezStringUtils::FindSubString_NoCase(s.GetData(), s2.GetData()) == &s.GetData()[12]);

    EZ_TEST_BOOL(ezStringUtils::FindSubString_NoCase(s.GetData(), "abC2") == &s.GetData()[30]);
    EZ_TEST_BOOL(ezStringUtils::FindSubString_NoCase(s.GetData(), "dEf2") == &s.GetData()[35]);
    EZ_TEST_BOOL(ezStringUtils::FindSubString_NoCase(s.GetData(), "Ghi2") == &s.GetData()[40]);
    EZ_TEST_BOOL(ezStringUtils::FindSubString_NoCase(s.GetData(), s3.GetData()) == &s.GetData()[45]);

    // substring test
    EZ_TEST_BOOL(ezStringUtils::FindSubString_NoCase(s.GetData(), "aBc2", s.GetData() + 34) == &s.GetData()[30]);
    EZ_TEST_BOOL(ezStringUtils::FindSubString_NoCase(s.GetData(), "abC2", s.GetData() + 33) == nullptr);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "FindLastSubString")
  {
    ezStringUtf8 s(L"abc def ghi äöü jkl ßßß abc2 def2 ghi2 äöü2 ß");
    ezStringUtf8 s2(L"äöü");
    ezStringUtf8 s3(L"äöü2");

    const char* szABC = "abc";

    EZ_TEST_BOOL(ezStringUtils::FindLastSubString(szABC, szABC) == szABC);
    EZ_TEST_BOOL(ezStringUtils::FindLastSubString("abc", "") == nullptr);
    EZ_TEST_BOOL(ezStringUtils::FindLastSubString("abc", nullptr) == nullptr);
    EZ_TEST_BOOL(ezStringUtils::FindLastSubString(nullptr, "abc") == nullptr);
    EZ_TEST_BOOL(ezStringUtils::FindLastSubString("", "abc") == nullptr);

    EZ_TEST_BOOL(ezStringUtils::FindLastSubString(s.GetData(), "abc") == &s.GetData()[30]);
    EZ_TEST_BOOL(ezStringUtils::FindLastSubString(s.GetData(), "def") == &s.GetData()[35]);
    EZ_TEST_BOOL(ezStringUtils::FindLastSubString(s.GetData(), "ghi") == &s.GetData()[40]);
    EZ_TEST_BOOL(ezStringUtils::FindLastSubString(s.GetData(), s2.GetData()) == &s.GetData()[45]);

    // substring test
    EZ_TEST_BOOL(ezStringUtils::FindLastSubString(s.GetData(), "abc", nullptr, s.GetData() + 33) == &s.GetData()[30]);
    EZ_TEST_BOOL(ezStringUtils::FindLastSubString(s.GetData(), "abc", nullptr, s.GetData() + 32) == &s.GetData()[0]);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "FindLastSubString_NoCase")
  {
    ezStringUtf8 s(L"abc def ghi äöü jkl ßßß abc2 def2 ghi2 äöü2 ß");
    ezStringUtf8 s2(L"äÖü");
    ezStringUtf8 s3(L"ÄöÜ2");

    const char* szABC = "abc";

    EZ_TEST_BOOL(ezStringUtils::FindLastSubString_NoCase(szABC, "aBC") == szABC);
    EZ_TEST_BOOL(ezStringUtils::FindLastSubString_NoCase("abc", "") == nullptr);
    EZ_TEST_BOOL(ezStringUtils::FindLastSubString_NoCase("abc", nullptr) == nullptr);
    EZ_TEST_BOOL(ezStringUtils::FindLastSubString_NoCase(nullptr, "abc") == nullptr);
    EZ_TEST_BOOL(ezStringUtils::FindLastSubString_NoCase("", "abc") == nullptr);

    EZ_TEST_BOOL(ezStringUtils::FindLastSubString_NoCase(s.GetData(), "Abc") == &s.GetData()[30]);
    EZ_TEST_BOOL(ezStringUtils::FindLastSubString_NoCase(s.GetData(), "dEf") == &s.GetData()[35]);
    EZ_TEST_BOOL(ezStringUtils::FindLastSubString_NoCase(s.GetData(), "ghI") == &s.GetData()[40]);
    EZ_TEST_BOOL(ezStringUtils::FindLastSubString_NoCase(s.GetData(), s2.GetData()) == &s.GetData()[45]);

    // substring test
    EZ_TEST_BOOL(ezStringUtils::FindLastSubString_NoCase(s.GetData(), "ABC", nullptr, s.GetData() + 33) == &s.GetData()[30]);
    EZ_TEST_BOOL(ezStringUtils::FindLastSubString_NoCase(s.GetData(), "ABC", nullptr, s.GetData() + 32) == &s.GetData()[0]);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "FindWholeWord")
  {
    ezStringUtf8 s(L"mompfhüßß ßßß öäü abcdef abc def");

    EZ_TEST_BOOL(ezStringUtils::FindWholeWord(s.GetData(), "abc", ezStringUtils::IsWordDelimiter_English) == &s.GetData()[34]);
    EZ_TEST_BOOL(ezStringUtils::FindWholeWord(s.GetData(), "def", ezStringUtils::IsWordDelimiter_English) == &s.GetData()[38]);
    EZ_TEST_BOOL(ezStringUtils::FindWholeWord(s.GetData(), "mompfh", ezStringUtils::IsWordDelimiter_English) == &s.GetData()[0]); // ü is not english

    // substring test
    EZ_TEST_BOOL(ezStringUtils::FindWholeWord(s.GetData(), "abc", ezStringUtils::IsWordDelimiter_English, s.GetData() + 37) == &s.GetData()[34]);
    EZ_TEST_BOOL(ezStringUtils::FindWholeWord(s.GetData(), "abc", ezStringUtils::IsWordDelimiter_English, s.GetData() + 36) == nullptr);
    EZ_TEST_BOOL(ezStringUtils::FindWholeWord(s.GetData(), "abc", ezStringUtils::IsWordDelimiter_English, s.GetData() + 30) == s.GetData() + 27);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "FindWholeWord_NoCase")
  {
    ezStringUtf8 s(L"mompfhüßß ßßß öäü abcdef abc def");

    EZ_TEST_BOOL(ezStringUtils::FindWholeWord_NoCase(s.GetData(), "ABC", ezStringUtils::IsWordDelimiter_English) == &s.GetData()[34]);
    EZ_TEST_BOOL(ezStringUtils::FindWholeWord_NoCase(s.GetData(), "DEF", ezStringUtils::IsWordDelimiter_English) == &s.GetData()[38]);
    EZ_TEST_BOOL(ezStringUtils::FindWholeWord_NoCase(s.GetData(), "momPFH", ezStringUtils::IsWordDelimiter_English) == &s.GetData()[0]);

    // substring test
    EZ_TEST_BOOL(ezStringUtils::FindWholeWord_NoCase(s.GetData(), "ABC", ezStringUtils::IsWordDelimiter_English, s.GetData() + 37) == &s.GetData()[34]);
    EZ_TEST_BOOL(ezStringUtils::FindWholeWord_NoCase(s.GetData(), "ABC", ezStringUtils::IsWordDelimiter_English, s.GetData() + 36) == nullptr);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SkipCharacters")
  {
    ezStringUtf8 s(L"mompf   hüßß ßßß öäü abcdef abc def");
    const char* szEmpty = "";

    EZ_TEST_BOOL(ezStringUtils::SkipCharacters(s.GetData(), ezStringUtils::IsWhiteSpace, false) == &s.GetData()[0]);
    EZ_TEST_BOOL(ezStringUtils::SkipCharacters(s.GetData(), ezStringUtils::IsWhiteSpace, true ) == &s.GetData()[1]);
    EZ_TEST_BOOL(ezStringUtils::SkipCharacters(&s.GetData()[5], ezStringUtils::IsWhiteSpace, false) == &s.GetData()[8]);
    EZ_TEST_BOOL(ezStringUtils::SkipCharacters(&s.GetData()[5], ezStringUtils::IsWhiteSpace, true ) == &s.GetData()[8]);
    EZ_TEST_BOOL(ezStringUtils::SkipCharacters(szEmpty, ezStringUtils::IsWhiteSpace, false) == szEmpty);
    EZ_TEST_BOOL(ezStringUtils::SkipCharacters(szEmpty, ezStringUtils::IsWhiteSpace, true) == szEmpty);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "FindWordEnd")
  {
    ezStringUtf8 s(L"mompf   hüßß ßßß öäü abcdef abc def");
    const char* szEmpty = "";

    EZ_TEST_BOOL(ezStringUtils::FindWordEnd(s.GetData(), ezStringUtils::IsWhiteSpace, true ) == &s.GetData()[5]);
    EZ_TEST_BOOL(ezStringUtils::FindWordEnd(s.GetData(), ezStringUtils::IsWhiteSpace, false) == &s.GetData()[5]);
    EZ_TEST_BOOL(ezStringUtils::FindWordEnd(&s.GetData()[5], ezStringUtils::IsWhiteSpace, true ) == &s.GetData()[6]);
    EZ_TEST_BOOL(ezStringUtils::FindWordEnd(&s.GetData()[5], ezStringUtils::IsWhiteSpace, false) == &s.GetData()[5]);
    EZ_TEST_BOOL(ezStringUtils::FindWordEnd(szEmpty, ezStringUtils::IsWhiteSpace, true ) == szEmpty);
    EZ_TEST_BOOL(ezStringUtils::FindWordEnd(szEmpty, ezStringUtils::IsWhiteSpace, false) == szEmpty);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsWhitespace")
  {
    EZ_TEST_BOOL(ezStringUtils::IsWhiteSpace(' '));
    EZ_TEST_BOOL(ezStringUtils::IsWhiteSpace('\t'));
    EZ_TEST_BOOL(ezStringUtils::IsWhiteSpace('\n'));
    EZ_TEST_BOOL(ezStringUtils::IsWhiteSpace('\r'));
    EZ_TEST_BOOL(ezStringUtils::IsWhiteSpace('\v'));

    for (ezUInt32 i = 0; i < 256; ++i)
    {
      if (i == ' ' ||
          i == '\t' ||
          i == '\v' ||
          i == '\n' ||
          i == '\r')
          continue;

      EZ_TEST_BOOL(ezStringUtils::IsWhiteSpace(i) == false);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsWordDelimiter_English / IsIdentifierDelimiter_C_Code")
  {
    for (ezUInt32 i = 0; i < 256; ++i)
    {
      const bool alpha = (i >= 'a' && i <= 'z');
      const bool alpha2 = (i >= 'A' && i <= 'Z');
      const bool num = (i >= '0' && i <= '9');
      const bool dash = i == '-';
      const bool underscore = i == '_';

      const bool bCode = alpha || alpha2 || num || underscore;
      const bool bWord = bCode || dash;
      

      EZ_TEST_BOOL(ezStringUtils::IsWordDelimiter_English(i) == !bWord);
      EZ_TEST_BOOL(ezStringUtils::IsIdentifierDelimiter_C_Code(i) == !bCode);
    }
  }
}


