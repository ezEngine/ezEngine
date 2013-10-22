#include <PCH.h>

EZ_CREATE_SIMPLE_TEST(Strings, String)
{
  EZ_TEST_BLOCK(true, "Constructor")
  {
    ezString s1;
    EZ_TEST(s1 == "");

    ezString s2("abc");
    EZ_TEST(s2 == "abc");
    
    ezString s3(s2);
    EZ_TEST(s2 == s3);
    EZ_TEST(s3 == "abc");

    ezString s4(L"abc");
    EZ_TEST(s4 == "abc");

    ezStringIterator it = s4.GetFirst(2);
    ezString s5(it);
    EZ_TEST(s5 == "ab");

    ezStringBuilder strB("wobwob");
    ezString s6(strB);
    EZ_TEST(s6 == "wobwob");
  }

  EZ_TEST_BLOCK(true, "operator=")
  {
    ezString s2; s2 = "abc";
    EZ_TEST(s2 == "abc");
    
    ezString s3; s3 = s2;
    EZ_TEST(s2 == s3);
    EZ_TEST(s3 == "abc");

    ezString s4; s4 = L"abc";
    EZ_TEST(s4 == "abc");

    ezString s5(L"abcdefghijklm");
    ezStringIterator it (s5.GetData() + 2, s5.GetData() + 10, s5.GetData() + 2);
    s5 = it;
    EZ_TEST(s5 == "cdefghij");

    ezString s6("aölsdföasld");
    ezStringBuilder strB("wobwob");
    s6 = strB;
    EZ_TEST(s6 == "wobwob");
  }

  EZ_TEST_BLOCK(true, "Clear")
  {
    ezString s("abcdef");
    EZ_TEST(s == "abcdef");

    s.Clear();
    EZ_TEST(s.IsEmpty());
    EZ_TEST(s == "");
    EZ_TEST(s == NULL);
    
  }

  EZ_TEST_BLOCK(true, "GetData")
  {
    const char* sz = "abcdef";

    ezString s(sz);
    EZ_TEST(s.GetData() != sz); // it should NOT be the exact same string
  }

  EZ_TEST_BLOCK(true, "GetElementCount / GetCharacterCount")
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

  EZ_TEST_BLOCK(true, "GetIteratorFront / GetIteratorBack")
  {
    ezString s(L"abcäöü€def");

    EZ_TEST(s.GetIteratorFront().StartsWith("abc"));
    EZ_TEST(s.GetIteratorFront().EndsWith("def"));

    EZ_TEST(s.GetIteratorBack().StartsWith("f"));
    EZ_TEST(s.GetIteratorBack().EndsWith("f"));
  }

  EZ_TEST_BLOCK(true, "GetSubString")
  {
    ezString s(L"abcäöü€def");
    ezStringUtf8 s8(L"äöü€");

    ezStringIterator it = s.GetSubString(3, 4);
    EZ_TEST(it == s8.GetData());
  }

  EZ_TEST_BLOCK(true, "GetFirst")
  {
    ezString s(L"abcäöü€def");

    EZ_TEST(s.GetFirst(3) == "abc");
  }

  EZ_TEST_BLOCK(true, "GetLast")
  {
    ezString s(L"abcäöü€def");

    EZ_TEST(s.GetLast(3) == "def");
  }
}

