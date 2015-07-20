#include <PCH.h>
#include <Foundation/Strings/String.h>

EZ_CREATE_SIMPLE_TEST(Strings, StringView)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor (simple)")
  {
    const char* sz = "abcdefghijklmnopqrstuvwxyz";

    ezStringView it(sz);

    EZ_TEST_BOOL(it.GetStartPosition() == sz);
    EZ_TEST_BOOL(it.GetData() == sz);
    EZ_TEST_BOOL(it.GetEndPosition() == sz + 26);
    EZ_TEST_INT(it.GetElementCount(), 26);

    ezStringView it2(sz + 15);

    EZ_TEST_BOOL(it2.GetStartPosition() == &sz[15]);
    EZ_TEST_BOOL(it2.GetData() == &sz[15]);
    EZ_TEST_BOOL(it2.GetEndPosition() == sz + 26);
    EZ_TEST_INT(it2.GetElementCount(), 11);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor (complex, YARLY!)")
  {
    const char* sz = "abcdefghijklmnopqrstuvwxyz";

    ezStringView it(sz + 3, sz + 17);
    it.SetStartPosition(sz + 5);

    EZ_TEST_BOOL(it.GetStartPosition() == sz + 5);
    EZ_TEST_BOOL(it.GetData() == sz + 5);
    EZ_TEST_BOOL(it.GetEndPosition() == sz + 17);
    EZ_TEST_INT(it.GetElementCount(), 12);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator++")
  {
    const char* sz = "abcdefghijklmnopqrstuvwxyz";
    ezStringView it(sz);

    for (ezInt32 i = 0; i < 26; ++i)
    {
      EZ_TEST_INT(it.GetCharacter(), sz[i]);
      EZ_TEST_BOOL(it.IsValid());
      ++it;
    }

    EZ_TEST_BOOL(!it.IsValid());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator== / operator!=")
  {
    ezString s1(L"abcdefghiäöüß€");
    ezString s2(L"ghiäöüß€abdef");

    ezStringView it1 = s1.GetSubString(8, 4);
    ezStringView it2 = s2.GetSubString(2, 4);
    ezStringView it3 = s2.GetSubString(2, 5);

    EZ_TEST_BOOL(it1 == it2);
    EZ_TEST_BOOL(it1 != it3);

    EZ_TEST_BOOL(it1 == ezString(L"iäöü").GetData());
    EZ_TEST_BOOL(it2 == ezString(L"iäöü").GetData());
    EZ_TEST_BOOL(it3 == ezString(L"iäöüß").GetData());

    s1 = "abcdefghijkl";
    s2 = "oghijklm";

    it1 = s1.GetSubString(6, 4);
    it2 = s2.GetSubString(1, 4);
    it3 = s2.GetSubString(1, 5);

    EZ_TEST_BOOL(it1 == it2);
    EZ_TEST_BOOL(it1 != it3);

    EZ_TEST_BOOL(it1 == "ghij");
    EZ_TEST_BOOL(it1 != "ghijk");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsEqual")
  {
    const char* sz = "abcdef";
    ezStringView it(sz);

    EZ_TEST_BOOL(it.IsEqual(ezStringView("abcdef")));
    EZ_TEST_BOOL(!it.IsEqual(ezStringView("abcde")));
    EZ_TEST_BOOL(!it.IsEqual(ezStringView("abcdefg")));

    ezStringView it2(sz + 2, sz + 5);

    const char* szRhs = "Abcdef";
    ezStringView it3(szRhs + 2, szRhs + 5);
    EZ_TEST_BOOL(it2.IsEqual(it3));
    it3 = ezStringView(szRhs + 1, szRhs + 5);
    EZ_TEST_BOOL(!it2.IsEqual(it3));
    it3 = ezStringView(szRhs + 2, szRhs + 6);
    EZ_TEST_BOOL(!it2.IsEqual(it3));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsEqual_NoCase")
  {
    const char* sz = "ABCDEF";
    ezStringView it(sz);

    EZ_TEST_BOOL(it.IsEqual_NoCase("abcdef"));
    EZ_TEST_BOOL(!it.IsEqual_NoCase("abcde"));
    EZ_TEST_BOOL(!it.IsEqual_NoCase("abcdefg"));

    ezStringView it2(sz + 1, sz + 5);
    it2.SetStartPosition(sz + 2);

    EZ_TEST_BOOL(it2.IsEqual_NoCase("cde"));
    EZ_TEST_BOOL(!it2.IsEqual_NoCase("bcde"));
    EZ_TEST_BOOL(!it2.IsEqual_NoCase("cdef"));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator+=")
  {
    const char* sz = "abcdefghijklmnopqrstuvwxyz";
    ezStringView it(sz);

    for (ezInt32 i = 0; i < 26; i += 2)
    {
      EZ_TEST_INT(it.GetCharacter(), sz[i]);
      EZ_TEST_BOOL(it.IsValid());
      it += 2;
    }

    EZ_TEST_BOOL(!it.IsValid());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetCharacter")
  {
    ezStringUtf8 s(L"abcäöü€");
    ezStringView it = ezStringView(s.GetData());

    EZ_TEST_INT(it.GetCharacter(), ezUnicodeUtils::ConvertUtf8ToUtf32(&s.GetData()[0])); ++it;
    EZ_TEST_INT(it.GetCharacter(), ezUnicodeUtils::ConvertUtf8ToUtf32(&s.GetData()[1])); ++it;
    EZ_TEST_INT(it.GetCharacter(), ezUnicodeUtils::ConvertUtf8ToUtf32(&s.GetData()[2])); ++it;
    EZ_TEST_INT(it.GetCharacter(), ezUnicodeUtils::ConvertUtf8ToUtf32(&s.GetData()[3])); ++it;
    EZ_TEST_INT(it.GetCharacter(), ezUnicodeUtils::ConvertUtf8ToUtf32(&s.GetData()[5])); ++it;
    EZ_TEST_INT(it.GetCharacter(), ezUnicodeUtils::ConvertUtf8ToUtf32(&s.GetData()[7])); ++it;
    EZ_TEST_INT(it.GetCharacter(), ezUnicodeUtils::ConvertUtf8ToUtf32(&s.GetData()[9])); ++it;
    EZ_TEST_BOOL(!it.IsValid());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetElementCount")
  {
    ezStringUtf8 s(L"abcäöü€");
    ezStringView it = ezStringView(s.GetData());

    EZ_TEST_INT(it.GetElementCount(), 12); ++it;    EZ_TEST_BOOL(it.IsValid());
    EZ_TEST_INT(it.GetElementCount(), 11); ++it;    EZ_TEST_BOOL(it.IsValid());
    EZ_TEST_INT(it.GetElementCount(), 10); ++it;    EZ_TEST_BOOL(it.IsValid());
    EZ_TEST_INT(it.GetElementCount(),  9); ++it;    EZ_TEST_BOOL(it.IsValid());
    EZ_TEST_INT(it.GetElementCount(),  7); ++it;    EZ_TEST_BOOL(it.IsValid());
    EZ_TEST_INT(it.GetElementCount(),  5); ++it;    EZ_TEST_BOOL(it.IsValid());
    EZ_TEST_INT(it.GetElementCount(),  3); ++it;    EZ_TEST_BOOL(!it.IsValid());
    EZ_TEST_INT(it.GetElementCount(),  0); ++it;    EZ_TEST_BOOL(!it.IsValid());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetStartPosition")
  {
    const char* sz = "abcdefghijklmnopqrstuvwxyz";
    ezStringView it(sz);

    for (ezInt32 i = 0; i < 26; ++i)
    {
      it.SetStartPosition(sz + i);
      EZ_TEST_BOOL(it.IsValid());
      EZ_TEST_BOOL(it.StartsWith(&sz[i]));
    }

    EZ_TEST_BOOL(it.IsValid());
    ++it;
    EZ_TEST_BOOL(!it.IsValid());

    it = ezStringView(sz);
    for (ezInt32 i = 0; i < 26; ++i)
    {
      it.SetStartPosition(sz + i);
      EZ_TEST_BOOL(it.IsValid());
      EZ_TEST_BOOL(it.StartsWith(&sz[i]));
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetStartPosition / GetEndPosition / GetData")
  {
    const char* sz = "abcdefghijklmnopqrstuvwxyz";
    ezStringView it(sz + 7, sz + 19);

    EZ_TEST_BOOL(it.GetStartPosition() == sz + 7);
    EZ_TEST_BOOL(it.GetEndPosition() == sz + 19);
    EZ_TEST_BOOL(it.GetData() == sz + 7);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Shrink")
  {
    ezStringUtf8 s(L"abcäöü€def");
    ezStringView it(s.GetData());

    EZ_TEST_BOOL(it.GetStartPosition() == &s.GetData()[0]);
    EZ_TEST_BOOL(it.GetEndPosition() == &s.GetData()[15]);
    EZ_TEST_BOOL(it.GetData() == &s.GetData()[0]);
    EZ_TEST_BOOL(it.IsValid());

    it.Shrink(1, 0);

    EZ_TEST_BOOL(it.GetStartPosition() == &s.GetData()[1]);
    EZ_TEST_BOOL(it.GetEndPosition() == &s.GetData()[15]);
    EZ_TEST_BOOL(it.GetData() == &s.GetData()[1]);
    EZ_TEST_BOOL(it.IsValid());

    it.Shrink(3, 0);

    EZ_TEST_BOOL(it.GetStartPosition() == &s.GetData()[5]);
    EZ_TEST_BOOL(it.GetEndPosition() == &s.GetData()[15]);
    EZ_TEST_BOOL(it.GetData() == &s.GetData()[5]);
    EZ_TEST_BOOL(it.IsValid());

    it.Shrink(0, 4);

    EZ_TEST_BOOL(it.GetStartPosition() == &s.GetData()[5]);
    EZ_TEST_BOOL(it.GetEndPosition() == &s.GetData()[9]);
    EZ_TEST_BOOL(it.GetData() == &s.GetData()[5]);
    EZ_TEST_BOOL(it.IsValid());

    it.Shrink(1, 1);

    EZ_TEST_BOOL(it.GetStartPosition() == &s.GetData()[7]);
    EZ_TEST_BOOL(it.GetEndPosition() == &s.GetData()[7]);
    EZ_TEST_BOOL(it.GetData() == &s.GetData()[7]);
    EZ_TEST_BOOL(!it.IsValid());

    it.Shrink(10, 10);

    EZ_TEST_BOOL(it.GetStartPosition() == &s.GetData()[7]);
    EZ_TEST_BOOL(it.GetEndPosition() == &s.GetData()[7]);
    EZ_TEST_BOOL(it.GetData() == &s.GetData()[7]);
    EZ_TEST_BOOL(!it.IsValid());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Trim")
  {
    // Empty input
    ezStringUtf8 utf8(L"");
    ezStringView view(utf8.GetData());
    view.Trim(" \t");
    EZ_TEST_BOOL(view.IsEqual(ezStringUtf8(L"").GetData()));
    view.Trim(nullptr, " \t");
    EZ_TEST_BOOL(view.IsEqual(ezStringUtf8(L"").GetData()));
    view.Trim(" \t", nullptr);
    EZ_TEST_BOOL(view.IsEqual(ezStringUtf8(L"").GetData()));

    // Clear all from one side
    ezStringUtf8 sUnicode(L"私はクリストハさんです"); 
    view = sUnicode.GetData();
    view.Trim(nullptr, sUnicode.GetData());
    EZ_TEST_BOOL(view.IsEqual(""));
    view = sUnicode.GetData();
    view.Trim(sUnicode.GetData(), nullptr);
    EZ_TEST_BOOL(view.IsEqual(""));

    // Clear partial side
    sUnicode = L"ですですですAにぱにぱにぱ";
    view = sUnicode.GetData();
    view.Trim(nullptr, ezStringUtf8(L"にぱ").GetData());
    sUnicode = L"ですですですA";
    EZ_TEST_BOOL(view.IsEqual(sUnicode.GetData()));
    view.Trim(ezStringUtf8(L"です").GetData(), nullptr);
    EZ_TEST_BOOL(view.IsEqual(ezStringUtf8(L"A").GetData()));

    sUnicode = L"ですですですAにぱにぱにぱ";
    view = sUnicode.GetData();
    view.Trim(ezStringUtf8(L"ですにぱ").GetData());
    EZ_TEST_BOOL(view.IsEqual(ezStringUtf8(L"A").GetData()));
  }
}

