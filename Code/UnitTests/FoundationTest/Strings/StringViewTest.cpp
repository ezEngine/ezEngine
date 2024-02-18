#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/Deque.h>
#include <Foundation/Strings/String.h>

#include <string_view>

using namespace std;

const ezStringView gConstant1 = "gConstant1"_ezsv;
const ezStringView gConstant2("gConstant2");
const std::string_view gConstant3 = "gConstant3"sv;

EZ_CREATE_SIMPLE_TEST(Strings, StringView)
{
  ezStringBuilder tmp;

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor (simple)")
  {
    const char* sz = "abcdefghijklmnopqrstuvwxyz";

    ezStringView it(sz);

    EZ_TEST_BOOL(it.GetStartPointer() == sz);
    EZ_TEST_STRING(it.GetData(tmp), sz);
    EZ_TEST_BOOL(it.GetEndPointer() == sz + 26);
    EZ_TEST_INT(it.GetElementCount(), 26);

    ezStringView it2(sz + 15);

    EZ_TEST_BOOL(it2.GetStartPointer() == &sz[15]);
    EZ_TEST_STRING(it2.GetData(tmp), &sz[15]);
    EZ_TEST_BOOL(it2.GetEndPointer() == sz + 26);
    EZ_TEST_INT(it2.GetElementCount(), 11);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor (complex, YARLY!)")
  {
    const char* sz = "abcdefghijklmnopqrstuvwxyz";

    ezStringView it(sz + 3, sz + 17);
    it.SetStartPosition(sz + 5);

    EZ_TEST_BOOL(it.GetStartPointer() == sz + 5);
    EZ_TEST_STRING(it.GetData(tmp), "fghijklmnopq");
    EZ_TEST_BOOL(it.GetEndPointer() == sz + 17);
    EZ_TEST_INT(it.GetElementCount(), 12);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor constexpr")
  {
    constexpr ezStringView b = ezStringView("Hello World", 10);
    EZ_TEST_INT(b.GetElementCount(), 10);
    EZ_TEST_STRING(b.GetData(tmp), "Hello Worl");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "String literal")
  {
    constexpr ezStringView a = "Hello World"_ezsv;
    EZ_TEST_INT(a.GetElementCount(), 11);
    EZ_TEST_STRING(a.GetData(tmp), "Hello World");

    ezStringView b = "Hello Worl"_ezsv;
    EZ_TEST_INT(b.GetElementCount(), 10);
    EZ_TEST_STRING(b.GetData(tmp), "Hello Worl");

    // tests a special case in which the MSVC compiler would run into trouble
    EZ_TEST_INT(gConstant1.GetElementCount(), 10);
    EZ_TEST_STRING(gConstant1.GetData(tmp), "gConstant1");

    EZ_TEST_INT(gConstant2.GetElementCount(), 10);
    EZ_TEST_STRING(gConstant2.GetData(tmp), "gConstant2");

    EZ_TEST_INT(gConstant3.size(), 10);
    EZ_TEST_BOOL(gConstant3 == "gConstant3");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator++")
  {
    const char* sz = "abcdefghijklmnopqrstuvwxyz";
    ezStringView it(sz);

    for (ezInt32 i = 0; i < 26; ++i)
    {
      EZ_TEST_INT(it.GetCharacter(), sz[i]);
      EZ_TEST_BOOL(it.IsValid());
      it.Shrink(1, 0);
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
      it.Shrink(2, 0);
    }

    EZ_TEST_BOOL(!it.IsValid());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetCharacter")
  {
    ezStringUtf8 s(L"abcäöü€");
    ezStringView it = ezStringView(s.GetData());

    EZ_TEST_INT(it.GetCharacter(), ezUnicodeUtils::ConvertUtf8ToUtf32(&s.GetData()[0]));
    it.Shrink(1, 0);
    EZ_TEST_INT(it.GetCharacter(), ezUnicodeUtils::ConvertUtf8ToUtf32(&s.GetData()[1]));
    it.Shrink(1, 0);
    EZ_TEST_INT(it.GetCharacter(), ezUnicodeUtils::ConvertUtf8ToUtf32(&s.GetData()[2]));
    it.Shrink(1, 0);
    EZ_TEST_INT(it.GetCharacter(), ezUnicodeUtils::ConvertUtf8ToUtf32(&s.GetData()[3]));
    it.Shrink(1, 0);
    EZ_TEST_INT(it.GetCharacter(), ezUnicodeUtils::ConvertUtf8ToUtf32(&s.GetData()[5]));
    it.Shrink(1, 0);
    EZ_TEST_INT(it.GetCharacter(), ezUnicodeUtils::ConvertUtf8ToUtf32(&s.GetData()[7]));
    it.Shrink(1, 0);
    EZ_TEST_INT(it.GetCharacter(), ezUnicodeUtils::ConvertUtf8ToUtf32(&s.GetData()[9]));
    it.Shrink(1, 0);
    EZ_TEST_BOOL(!it.IsValid());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetElementCount")
  {
    ezStringUtf8 s(L"abcäöü€");
    ezStringView it = ezStringView(s.GetData());

    EZ_TEST_INT(it.GetElementCount(), 12);
    it.Shrink(1, 0);
    EZ_TEST_BOOL(it.IsValid());
    EZ_TEST_INT(it.GetElementCount(), 11);
    it.Shrink(1, 0);
    EZ_TEST_BOOL(it.IsValid());
    EZ_TEST_INT(it.GetElementCount(), 10);
    it.Shrink(1, 0);
    EZ_TEST_BOOL(it.IsValid());
    EZ_TEST_INT(it.GetElementCount(), 9);
    it.Shrink(1, 0);
    EZ_TEST_BOOL(it.IsValid());
    EZ_TEST_INT(it.GetElementCount(), 7);
    it.Shrink(1, 0);
    EZ_TEST_BOOL(it.IsValid());
    EZ_TEST_INT(it.GetElementCount(), 5);
    it.Shrink(1, 0);
    EZ_TEST_BOOL(it.IsValid());
    EZ_TEST_INT(it.GetElementCount(), 3);
    it.Shrink(1, 0);
    EZ_TEST_BOOL(!it.IsValid());
    EZ_TEST_INT(it.GetElementCount(), 0);
    it.Shrink(1, 0);
    EZ_TEST_BOOL(!it.IsValid());
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
    it.Shrink(1, 0);
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

    EZ_TEST_BOOL(it.GetStartPointer() == sz + 7);
    EZ_TEST_BOOL(it.GetEndPointer() == sz + 19);
    EZ_TEST_STRING(it.GetData(tmp), "hijklmnopqrs");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Shrink")
  {
    ezStringUtf8 s(L"abcäöü€def");
    ezStringView it(s.GetData());

    EZ_TEST_BOOL(it.GetStartPointer() == &s.GetData()[0]);
    EZ_TEST_BOOL(it.GetEndPointer() == &s.GetData()[15]);
    EZ_TEST_STRING(it.GetData(tmp), &s.GetData()[0]);
    EZ_TEST_BOOL(it.IsValid());

    it.Shrink(1, 0);

    EZ_TEST_BOOL(it.GetStartPointer() == &s.GetData()[1]);
    EZ_TEST_BOOL(it.GetEndPointer() == &s.GetData()[15]);
    EZ_TEST_STRING(it.GetData(tmp), &s.GetData()[1]);
    EZ_TEST_BOOL(it.IsValid());

    it.Shrink(3, 0);

    EZ_TEST_BOOL(it.GetStartPointer() == &s.GetData()[5]);
    EZ_TEST_BOOL(it.GetEndPointer() == &s.GetData()[15]);
    EZ_TEST_STRING(it.GetData(tmp), &s.GetData()[5]);
    EZ_TEST_BOOL(it.IsValid());

    it.Shrink(0, 4);

    EZ_TEST_BOOL(it.GetStartPointer() == &s.GetData()[5]);
    EZ_TEST_BOOL(it.GetEndPointer() == &s.GetData()[9]);
    EZ_TEST_STRING(it.GetData(tmp), (const char*)u8"öü");
    EZ_TEST_BOOL(it.IsValid());

    it.Shrink(1, 1);

    EZ_TEST_BOOL(it.GetStartPointer() == &s.GetData()[7]);
    EZ_TEST_BOOL(it.GetEndPointer() == &s.GetData()[7]);
    EZ_TEST_STRING(it.GetData(tmp), "");
    EZ_TEST_BOOL(!it.IsValid());

    it.Shrink(10, 10);

    EZ_TEST_BOOL(it.GetStartPointer() == &s.GetData()[7]);
    EZ_TEST_BOOL(it.GetEndPointer() == &s.GetData()[7]);
    EZ_TEST_STRING(it.GetData(tmp), "");
    EZ_TEST_BOOL(!it.IsValid());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ChopAwayFirstCharacterUtf8")
  {
    ezStringUtf8 utf8(L"О, Господи!");
    ezStringView s(utf8.GetData());

    const char* szOrgStart = s.GetStartPointer();
    const char* szOrgEnd = s.GetEndPointer();

    while (!s.IsEmpty())
    {
      const ezUInt32 uiNumCharsBefore = ezStringUtils::GetCharacterCount(s.GetStartPointer(), s.GetEndPointer());
      s.ChopAwayFirstCharacterUtf8();
      const ezUInt32 uiNumCharsAfter = ezStringUtils::GetCharacterCount(s.GetStartPointer(), s.GetEndPointer());

      EZ_TEST_INT(uiNumCharsBefore, uiNumCharsAfter + 1);
    }

    // this needs to be true, some code relies on the fact that the start pointer always moves forwards
    EZ_TEST_BOOL(s.GetStartPointer() == szOrgEnd);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ChopAwayFirstCharacterAscii")
  {
    ezStringUtf8 utf8(L"Wosn Schmarrn");
    ezStringView s("");

    const char* szOrgStart = s.GetStartPointer();
    const char* szOrgEnd = s.GetEndPointer();

    while (!s.IsEmpty())
    {
      const ezUInt32 uiNumCharsBefore = s.GetElementCount();
      s.ChopAwayFirstCharacterAscii();
      const ezUInt32 uiNumCharsAfter = s.GetElementCount();

      EZ_TEST_INT(uiNumCharsBefore, uiNumCharsAfter + 1);
    }

    // this needs to be true, some code relies on the fact that the start pointer always moves forwards
    EZ_TEST_BOOL(s.GetStartPointer() == szOrgEnd);
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

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "TrimWordStart")
  {
    ezStringView sb;

    {
      sb = "<test>abc<test>";
      EZ_TEST_BOOL(sb.TrimWordStart("<test>"));
      EZ_TEST_STRING(sb, "abc<test>");
      EZ_TEST_BOOL(sb.TrimWordStart("<test>") == false);
      EZ_TEST_STRING(sb, "abc<test>");
    }

    {
      sb = "<test><tut><test><test><tut>abc<tut><test>";
      EZ_TEST_BOOL(!sb.TrimWordStart("<tut>"));
      EZ_TEST_BOOL(sb.TrimWordStart("<test>"));
      EZ_TEST_BOOL(sb.TrimWordStart("<tut>"));
      EZ_TEST_BOOL(sb.TrimWordStart("<test>"));
      EZ_TEST_BOOL(sb.TrimWordStart("<test>"));
      EZ_TEST_BOOL(sb.TrimWordStart("<tut>"));
      EZ_TEST_STRING(sb, "abc<tut><test>");
      EZ_TEST_BOOL(sb.TrimWordStart("<tut>") == false);
      EZ_TEST_BOOL(sb.TrimWordStart("<test>") == false);
      EZ_TEST_STRING(sb, "abc<tut><test>");
    }

    {
      sb = "<a><b><c><d><e><a><b><c><d><e>abc";

      while (sb.TrimWordStart("<a>") ||
             sb.TrimWordStart("<b>") ||
             sb.TrimWordStart("<c>") ||
             sb.TrimWordStart("<d>") ||
             sb.TrimWordStart("<e>"))
      {
      }

      EZ_TEST_STRING(sb, "abc");
    }

    {
      sb = "<a><b><c><d><e><a><b><c><d><e>";

      while (sb.TrimWordStart("<a>") ||
             sb.TrimWordStart("<b>") ||
             sb.TrimWordStart("<c>") ||
             sb.TrimWordStart("<d>") ||
             sb.TrimWordStart("<e>"))
      {
      }

      EZ_TEST_STRING(sb, "");
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "TrimWordEnd")
  {
    ezStringView sb;

    {
      sb = "<test>abc<test>";
      EZ_TEST_BOOL(sb.TrimWordEnd("<test>"));
      EZ_TEST_STRING(sb, "<test>abc");
      EZ_TEST_BOOL(sb.TrimWordEnd("<test>") == false);
      EZ_TEST_STRING(sb, "<test>abc");
    }

    {
      sb = "<tut><test>abc<test><tut><test><test><tut>";
      EZ_TEST_BOOL(sb.TrimWordEnd("<tut>"));
      EZ_TEST_BOOL(sb.TrimWordEnd("<test>"));
      EZ_TEST_BOOL(sb.TrimWordEnd("<test>"));
      EZ_TEST_BOOL(sb.TrimWordEnd("<tut>"));
      EZ_TEST_BOOL(sb.TrimWordEnd("<test>"));
      EZ_TEST_STRING(sb, "<tut><test>abc");
      EZ_TEST_BOOL(sb.TrimWordEnd("<tut>") == false);
      EZ_TEST_BOOL(sb.TrimWordEnd("<test>") == false);
      EZ_TEST_STRING(sb, "<tut><test>abc");
    }

    {
      sb = "abc<a><b><c><d><e><a><b><c><d><e>";

      while (sb.TrimWordEnd("<a>") ||
             sb.TrimWordEnd("<b>") ||
             sb.TrimWordEnd("<c>") ||
             sb.TrimWordEnd("<d>") ||
             sb.TrimWordEnd("<e>"))
      {
      }

      EZ_TEST_STRING(sb, "abc");
    }

    {
      sb = "<a><b><c><d><e><a><b><c><d><e>";

      while (sb.TrimWordEnd("<a>") ||
             sb.TrimWordEnd("<b>") ||
             sb.TrimWordEnd("<c>") ||
             sb.TrimWordEnd("<d>") ||
             sb.TrimWordEnd("<e>"))
      {
      }

      EZ_TEST_STRING(sb, "");
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Split")
  {
    ezStringView s = "|abc,def<>ghi|,<>jkl|mno,pqr|stu";

    ezDeque<ezStringView> SubStrings;

    s.Split(false, SubStrings, ",", "|", "<>");

    EZ_TEST_INT(SubStrings.GetCount(), 7);
    EZ_TEST_BOOL(SubStrings[0] == "abc");
    EZ_TEST_BOOL(SubStrings[1] == "def");
    EZ_TEST_BOOL(SubStrings[2] == "ghi");
    EZ_TEST_BOOL(SubStrings[3] == "jkl");
    EZ_TEST_BOOL(SubStrings[4] == "mno");
    EZ_TEST_BOOL(SubStrings[5] == "pqr");
    EZ_TEST_BOOL(SubStrings[6] == "stu");

    s.Split(true, SubStrings, ",", "|", "<>");

    EZ_TEST_INT(SubStrings.GetCount(), 10);
    EZ_TEST_BOOL(SubStrings[0] == "");
    EZ_TEST_BOOL(SubStrings[1] == "abc");
    EZ_TEST_BOOL(SubStrings[2] == "def");
    EZ_TEST_BOOL(SubStrings[3] == "ghi");
    EZ_TEST_BOOL(SubStrings[4] == "");
    EZ_TEST_BOOL(SubStrings[5] == "");
    EZ_TEST_BOOL(SubStrings[6] == "jkl");
    EZ_TEST_BOOL(SubStrings[7] == "mno");
    EZ_TEST_BOOL(SubStrings[8] == "pqr");
    EZ_TEST_BOOL(SubStrings[9] == "stu");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "HasAnyExtension")
  {
    ezStringView p = "This/Is\\My//Path.dot\\file.extension";
    EZ_TEST_BOOL(p.HasAnyExtension());

    p = "This/Is\\My//Path.dot\\file_no_extension";
    EZ_TEST_BOOL(!p.HasAnyExtension());
    EZ_TEST_BOOL(!p.HasAnyExtension());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "HasExtension")
  {
    ezStringView p;

    p = "This/Is\\My//Path.dot\\file.extension";
    EZ_TEST_BOOL(p.HasExtension(".Extension"));

    p = "This/Is\\My//Path.dot\\file.ext";
    EZ_TEST_BOOL(p.HasExtension("EXT"));

    p = "This/Is\\My//Path.dot\\file.ext";
    EZ_TEST_BOOL(!p.HasExtension("NEXT"));

    p = "This/Is\\My//Path.dot\\file.extension";
    EZ_TEST_BOOL(!p.HasExtension(".Ext"));

    p = "This/Is\\My//Path.dot\\file.extension";
    EZ_TEST_BOOL(!p.HasExtension("sion"));

    p = "";
    EZ_TEST_BOOL(!p.HasExtension("ext"));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetFileExtension")
  {
    ezStringView p;

    p = "This/Is\\My//Path.dot\\file.extension";
    EZ_TEST_BOOL(p.GetFileExtension() == "extension");

    p = "This/Is\\My//Path.dot\\file";
    EZ_TEST_BOOL(p.GetFileExtension() == "");

    p = "";
    EZ_TEST_BOOL(p.GetFileExtension() == "");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetFileNameAndExtension")
  {
    ezStringView p;

    p = "This/Is\\My//Path.dot\\file.extension";
    EZ_TEST_BOOL(p.GetFileNameAndExtension() == "file.extension");

    p = "This/Is\\My//Path.dot\\.extension";
    EZ_TEST_BOOL(p.GetFileNameAndExtension() == ".extension");

    p = "This/Is\\My//Path.dot\\file";
    EZ_TEST_BOOL(p.GetFileNameAndExtension() == "file");

    p = "\\file";
    EZ_TEST_BOOL(p.GetFileNameAndExtension() == "file");

    p = "";
    EZ_TEST_BOOL(p.GetFileNameAndExtension() == "");

    p = "/";
    EZ_TEST_BOOL(p.GetFileNameAndExtension() == "");

    p = "This/Is\\My//Path.dot\\";
    EZ_TEST_BOOL(p.GetFileNameAndExtension() == "");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetFileName")
  {
    ezStringView p;

    p = "This/Is\\My//Path.dot\\file.extension";
    EZ_TEST_BOOL(p.GetFileName() == "file");

    p = "This/Is\\My//Path.dot\\file";
    EZ_TEST_BOOL(p.GetFileName() == "file");

    p = "\\file";
    EZ_TEST_BOOL(p.GetFileName() == "file");

    p = "";
    EZ_TEST_BOOL(p.GetFileName() == "");

    p = "/";
    EZ_TEST_BOOL(p.GetFileName() == "");

    p = "This/Is\\My//Path.dot\\";
    EZ_TEST_BOOL(p.GetFileName() == "");

    p = "This/Is\\My//Path.dot\\.stupidfile";
    EZ_TEST_BOOL(p.GetFileName() == ".stupidfile");

    p = "This/Is\\My//Path.dot\\.stupidfile.ext";
    EZ_TEST_BOOL(p.GetFileName() == ".stupidfile");

    p = "This/Is\\My//Path.dot\\.stupidfile.ext.";
    EZ_TEST_BOOL(p.GetFileName() == ".stupidfile.ext.");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetFileDirectory")
  {
    ezStringView p;

    p = "This/Is\\My//Path.dot\\file.extension";
    EZ_TEST_BOOL(p.GetFileDirectory() == "This/Is\\My//Path.dot\\");

    p = "This/Is\\My//Path.dot\\.extension";
    EZ_TEST_BOOL(p.GetFileDirectory() == "This/Is\\My//Path.dot\\");

    p = "This/Is\\My//Path.dot\\file";
    EZ_TEST_BOOL(p.GetFileDirectory() == "This/Is\\My//Path.dot\\");

    p = "\\file";
    EZ_TEST_BOOL(p.GetFileDirectory() == "\\");

    p = "";
    EZ_TEST_BOOL(p.GetFileDirectory() == "");

    p = "/";
    EZ_TEST_BOOL(p.GetFileDirectory() == "/");

    p = "This/Is\\My//Path.dot\\";
    EZ_TEST_BOOL(p.GetFileDirectory() == "This/Is\\My//Path.dot\\");

    p = "This";
    EZ_TEST_BOOL(p.GetFileDirectory() == "");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsAbsolutePath / IsRelativePath / IsRootedPath")
  {
    ezStringView p;

    p = "";
    EZ_TEST_BOOL(!p.IsAbsolutePath());
    EZ_TEST_BOOL(p.IsRelativePath());
    EZ_TEST_BOOL(!p.IsRootedPath());

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
    p = "C:\\temp.stuff";
    EZ_TEST_BOOL(p.IsAbsolutePath());
    EZ_TEST_BOOL(!p.IsRelativePath());
    EZ_TEST_BOOL(!p.IsRootedPath());

    p = "C:/temp.stuff";
    EZ_TEST_BOOL(p.IsAbsolutePath());
    EZ_TEST_BOOL(!p.IsRelativePath());
    EZ_TEST_BOOL(!p.IsRootedPath());

    p = "\\\\myserver\\temp.stuff";
    EZ_TEST_BOOL(p.IsAbsolutePath());
    EZ_TEST_BOOL(!p.IsRelativePath());
    EZ_TEST_BOOL(!p.IsRootedPath());

    p = "\\myserver\\temp.stuff";
    EZ_TEST_BOOL(!p.IsAbsolutePath());
    EZ_TEST_BOOL(!p.IsRelativePath()); // neither absolute nor relativ, just stupid
    EZ_TEST_BOOL(!p.IsRootedPath());

    p = "temp.stuff";
    EZ_TEST_BOOL(!p.IsAbsolutePath());
    EZ_TEST_BOOL(p.IsRelativePath());
    EZ_TEST_BOOL(!p.IsRootedPath());

    p = "/temp.stuff";
    EZ_TEST_BOOL(!p.IsAbsolutePath());
    EZ_TEST_BOOL(!p.IsRelativePath()); // bloed
    EZ_TEST_BOOL(!p.IsRootedPath());

    p = "\\temp.stuff";
    EZ_TEST_BOOL(!p.IsAbsolutePath());
    EZ_TEST_BOOL(!p.IsRelativePath()); // bloed
    EZ_TEST_BOOL(!p.IsRootedPath());

    p = "..\\temp.stuff";
    EZ_TEST_BOOL(!p.IsAbsolutePath());
    EZ_TEST_BOOL(p.IsRelativePath());
    EZ_TEST_BOOL(!p.IsRootedPath());

    p = ".\\temp.stuff";
    EZ_TEST_BOOL(!p.IsAbsolutePath());
    EZ_TEST_BOOL(p.IsRelativePath());
    EZ_TEST_BOOL(!p.IsRootedPath());

    p = ":MyDataDir\bla";
    EZ_TEST_BOOL(!p.IsAbsolutePath());
    EZ_TEST_BOOL(!p.IsRelativePath());
    EZ_TEST_BOOL(p.IsRootedPath());

    p = ":\\MyDataDir\bla";
    EZ_TEST_BOOL(!p.IsAbsolutePath());
    EZ_TEST_BOOL(!p.IsRelativePath());
    EZ_TEST_BOOL(p.IsRootedPath());

    p = ":/MyDataDir/bla";
    EZ_TEST_BOOL(!p.IsAbsolutePath());
    EZ_TEST_BOOL(!p.IsRelativePath());
    EZ_TEST_BOOL(p.IsRootedPath());

#elif EZ_ENABLED(EZ_PLATFORM_OSX) || EZ_ENABLED(EZ_PLATFORM_LINUX) || EZ_ENABLED(EZ_PLATFORM_ANDROID)

    p = "C:\\temp.stuff";
    EZ_TEST_BOOL(!p.IsAbsolutePath());
    EZ_TEST_BOOL(p.IsRelativePath());
    EZ_TEST_BOOL(!p.IsRootedPath());

    p = "temp.stuff";
    EZ_TEST_BOOL(!p.IsAbsolutePath());
    EZ_TEST_BOOL(p.IsRelativePath());
    EZ_TEST_BOOL(!p.IsRootedPath());

    p = "/temp.stuff";
    EZ_TEST_BOOL(p.IsAbsolutePath());
    EZ_TEST_BOOL(!p.IsRelativePath());
    EZ_TEST_BOOL(!p.IsRootedPath());

    p = "..\\temp.stuff";
    EZ_TEST_BOOL(!p.IsAbsolutePath());
    EZ_TEST_BOOL(p.IsRelativePath());
    EZ_TEST_BOOL(!p.IsRootedPath());

    p = ".\\temp.stuff";
    EZ_TEST_BOOL(!p.IsAbsolutePath());
    EZ_TEST_BOOL(p.IsRelativePath());
    EZ_TEST_BOOL(!p.IsRootedPath());

#else
#  error "Unknown platform."
#endif
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetRootedPathRootName")
  {
    ezStringView p;

    p = ":root\\bla";
    EZ_TEST_BOOL(p.GetRootedPathRootName() == "root");

    p = ":root/bla";
    EZ_TEST_BOOL(p.GetRootedPathRootName() == "root");

    p = "://root/bla";
    EZ_TEST_BOOL(p.GetRootedPathRootName() == "root");

    p = ":/\\/root\\/bla";
    EZ_TEST_BOOL(p.GetRootedPathRootName() == "root");

    p = "://\\root";
    EZ_TEST_BOOL(p.GetRootedPathRootName() == "root");

    p = ":";
    EZ_TEST_BOOL(p.GetRootedPathRootName() == "");

    p = "";
    EZ_TEST_BOOL(p.GetRootedPathRootName() == "");

    p = "noroot\\bla";
    EZ_TEST_BOOL(p.GetRootedPathRootName() == "");

    p = "C:\\noroot/bla";
    EZ_TEST_BOOL(p.GetRootedPathRootName() == "");

    p = "/noroot/bla";
    EZ_TEST_BOOL(p.GetRootedPathRootName() == "");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetSubString")
  {
    ezStringView s = u8"Пожалуйста, дай мне очень длинные Unicode-стринги!";

    EZ_TEST_BOOL(s.GetElementCount() > ezStringUtils::GetCharacterCount(s.GetStartPointer(), s.GetEndPointer()));

    ezStringView w1 = s.GetSubString(0, 10);
    ezStringView w2 = s.GetSubString(12, 3);
    ezStringView w3 = s.GetSubString(20, 5);
    ezStringView w4 = s.GetSubString(34, 15);
    ezStringView w5 = s.GetSubString(34, 20);
    ezStringView w6 = s.GetSubString(100, 10);

    EZ_TEST_BOOL(w1 == ezStringView(u8"Пожалуйста"));
    EZ_TEST_BOOL(w2 == ezStringView(u8"дай"));
    EZ_TEST_BOOL(w3 == ezStringView(u8"очень"));
    EZ_TEST_BOOL(w4 == ezStringView(u8"Unicode-стринги"));
    EZ_TEST_BOOL(w5 == ezStringView(u8"Unicode-стринги!"));
    EZ_TEST_BOOL(!w6.IsValid());
    EZ_TEST_BOOL(w6 == ezStringView(""));
  }
}
