#include <TestFramework/Framework/TestFramework.h>
#include <Foundation/Strings/StringConversion.h>
#include <Foundation/Strings/StringIterator.h>

EZ_CREATE_SIMPLE_TEST(Strings, StringIterator)
{
  EZ_TEST_BLOCK(true, "Constructor (simple)")
  {
    const char* sz = "abcdefghijklmnopqrstuvwxyz";

    ezStringIterator it(sz);

    EZ_TEST(it.GetStart() == sz);
    EZ_TEST(it.GetData() == sz);
    EZ_TEST(it.GetEnd() == sz + 26);
    EZ_TEST_INT(it.GetElementCount(), 26);

    ezStringIterator it2(sz + 15);

    EZ_TEST(it2.GetStart() == &sz[15]);
    EZ_TEST(it2.GetData() == &sz[15]);
    EZ_TEST(it2.GetEnd() == sz + 26);
    EZ_TEST_INT(it2.GetElementCount(), 11);
  }

  EZ_TEST_BLOCK(true, "Constructor (complex, YARLY!)")
  {
    const char* sz = "abcdefghijklmnopqrstuvwxyz";

    ezStringIterator it(sz + 3, sz + 17, sz + 5);

    EZ_TEST(it.GetStart() == sz + 3);
    EZ_TEST(it.GetData() == sz + 5);
    EZ_TEST(it.GetEnd() == sz + 17);
    EZ_TEST_INT(it.GetElementCount(), 12);
  }

  EZ_TEST_BLOCK(true, "operator++")
  {
    const char* sz = "abcdefghijklmnopqrstuvwxyz";
    ezStringIterator it(sz);

    for (ezInt32 i = 0; i < 26; ++i)
    {
      EZ_TEST_INT(it.GetCharacter(), sz[i]);
      EZ_TEST(it.IsValid());
      ++it;
    }

    EZ_TEST(!it.IsValid());
    --it;
    EZ_TEST(!it.IsValid());
    ++it;
    EZ_TEST(!it.IsValid());
  }

  EZ_TEST_BLOCK(true, "operator--")
  {
    const char* sz = "abcdefghijklmnopqrstuvwxyz";
    ezStringIterator it(sz, sz + 26, sz + 25);

    for (ezInt32 i = 25; i >= 0; --i)
    {
      EZ_TEST_INT(it.GetCharacter(), sz[i]);
      EZ_TEST(it.IsValid());
      --it;
    }

    EZ_TEST(!it.IsValid());
    ++it;
    EZ_TEST(!it.IsValid());
    --it;
    EZ_TEST(!it.IsValid());
  }

  EZ_TEST_BLOCK(true, "operator+=")
  {
    const char* sz = "abcdefghijklmnopqrstuvwxyz";
    ezStringIterator it(sz);

    for (ezInt32 i = 0; i < 26; i += 2)
    {
      EZ_TEST_INT(it.GetCharacter(), sz[i]);
      EZ_TEST(it.IsValid());
      it += 2;
    }

    EZ_TEST(!it.IsValid());
  }

  EZ_TEST_BLOCK(true, "operator-=")
  {
    const char* sz = "abcdefghijklmnopqrstuvwxyz";
    ezStringIterator it(sz, sz + 26, sz + 25);

    for (ezInt32 i = 25; i >= 0; i -= 2)
    {
      EZ_TEST_INT(it.GetCharacter(), sz[i]);
      EZ_TEST(it.IsValid());
      it -= 2;
    }
  }

  EZ_TEST_BLOCK(true, "GetCharacter")
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
    EZ_TEST(!it.IsValid());
  }

  EZ_TEST_BLOCK(true, "GetElementCount")
  {
    ezStringUtf8 s(L"abcäöü€");
    ezStringIterator it = ezStringIterator(s.GetData());

    EZ_TEST_INT(it.GetElementCount(), 12); ++it;    EZ_TEST(it.IsValid());
    EZ_TEST_INT(it.GetElementCount(), 11); ++it;    EZ_TEST(it.IsValid());
    EZ_TEST_INT(it.GetElementCount(), 10); ++it;    EZ_TEST(it.IsValid());
    EZ_TEST_INT(it.GetElementCount(),  9); ++it;    EZ_TEST(it.IsValid());
    EZ_TEST_INT(it.GetElementCount(),  7); ++it;    EZ_TEST(it.IsValid());
    EZ_TEST_INT(it.GetElementCount(),  5); ++it;    EZ_TEST(it.IsValid());
    EZ_TEST_INT(it.GetElementCount(),  3); ++it;    EZ_TEST(!it.IsValid());
    EZ_TEST_INT(it.GetElementCount(),  0); ++it;    EZ_TEST(!it.IsValid());
  }

  EZ_TEST_BLOCK(true, "SetCurrentPosition")
  {
    const char* sz = "abcdefghijklmnopqrstuvwxyz";
    ezStringIterator it(sz);

    for (ezInt32 i = 0; i < 26; ++i)
    {
      it.SetCurrentPosition(sz + i);
      EZ_TEST(it.IsValid());
      EZ_TEST(it.StartsWith(&sz[i]));
    }

    EZ_TEST(it.IsValid());
    ++it;
    EZ_TEST(!it.IsValid());

    for (ezInt32 i = 0; i < 26; ++i)
    {
      it.SetCurrentPosition(sz + i);
      EZ_TEST(it.IsValid());
      EZ_TEST(it.StartsWith(&sz[i]));
    }
  }

  EZ_TEST_BLOCK(true, "IsPureASCII")
  {
    const char* sz = "abcdefghijklmnopqrstuvwxyz";
    ezStringIterator it(sz, true);
    ezStringIterator it2(sz, false);

    // there is no automatic detection for this in ezStringIterator (in ezString yes)
    EZ_TEST(it.IsPureASCII());
    EZ_TEST(!it2.IsPureASCII());
  }

  EZ_TEST_BLOCK(true, "GetStart / GetEnd / GetData")
  {
    const char* sz = "abcdefghijklmnopqrstuvwxyz";
    ezStringIterator it(sz + 7, sz + 19, sz + 13);

    EZ_TEST(it.GetStart() == sz + 7);
    EZ_TEST(it.GetEnd() == sz + 19);
    EZ_TEST(it.GetData() == sz + 13);
  }

  EZ_TEST_BLOCK(true, "Shrink")
  {
    ezStringUtf8 s(L"abcäöü€def");
    ezStringIterator it(s.GetData());
    it += 2;

    EZ_TEST(it.GetStart() == &s.GetData()[0]);
    EZ_TEST(it.GetEnd() == &s.GetData()[15]);
    EZ_TEST(it.GetData() == &s.GetData()[2]);
    EZ_TEST(it.IsValid());

    it.Shrink(1, 0);

    EZ_TEST(it.GetStart() == &s.GetData()[1]);
    EZ_TEST(it.GetEnd() == &s.GetData()[15]);
    EZ_TEST(it.GetData() == &s.GetData()[2]);
    EZ_TEST(it.IsValid());

    it.Shrink(3, 0);

    EZ_TEST(it.GetStart() == &s.GetData()[5]);
    EZ_TEST(it.GetEnd() == &s.GetData()[15]);
    EZ_TEST(it.GetData() == &s.GetData()[5]);
    EZ_TEST(it.IsValid());

    it.Shrink(0, 4);

    EZ_TEST(it.GetStart() == &s.GetData()[5]);
    EZ_TEST(it.GetEnd() == &s.GetData()[9]);
    EZ_TEST(it.GetData() == &s.GetData()[5]);
    EZ_TEST(it.IsValid());

    it.Shrink(1, 1);

    EZ_TEST(it.GetStart() == &s.GetData()[7]);
    EZ_TEST(it.GetEnd() == &s.GetData()[7]);
    EZ_TEST(it.GetData() == &s.GetData()[7]);
    EZ_TEST(!it.IsValid());

    it.Shrink(10, 10);

    EZ_TEST(it.GetStart() == &s.GetData()[7]);
    EZ_TEST(it.GetEnd() == &s.GetData()[7]);
    EZ_TEST(it.GetData() == &s.GetData()[7]);
    EZ_TEST(!it.IsValid());
  }

  EZ_TEST_BLOCK(true, "ResetToFront")
  {
    const char* sz = "abcdefghijklmnopqrstuvwxyz";
    ezStringIterator it(sz + 7, sz + 19, sz + 13);

    EZ_TEST(it.GetStart() == sz + 7);
    EZ_TEST(it.GetEnd() == sz + 19);
    EZ_TEST(it.GetData() == sz + 13);

    it.ResetToFront();
    EZ_TEST(it.GetData() == sz + 7);
    EZ_TEST(it.IsValid());

    --it;
    EZ_TEST(!it.IsValid());

    it.ResetToFront();
    EZ_TEST(it.GetData() == sz + 7);
    EZ_TEST(it.IsValid());
  }

  EZ_TEST_BLOCK(true, "ResetToBack")
  {
    const char* sz = "abcdefghijklmnopqrstuvwxyz";
    ezStringIterator it(sz + 7, sz + 19, sz + 13);

    EZ_TEST(it.GetStart() == sz + 7);
    EZ_TEST(it.GetEnd() == sz + 19);
    EZ_TEST(it.GetData() == sz + 13);

    it.ResetToBack();
    EZ_TEST(it.GetData() == sz + 18);
    EZ_TEST(it.IsValid());

    ++it;
    EZ_TEST(!it.IsValid());

    it.ResetToBack();
    EZ_TEST(it.GetData() == sz + 18);
    EZ_TEST(it.IsValid());
  }
}

