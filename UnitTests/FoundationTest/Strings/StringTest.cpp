#include <PCH.h>
#include <Foundation/Strings/String.h>
#include <Foundation/IO/MemoryStream.h>

static ezString GetString(const char* sz)
{
  ezString s;
  s = sz;
  return s;
}

static ezStringBuilder GetStringBuilder(const char* sz)
{
  ezStringBuilder s;

  for (ezUInt32 i = 0; i < 10; ++i)
    s.Append(sz);

  return s;
}

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

    ezStringView it = s4.GetFirst(2);
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
    ezStringView it (s5.GetData() + 2, s5.GetData() + 10);
    s5 = it;
    EZ_TEST_BOOL(s5 == "cdefghij");

    ezString s6(L"aölsdföasld");
    ezStringBuilder strB("wobwob");
    s6 = strB;
    EZ_TEST_BOOL(s6 == "wobwob");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "convert to ezStringView")
  {
    ezString s(L"aölsdföasld");

    ezStringView sv = s;

    EZ_TEST_STRING(sv.GetData(), ezStringUtf8(L"aölsdföasld").GetData());
    EZ_TEST_BOOL(sv == ezStringUtf8(L"aölsdföasld").GetData());

    s = "abcdef";

    EZ_TEST_STRING(sv.GetData(), "abcdef");
    EZ_TEST_BOOL(sv == "abcdef");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Move constructor / operator")
  {
    ezString s1 (GetString("move me"));
    EZ_TEST_STRING(s1.GetData(), "move me");

    s1 = GetString("move move move move move move move move ");
    EZ_TEST_STRING(s1.GetData(), "move move move move move move move move ");

    ezString s2 (GetString("move move move move move move move move "));
    EZ_TEST_STRING(s2.GetData(), "move move move move move move move move ");

    s2 = GetString("move me");
    EZ_TEST_STRING(s2.GetData(), "move me");

    s1 = s2;
    EZ_TEST_STRING(s1.GetData(), "move me");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Move constructor / operator (StringBuilder)")
  {
    const ezString s1 (GetStringBuilder("move me"));
    const ezString s2 (GetStringBuilder("move move move move move move move move "));

    ezString s3 (GetStringBuilder("move me"));
    EZ_TEST_BOOL(s3 == s1);

    s3 = GetStringBuilder("move move move move move move move move ");
    EZ_TEST_BOOL(s3 == s2);

    ezString s4 (GetStringBuilder("move move move move move move move move "));
    EZ_TEST_BOOL(s4 == s2);

    s4 = GetStringBuilder("move me");
    EZ_TEST_BOOL(s4 == s1);

    s3 = s4;
    EZ_TEST_BOOL(s3 == s1);
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

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Convert to ezStringView")
  {
    ezString s(L"abcäöü€def");

    ezStringView view = s;
    EZ_TEST_BOOL(view.StartsWith("abc"));
    EZ_TEST_BOOL(view.EndsWith("def"));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetSubString")
  {
    ezString s(L"abcäöü€def");
    ezStringUtf8 s8(L"äöü€");

    ezStringView it = s.GetSubString(3, 4);
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

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ReadAll")
  {
    ezMemoryStreamStorage StreamStorage;
    
    ezMemoryStreamWriter MemoryWriter(&StreamStorage);
    ezMemoryStreamReader MemoryReader(&StreamStorage);

    const char* szText = "l;kjasdflkjdfasjlk asflkj asfljwe oiweq2390432 4 @#$ otrjk3l;2rlkhitoqhrn324:R l324h32kjr hnasfhsakfh234fas1440687873242321245";

    MemoryWriter.WriteBytes(szText, ezStringUtils::GetStringElementCount(szText));

    ezString s;
    s.ReadAll(MemoryReader);

    EZ_TEST_BOOL(s == szText);
  }
}

