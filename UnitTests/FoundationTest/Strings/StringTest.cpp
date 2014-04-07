#include <PCH.h>

EZ_CREATE_SIMPLE_TEST(Strings, String)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor")
  {
    ezString s1;
    EZ_TEST_BOOL(s1 == "");

    ezString s2("abc");
    EZ_TEST_BOOL(s2 == "abc");
    
    ezString s3(s2);
    EZ_TEST_BOOL(s2 == s3);
    EZ_TEST_BOOL(s3 == "abc");

    ezString s4(L"abc");
    EZ_TEST_BOOL(s4 == "abc");

    ezStringIterator it = s4.GetFirst(2);
    ezString s5(it);
    EZ_TEST_BOOL(s5 == "ab");

    ezStringBuilder strB("wobwob");
    ezString s6(strB);
    EZ_TEST_BOOL(s6 == "wobwob");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator=")
  {
    ezString s2; s2 = "abc";
    EZ_TEST_BOOL(s2 == "abc");
    
    ezString s3; s3 = s2;
    EZ_TEST_BOOL(s2 == s3);
    EZ_TEST_BOOL(s3 == "abc");

    ezString s4; s4 = L"abc";
    EZ_TEST_BOOL(s4 == "abc");

    ezString s5(L"abcdefghijklm");
    ezStringIterator it (s5.GetData() + 2, s5.GetData() + 10, s5.GetData() + 2);
    s5 = it;
    EZ_TEST_BOOL(s5 == "cdefghij");

    ezString s6("aölsdföasld");
    ezStringBuilder strB("wobwob");
    s6 = strB;
    EZ_TEST_BOOL(s6 == "wobwob");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Clear")
  {
    ezString s("abcdef");
    EZ_TEST_BOOL(s == "abcdef");

    s.Clear();
    EZ_TEST_BOOL(s.IsEmpty());
    EZ_TEST_BOOL(s == "");
    EZ_TEST_BOOL(s == nullptr);
    
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetData")
  {
    const char* sz = "abcdef";

    ezString s(sz);
    EZ_TEST_BOOL(s.GetData() != sz); // it should NOT be the exact same string
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetElementCount / GetCharacterCount")
  {
    ezString s(L"abcäöü€");

    EZ_TEST_INT(s.GetElementCount(), 12);
    EZ_TEST_INT(s.GetCharacterCount(), 7);

    s = "testtest";
    EZ_TEST_INT(s.GetElementCount(), 8);
    EZ_TEST_INT(s.GetCharacterCount(), 8);

    s.Clear();

    EZ_TEST_INT(s.GetElementCount(), 0);
    EZ_TEST_INT(s.GetCharacterCount(), 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetIteratorFront / GetIteratorBack")
  {
    ezString s(L"abcäöü€def");

    EZ_TEST_BOOL(s.GetIteratorFront().StartsWith("abc"));
    EZ_TEST_BOOL(s.GetIteratorFront().EndsWith("def"));

    EZ_TEST_BOOL(s.GetIteratorBack().StartsWith("f"));
    EZ_TEST_BOOL(s.GetIteratorBack().EndsWith("f"));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetSubString")
  {
    ezString s(L"abcäöü€def");
    ezStringUtf8 s8(L"äöü€");

    ezStringIterator it = s.GetSubString(3, 4);
    EZ_TEST_BOOL(it == s8.GetData());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetFirst")
  {
    ezString s(L"abcäöü€def");

    EZ_TEST_BOOL(s.GetFirst(3) == "abc");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetLast")
  {
    ezString s(L"abcäöü€def");

    EZ_TEST_BOOL(s.GetLast(3) == "def");
  }
}

