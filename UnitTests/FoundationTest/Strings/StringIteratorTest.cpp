#include <PCH.h>
#include <Foundation/Strings/String.h>

EZ_CREATE_SIMPLE_TEST(Strings, StringIterator)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor (simple)")
  {
    const char* sz = "abcdefghijklmnopqrstuvwxyz";

    ezStringIterator it(sz);

    EZ_TEST_BOOL(it.GetStart() == sz);
    EZ_TEST_BOOL(it.GetData() == sz);
    EZ_TEST_BOOL(it.GetEnd() == sz + 26);
    EZ_TEST_INT(it.GetElementCount(), 26);

    ezStringIterator it2(sz + 15);

    EZ_TEST_BOOL(it2.GetStart() == &sz[15]);
    EZ_TEST_BOOL(it2.GetData() == &sz[15]);
    EZ_TEST_BOOL(it2.GetEnd() == sz + 26);
    EZ_TEST_INT(it2.GetElementCount(), 11);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor (complex, YARLY!)")
  {
    const char* sz = "abcdefghijklmnopqrstuvwxyz";

    ezStringIterator it(sz + 3, sz + 17, sz + 5);

    EZ_TEST_BOOL(it.GetStart() == sz + 3);
    EZ_TEST_BOOL(it.GetData() == sz + 5);
    EZ_TEST_BOOL(it.GetEnd() == sz + 17);
    EZ_TEST_INT(it.GetElementCount(), 12);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator++")
  {
    const char* sz = "abcdefghijklmnopqrstuvwxyz";
    ezStringIterator it(sz);

    for (ezInt32 i = 0; i < 26; ++i)
    {
      EZ_TEST_INT(it.GetCharacter(), sz[i]);
      EZ_TEST_BOOL(it.IsValid());
      ++it;
    }

    EZ_TEST_BOOL(!it.IsValid());
    --it;
    EZ_TEST_BOOL(!it.IsValid());
    ++it;
    EZ_TEST_BOOL(!it.IsValid());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator--")
  {
    const char* sz = "abcdefghijklmnopqrstuvwxyz";
    ezStringIterator it(sz, sz + 26, sz + 25);

    for (ezInt32 i = 25; i >= 0; --i)
    {
      EZ_TEST_INT(it.GetCharacter(), sz[i]);
      EZ_TEST_BOOL(it.IsValid());
      --it;
    }

    EZ_TEST_BOOL(!it.IsValid());
    ++it;
    EZ_TEST_BOOL(!it.IsValid());
    --it;
    EZ_TEST_BOOL(!it.IsValid());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator==")
  {
    ezString s1(L"abcdefghiäöüß€");
    ezString s2(L"ghiäöüß€abdef");

    ezStringIterator it1 = s1.GetSubString(8, 4);
    ezStringIterator it2 = s2.GetSubString(2, 4);
    ezStringIterator it3 = s2.GetSubString(2, 5);

    EZ_TEST_BOOL(it1 == it2);
    EZ_TEST_BOOL(it1 != it3);

    EZ_TEST_BOOL(it1 == ezString(L"iäöü").GetData());
    EZ_TEST_BOOL(it2 == ezString(L"iäöü").GetData());
    EZ_TEST_BOOL(it3 == ezString(L"iäöüß").GetData());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator+=")
  {
    const char* sz = "abcdefghijklmnopqrstuvwxyz";
    ezStringIterator it(sz);

    for (ezInt32 i = 0; i < 26; i += 2)
    {
      EZ_TEST_INT(it.GetCharacter(), sz[i]);
      EZ_TEST_BOOL(it.IsValid());
      it += 2;
    }

    EZ_TEST_BOOL(!it.IsValid());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator-=")
  {
    const char* sz = "abcdefghijklmnopqrstuvwxyz";
    ezStringIterator it(sz, sz + 26, sz + 25);

    for (ezInt32 i = 25; i >= 0; i -= 2)
    {
      EZ_TEST_INT(it.GetCharacter(), sz[i]);
      EZ_TEST_BOOL(it.IsValid());
      it -= 2;
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetCharacter")
  {
    ezStringUtf8 s(L"abcäöü€");
    ezStringIterator it = ezStringIterator(s.GetData());

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
    ezStringIterator it = ezStringIterator(s.GetData());

    EZ_TEST_INT(it.GetElementCount(), 12); ++it;    EZ_TEST_BOOL(it.IsValid());
    EZ_TEST_INT(it.GetElementCount(), 11); ++it;    EZ_TEST_BOOL(it.IsValid());
    EZ_TEST_INT(it.GetElementCount(), 10); ++it;    EZ_TEST_BOOL(it.IsValid());
    EZ_TEST_INT(it.GetElementCount(),  9); ++it;    EZ_TEST_BOOL(it.IsValid());
    EZ_TEST_INT(it.GetElementCount(),  7); ++it;    EZ_TEST_BOOL(it.IsValid());
    EZ_TEST_INT(it.GetElementCount(),  5); ++it;    EZ_TEST_BOOL(it.IsValid());
    EZ_TEST_INT(it.GetElementCount(),  3); ++it;    EZ_TEST_BOOL(!it.IsValid());
    EZ_TEST_INT(it.GetElementCount(),  0); ++it;    EZ_TEST_BOOL(!it.IsValid());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetCurrentPosition")
  {
    const char* sz = "abcdefghijklmnopqrstuvwxyz";
    ezStringIterator it(sz);

    for (ezInt32 i = 0; i < 26; ++i)
    {
      it.SetCurrentPosition(sz + i);
      EZ_TEST_BOOL(it.IsValid());
      EZ_TEST_BOOL(it.StartsWith(&sz[i]));
    }

    EZ_TEST_BOOL(it.IsValid());
    ++it;
    EZ_TEST_BOOL(!it.IsValid());

    for (ezInt32 i = 0; i < 26; ++i)
    {
      it.SetCurrentPosition(sz + i);
      EZ_TEST_BOOL(it.IsValid());
      EZ_TEST_BOOL(it.StartsWith(&sz[i]));
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsPureASCII")
  {
    const char* sz = "abcdefghijklmnopqrstuvwxyz";
    ezStringIterator it(sz, true);
    ezStringIterator it2(sz, false);

    // there is no automatic detection for this in ezStringIterator (in ezString yes)
    EZ_TEST_BOOL(it.IsPureASCII());
    EZ_TEST_BOOL(!it2.IsPureASCII());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetStart / GetEnd / GetData")
  {
    const char* sz = "abcdefghijklmnopqrstuvwxyz";
    ezStringIterator it(sz + 7, sz + 19, sz + 13);

    EZ_TEST_BOOL(it.GetStart() == sz + 7);
    EZ_TEST_BOOL(it.GetEnd() == sz + 19);
    EZ_TEST_BOOL(it.GetData() == sz + 13);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Shrink")
  {
    ezStringUtf8 s(L"abcäöü€def");
    ezStringIterator it(s.GetData());
    it += 2;

    EZ_TEST_BOOL(it.GetStart() == &s.GetData()[0]);
    EZ_TEST_BOOL(it.GetEnd() == &s.GetData()[15]);
    EZ_TEST_BOOL(it.GetData() == &s.GetData()[2]);
    EZ_TEST_BOOL(it.IsValid());

    it.Shrink(1, 0);

    EZ_TEST_BOOL(it.GetStart() == &s.GetData()[1]);
    EZ_TEST_BOOL(it.GetEnd() == &s.GetData()[15]);
    EZ_TEST_BOOL(it.GetData() == &s.GetData()[2]);
    EZ_TEST_BOOL(it.IsValid());

    it.Shrink(3, 0);

    EZ_TEST_BOOL(it.GetStart() == &s.GetData()[5]);
    EZ_TEST_BOOL(it.GetEnd() == &s.GetData()[15]);
    EZ_TEST_BOOL(it.GetData() == &s.GetData()[5]);
    EZ_TEST_BOOL(it.IsValid());

    it.Shrink(0, 4);

    EZ_TEST_BOOL(it.GetStart() == &s.GetData()[5]);
    EZ_TEST_BOOL(it.GetEnd() == &s.GetData()[9]);
    EZ_TEST_BOOL(it.GetData() == &s.GetData()[5]);
    EZ_TEST_BOOL(it.IsValid());

    it.Shrink(1, 1);

    EZ_TEST_BOOL(it.GetStart() == &s.GetData()[7]);
    EZ_TEST_BOOL(it.GetEnd() == &s.GetData()[7]);
    EZ_TEST_BOOL(it.GetData() == &s.GetData()[7]);
    EZ_TEST_BOOL(!it.IsValid());

    it.Shrink(10, 10);

    EZ_TEST_BOOL(it.GetStart() == &s.GetData()[7]);
    EZ_TEST_BOOL(it.GetEnd() == &s.GetData()[7]);
    EZ_TEST_BOOL(it.GetData() == &s.GetData()[7]);
    EZ_TEST_BOOL(!it.IsValid());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ResetToFront")
  {
    const char* sz = "abcdefghijklmnopqrstuvwxyz";
    ezStringIterator it(sz + 7, sz + 19, sz + 13);

    EZ_TEST_BOOL(it.GetStart() == sz + 7);
    EZ_TEST_BOOL(it.GetEnd() == sz + 19);
    EZ_TEST_BOOL(it.GetData() == sz + 13);

    it.ResetToFront();
    EZ_TEST_BOOL(it.GetData() == sz + 7);
    EZ_TEST_BOOL(it.IsValid());

    --it;
    EZ_TEST_BOOL(!it.IsValid());

    it.ResetToFront();
    EZ_TEST_BOOL(it.GetData() == sz + 7);
    EZ_TEST_BOOL(it.IsValid());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ResetToBack")
  {
    const char* sz = "abcdefghijklmnopqrstuvwxyz";
    ezStringIterator it(sz + 7, sz + 19, sz + 13);

    EZ_TEST_BOOL(it.GetStart() == sz + 7);
    EZ_TEST_BOOL(it.GetEnd() == sz + 19);
    EZ_TEST_BOOL(it.GetData() == sz + 13);

    it.ResetToBack();
    EZ_TEST_BOOL(it.GetData() == sz + 18);
    EZ_TEST_BOOL(it.IsValid());

    ++it;
    EZ_TEST_BOOL(!it.IsValid());

    it.ResetToBack();
    EZ_TEST_BOOL(it.GetData() == sz + 18);
    EZ_TEST_BOOL(it.IsValid());
  }
}

