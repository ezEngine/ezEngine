﻿#include <FoundationTestPCH.h>

// NOTE: always save as Unicode UTF-8 with signature

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/Strings/String.h>

EZ_CREATE_SIMPLE_TEST(Strings, StringBuilder)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor(empty)")
  {
    ezStringBuilder s;

    EZ_TEST_BOOL(s.IsEmpty());
    EZ_TEST_INT(s.GetCharacterCount(), 0);
    EZ_TEST_INT(s.GetElementCount(), 0);
    EZ_TEST_BOOL(s.IsPureASCII());
    EZ_TEST_BOOL(s == "");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor(Utf8)")
  {
    ezStringUtf8 sUtf8(L"abc äöü € def");
    ezStringBuilder s(sUtf8.GetData());

    EZ_TEST_BOOL(s.GetData() != sUtf8.GetData());
    EZ_TEST_BOOL(s == sUtf8.GetData());
    EZ_TEST_INT(s.GetElementCount(), 18);
    EZ_TEST_INT(s.GetCharacterCount(), 13);
    EZ_TEST_BOOL(!s.IsPureASCII());

    ezStringBuilder s2("test test");

    EZ_TEST_BOOL(s2 == "test test");
    EZ_TEST_INT(s2.GetElementCount(), 9);
    EZ_TEST_INT(s2.GetCharacterCount(), 9);
    EZ_TEST_BOOL(s2.IsPureASCII());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor(wchar_t)")
  {
    ezStringUtf8 sUtf8(L"abc äöü € def");
    ezStringBuilder s(L"abc äöü € def");

    EZ_TEST_BOOL(s == sUtf8.GetData());
    EZ_TEST_INT(s.GetElementCount(), 18);
    EZ_TEST_INT(s.GetCharacterCount(), 13);
    EZ_TEST_BOOL(!s.IsPureASCII());

    ezStringBuilder s2(L"test test");

    EZ_TEST_BOOL(s2 == "test test");
    EZ_TEST_INT(s2.GetElementCount(), 9);
    EZ_TEST_INT(s2.GetCharacterCount(), 9);
    EZ_TEST_BOOL(s2.IsPureASCII());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor(copy)")
  {
    ezStringUtf8 sUtf8(L"abc äöü € def");
    ezStringBuilder s(L"abc äöü € def");
    ezStringBuilder s2(s);

    EZ_TEST_BOOL(s2 == sUtf8.GetData());
    EZ_TEST_INT(s2.GetElementCount(), 18);
    EZ_TEST_INT(s2.GetCharacterCount(), 13);
    EZ_TEST_BOOL(!s2.IsPureASCII());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor(StringView)")
  {
    ezStringUtf8 sUtf8(L"abc äöü € def");

    ezStringView it(sUtf8.GetData() + 2, sUtf8.GetData() + 8);

    ezStringBuilder s(it);

    EZ_TEST_INT(s.GetElementCount(), 6);
    EZ_TEST_INT(s.GetCharacterCount(), 4);
    EZ_TEST_BOOL(!s.IsPureASCII());
    EZ_TEST_BOOL(s == ezStringUtf8(L"c äö").GetData());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Constructor(multiple)")
  {
    ezStringUtf8 sUtf8(L"⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺");
    ezStringUtf8 sUtf2(
        L"⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺");

    ezStringBuilder sb(sUtf8.GetData(), sUtf8.GetData(), sUtf8.GetData(), sUtf8.GetData(), sUtf8.GetData(), sUtf8.GetData());

    EZ_TEST_STRING(sb.GetData(), sUtf2.GetData());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator=(Utf8)")
  {
    ezStringUtf8 sUtf8(L"abc äöü € def");
    ezStringBuilder s("bla");
    s = sUtf8.GetData();

    EZ_TEST_BOOL(s.GetData() != sUtf8.GetData());
    EZ_TEST_BOOL(s == sUtf8.GetData());
    EZ_TEST_INT(s.GetElementCount(), 18);
    EZ_TEST_INT(s.GetCharacterCount(), 13);
    EZ_TEST_BOOL(!s.IsPureASCII());

    ezStringBuilder s2("bla");
    s2 = "test test";

    EZ_TEST_BOOL(s2 == "test test");
    EZ_TEST_INT(s2.GetElementCount(), 9);
    EZ_TEST_INT(s2.GetCharacterCount(), 9);
    EZ_TEST_BOOL(s2.IsPureASCII());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator=(wchar_t)")
  {
    ezStringUtf8 sUtf8(L"abc äöü € def");
    ezStringBuilder s("bla");
    s = L"abc äöü € def";

    EZ_TEST_BOOL(s == sUtf8.GetData());
    EZ_TEST_INT(s.GetElementCount(), 18);
    EZ_TEST_INT(s.GetCharacterCount(), 13);
    EZ_TEST_BOOL(!s.IsPureASCII());

    ezStringBuilder s2("bla");
    s2 = L"test test";

    EZ_TEST_BOOL(s2 == "test test");
    EZ_TEST_INT(s2.GetElementCount(), 9);
    EZ_TEST_INT(s2.GetCharacterCount(), 9);
    EZ_TEST_BOOL(s2.IsPureASCII());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator=(copy)")
  {
    ezStringUtf8 sUtf8(L"abc äöü € def");
    ezStringBuilder s(L"abc äöü € def");
    ezStringBuilder s2;
    s2 = s;

    EZ_TEST_BOOL(s2 == sUtf8.GetData());
    EZ_TEST_INT(s2.GetElementCount(), 18);
    EZ_TEST_INT(s2.GetCharacterCount(), 13);
    EZ_TEST_BOOL(!s2.IsPureASCII());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator=(StringView)")
  {
    ezStringBuilder s("abcdefghi");
    ezStringView it(s.GetData() + 2, s.GetData() + 8);
    it.SetStartPosition(s.GetData() + 3);

    s = it;

    EZ_TEST_BOOL(s == "defgh");
    EZ_TEST_INT(s.GetElementCount(), 5);
    EZ_TEST_INT(s.GetCharacterCount(), 5);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "convert to ezStringView")
  {
    ezStringBuilder s(L"aölsdföasld");
    ezStringBuilder tmp;

    ezStringView sv = s;

    EZ_TEST_STRING(sv.GetData(tmp), ezStringUtf8(L"aölsdföasld").GetData());
    EZ_TEST_BOOL(sv == ezStringUtf8(L"aölsdföasld").GetData());

    s = "abcdef";

    EZ_TEST_STRING(sv.GetStartPointer(), "abcdef");
    EZ_TEST_BOOL(sv == "abcdef");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Clear")
  {
    ezStringBuilder s(L"abc äöü € def");

    EZ_TEST_BOOL(!s.IsEmpty());
    EZ_TEST_BOOL(!s.IsPureASCII());

    s.Clear();
    EZ_TEST_BOOL(s.IsEmpty());
    EZ_TEST_INT(s.GetElementCount(), 0);
    EZ_TEST_INT(s.GetCharacterCount(), 0);
    EZ_TEST_BOOL(s.IsPureASCII());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetElementCount / GetCharacterCount / IsPureASCII")
  {
    ezStringBuilder s(L"abc äöü € def");

    EZ_TEST_BOOL(!s.IsPureASCII());
    EZ_TEST_INT(s.GetElementCount(), 18);
    EZ_TEST_INT(s.GetCharacterCount(), 13);

    s = "abc";

    EZ_TEST_BOOL(s.IsPureASCII());
    EZ_TEST_INT(s.GetElementCount(), 3);
    EZ_TEST_INT(s.GetCharacterCount(), 3);

    s = L"Hällo! I love €";

    EZ_TEST_BOOL(!s.IsPureASCII());
    EZ_TEST_INT(s.GetElementCount(), 18);
    EZ_TEST_INT(s.GetCharacterCount(), 15);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Append(single unicode char)")
  {
    ezStringUtf32 u32(L"äöüß");

    ezStringBuilder s("abc");
    EZ_TEST_INT(s.GetCharacterCount(), 3);
    s.Append(u32.GetData()[0]);
    EZ_TEST_INT(s.GetCharacterCount(), 4);

    EZ_TEST_BOOL(s == ezStringUtf8(L"abcä").GetData());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Prepend(single unicode char)")
  {
    ezStringUtf32 u32(L"äöüß");

    ezStringBuilder s("abc");
    EZ_TEST_INT(s.GetCharacterCount(), 3);
    s.Prepend(u32.GetData()[0]);
    EZ_TEST_INT(s.GetCharacterCount(), 4);

    EZ_TEST_BOOL(s == ezStringUtf8(L"äabc").GetData());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Append(char)")
  {
    ezStringBuilder s("abc");
    EZ_TEST_INT(s.GetCharacterCount(), 3);
    s.Append("de", "fg", "hi", ezStringUtf8(L"öä").GetData(), "jk", ezStringUtf8(L"ü€").GetData());
    EZ_TEST_INT(s.GetCharacterCount(), 15);

    EZ_TEST_BOOL(s == ezStringUtf8(L"abcdefghiöäjkü€").GetData());

    s = "pups";
    s.Append(nullptr, "b", nullptr, "d", nullptr, ezStringUtf8(L"ü€").GetData());
    EZ_TEST_BOOL(s == ezStringUtf8(L"pupsbdü€").GetData());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Append(wchar_t)")
  {
    ezStringBuilder s("abc");
    EZ_TEST_INT(s.GetCharacterCount(), 3);
    s.Append(L"de", L"fg", L"hi", L"öä", L"jk", L"ü€");
    EZ_TEST_INT(s.GetCharacterCount(), 15);

    EZ_TEST_BOOL(s == ezStringUtf8(L"abcdefghiöäjkü€").GetData());

    s = "pups";
    s.Append(nullptr, L"b", nullptr, L"d", nullptr, L"ü€");
    EZ_TEST_BOOL(s == ezStringUtf8(L"pupsbdü€").GetData());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Append(multiple)")
  {
    ezStringUtf8 sUtf8(L"⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺");
    ezStringUtf8 sUtf2(L"Test⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺Test2");

    ezStringBuilder sb("Test");
    sb.Append(sUtf8.GetData(), sUtf8.GetData(), sUtf8.GetData(), sUtf8.GetData(), sUtf8.GetData(), "Test2");

    EZ_TEST_STRING(sb.GetData(), sUtf2.GetData());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Set(multiple)")
  {
    ezStringUtf8 sUtf8(L"⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺");
    ezStringUtf8 sUtf2(L"⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺⺅⻩⽇⿕〄㈷㑧䆴ظؼݻ༺Test2");

    ezStringBuilder sb("Test");
    sb.Set(sUtf8.GetData(), sUtf8.GetData(), sUtf8.GetData(), sUtf8.GetData(), sUtf8.GetData(), "Test2");

    EZ_TEST_STRING(sb.GetData(), sUtf2.GetData());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "AppendFormat")
  {
    ezStringBuilder s("abc");
    s.AppendFormat("Test{0}{1}{2}", 42, "foo", ezStringUtf8(L"bär").GetData());

    EZ_TEST_BOOL(s == ezStringUtf8(L"abcTest42foobär").GetData());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Prepend(char)")
  {
    ezStringBuilder s("abc");
    s.Prepend("de", "fg", "hi", ezStringUtf8(L"öä").GetData(), "jk", ezStringUtf8(L"ü€").GetData());

    EZ_TEST_BOOL(s == ezStringUtf8(L"defghiöäjkü€abc").GetData());

    s = "pups";
    s.Prepend(nullptr, "b", nullptr, "d", nullptr, ezStringUtf8(L"ü€").GetData());
    EZ_TEST_BOOL(s == ezStringUtf8(L"bdü€pups").GetData());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Prepend(wchar_t)")
  {
    ezStringBuilder s("abc");
    s.Prepend(L"de", L"fg", L"hi", L"öä", L"jk", L"ü€");

    EZ_TEST_BOOL(s == ezStringUtf8(L"defghiöäjkü€abc").GetData());

    s = "pups";
    s.Prepend(nullptr, L"b", nullptr, L"d", nullptr, L"ü€");
    EZ_TEST_BOOL(s == ezStringUtf8(L"bdü€pups").GetData());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "PrependFormat")
  {
    ezStringBuilder s("abc");
    s.PrependFormat("Test{0}{1}{2}", 42, "foo", ezStringUtf8(L"bär").GetData());

    EZ_TEST_BOOL(s == ezStringUtf8(L"Test42foobärabc").GetData());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Printf")
  {
    ezStringBuilder s("abc");
    s.Printf("Test%i%s%s", 42, "foo", ezStringUtf8(L"bär").GetData());

    EZ_TEST_BOOL(s == ezStringUtf8(L"Test42foobär").GetData());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Format")
  {
    ezStringBuilder s("abc");
    s.Format("Test{0}{1}{2}", 42, "foo", ezStringUtf8(L"bär").GetData());

    EZ_TEST_BOOL(s == ezStringUtf8(L"Test42foobär").GetData());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ToUpper")
  {
    ezStringBuilder s(L"abcdefghijklmnopqrstuvwxyzäöü€ß");
    s.ToUpper();
    EZ_TEST_BOOL(s == ezStringUtf8(L"ABCDEFGHIJKLMNOPQRSTUVWXYZÄÖÜ€ß").GetData());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ToLower")
  {
    ezStringBuilder s(L"ABCDEFGHIJKLMNOPQRSTUVWXYZÄÖÜ€ß");
    s.ToLower();
    EZ_TEST_BOOL(s == ezStringUtf8(L"abcdefghijklmnopqrstuvwxyzäöü€ß").GetData());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Shrink")
  {
    ezStringBuilder s(L"abcdefghijklmnopqrstuvwxyzäöü€ß");
    s.Shrink(5, 3);

    EZ_TEST_BOOL(s == ezStringUtf8(L"fghijklmnopqrstuvwxyzäö").GetData());

    s.Shrink(9, 7);
    EZ_TEST_BOOL(s == ezStringUtf8(L"opqrstu").GetData());

    s.Shrink(3, 2);
    EZ_TEST_BOOL(s == ezStringUtf8(L"rs").GetData());

    s.Shrink(1, 0);
    EZ_TEST_BOOL(s == ezStringUtf8(L"s").GetData());

    s.Shrink(0, 0);
    EZ_TEST_BOOL(s == ezStringUtf8(L"s").GetData());

    s.Shrink(0, 1);
    EZ_TEST_BOOL(s == ezStringUtf8(L"").GetData());

    s.Shrink(10, 0);
    EZ_TEST_BOOL(s == ezStringUtf8(L"").GetData());

    s.Shrink(0, 10);
    EZ_TEST_BOOL(s == ezStringUtf8(L"").GetData());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Reserve")
  {
    ezHeapAllocator allocator("reserve test allocator");
    ezStringBuilder s(L"abcdefghijklmnopqrstuvwxyzäöü€ß", &allocator);
    ezUInt32 characterCountBefore = s.GetCharacterCount();

    s.Reserve(2048);

    EZ_TEST_BOOL(s.GetCharacterCount() == characterCountBefore);

    ezUInt64 iNumAllocs = allocator.GetStats().m_uiNumAllocations;
    s.Append("blablablablablablablablablablablablablablablablablablablablablablablablablablablablablabla");
    EZ_TEST_BOOL(iNumAllocs == allocator.GetStats().m_uiNumAllocations);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Convert to StringView")
  {
    ezStringBuilder s(L"abcdefghijklmnopqrstuvwxyzäöü€ß");
    ezStringView it = s;

    EZ_TEST_BOOL(it.StartsWith(ezStringUtf8(L"abcdefghijklmnopqrstuvwxyzäöü€ß").GetData()));
    EZ_TEST_BOOL(it.EndsWith(ezStringUtf8(L"abcdefghijklmnopqrstuvwxyzäöü€ß").GetData()));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ChangeCharacter")
  {
    ezStringBuilder s(L"abcdefghijklmnopqrstuvwxyzäöü€ß");

    ezStringUtf8 upr(L"ÄÖÜ€ßABCDEFGHIJKLMNOPQRSTUVWXYZ");
    ezStringView view(upr.GetData());

    for (auto it = begin(s); it.IsValid(); ++it, ++view)
    {
      s.ChangeCharacter(it, view.GetCharacter());

      EZ_TEST_BOOL(it.GetCharacter() == view.GetCharacter()); // iterator reflects the changes
    }

    EZ_TEST_BOOL(s == upr.GetData());
    EZ_TEST_INT(s.GetCharacterCount(), 31);
    EZ_TEST_INT(s.GetElementCount(), 37);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ReplaceSubString")
  {
    ezStringBuilder s(L"abcdefghijklmnopqrstuvwxyzäöü€ß");

    s.ReplaceSubString(s.GetData() + 3, s.GetData() + 7, "DEFG"); // equal length, equal num characters
    EZ_TEST_BOOL(s == ezStringUtf8(L"abcDEFGhijklmnopqrstuvwxyzäöü€ß").GetData());
    EZ_TEST_INT(s.GetCharacterCount(), 31);
    EZ_TEST_INT(s.GetElementCount(), 37);

    s.ReplaceSubString(s.GetData() + 7, s.GetData() + 15, ""); // remove
    EZ_TEST_BOOL(s == ezStringUtf8(L"abcDEFGpqrstuvwxyzäöü€ß").GetData());
    EZ_TEST_INT(s.GetCharacterCount(), 23);
    EZ_TEST_INT(s.GetElementCount(), 29);

    s.ReplaceSubString(s.GetData() + 17, s.GetData() + 22, "blablub"); // make longer
    EZ_TEST_BOOL(s == ezStringUtf8(L"abcDEFGpqrstuvwxyblablubü€ß").GetData());
    EZ_TEST_INT(s.GetCharacterCount(), 27);
    EZ_TEST_INT(s.GetElementCount(), 31);

    s.ReplaceSubString(s.GetData() + 22, s.GetData() + 22, ezStringUtf8(L"määh!").GetData()); // insert
    EZ_TEST_BOOL(s == ezStringUtf8(L"abcDEFGpqrstuvwxyblablmääh!ubü€ß").GetData());
    EZ_TEST_INT(s.GetCharacterCount(), 32);
    EZ_TEST_INT(s.GetElementCount(), 38);

    s.ReplaceSubString(s.GetData(), s.GetData() + 10, nullptr); // remove at front
    EZ_TEST_BOOL(s == ezStringUtf8(L"stuvwxyblablmääh!ubü€ß").GetData());
    EZ_TEST_INT(s.GetCharacterCount(), 22);
    EZ_TEST_INT(s.GetElementCount(), 28);

    s.ReplaceSubString(s.GetData() + 18, s.GetData() + 28, nullptr); // remove at back
    EZ_TEST_BOOL(s == ezStringUtf8(L"stuvwxyblablmääh").GetData());
    EZ_TEST_INT(s.GetCharacterCount(), 16);
    EZ_TEST_INT(s.GetElementCount(), 18);

    s.ReplaceSubString(s.GetData(), s.GetData() + 18, nullptr); // clear
    EZ_TEST_BOOL(s == ezStringUtf8(L"").GetData());
    EZ_TEST_INT(s.GetCharacterCount(), 0);
    EZ_TEST_INT(s.GetElementCount(), 0);

    const char* szInsert = "abc def ghi";

    s.ReplaceSubString(s.GetData(), s.GetData(), ezStringView(szInsert, szInsert + 7)); // partial insert into empty
    EZ_TEST_BOOL(s == ezStringUtf8(L"abc def").GetData());
    EZ_TEST_INT(s.GetCharacterCount(), 7);
    EZ_TEST_INT(s.GetElementCount(), 7);

    // insert very large block
    s = ezStringBuilder("a"); // hard reset to keep buffer small
    ezString insertString("omfg this string is so long it possibly won't never ever ever ever fit into the current buffer - this will "
                          "hopefully lead to a buffer resize :)"
                          "................................................................................................................"
                          "........................................................"
                          "................................................................................................................"
                          "........................................................"
                          "................................................................................................................"
                          "........................................................"
                          "................................................................................................................"
                          "........................................................"
                          "................................................................................................................"
                          "........................................................"
                          "................................................................................................................"
                          "........................................................");
    s.ReplaceSubString(s.GetData(), s.GetData() + s.GetElementCount(), insertString.GetData());
    EZ_TEST_BOOL(s == insertString.GetData());
    EZ_TEST_INT(s.GetCharacterCount(), insertString.GetCharacterCount());
    EZ_TEST_INT(s.GetElementCount(), insertString.GetElementCount());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Insert")
  {
    ezStringBuilder s;

    s.Insert(s.GetData(), "test");
    EZ_TEST_BOOL(s == "test");

    s.Insert(s.GetData() + 2, "TUT");
    EZ_TEST_BOOL(s == "teTUTst");

    s.Insert(s.GetData(), "MOEP");
    EZ_TEST_BOOL(s == "MOEPteTUTst");

    s.Insert(s.GetData() + s.GetElementCount(), "hompf");
    EZ_TEST_BOOL(s == "MOEPteTUTsthompf");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Remove")
  {
    ezStringBuilder s("MOEPteTUTsthompf");

    s.Remove(s.GetData() + 11, s.GetData() + s.GetElementCount());
    EZ_TEST_BOOL(s == "MOEPteTUTst");

    s.Remove(s.GetData(), s.GetData() + 4);
    EZ_TEST_BOOL(s == "teTUTst");

    s.Remove(s.GetData() + 2, s.GetData() + 5);
    EZ_TEST_BOOL(s == "test");

    s.Remove(s.GetData(), s.GetData() + s.GetElementCount());
    EZ_TEST_BOOL(s == "");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ReplaceFirst")
  {
    ezStringBuilder s = "abc def abc def ghi abc ghi";

    s.ReplaceFirst("def", "BLOED");
    EZ_TEST_BOOL(s == "abc BLOED abc def ghi abc ghi");

    s.ReplaceFirst("abc", "BLOED");
    EZ_TEST_BOOL(s == "BLOED BLOED abc def ghi abc ghi");

    s.ReplaceFirst("abc", "BLOED", s.GetData() + 15);
    EZ_TEST_BOOL(s == "BLOED BLOED abc def ghi BLOED ghi");

    s.ReplaceFirst("ghi", "LAANGWEILIG");
    EZ_TEST_BOOL(s == "BLOED BLOED abc def LAANGWEILIG BLOED ghi");

    s.ReplaceFirst("ghi", "LAANGWEILIG");
    EZ_TEST_BOOL(s == "BLOED BLOED abc def LAANGWEILIG BLOED LAANGWEILIG");

    s.ReplaceFirst("def", "OEDE");
    EZ_TEST_BOOL(s == "BLOED BLOED abc OEDE LAANGWEILIG BLOED LAANGWEILIG");

    s.ReplaceFirst("abc", "BLOEDE");
    EZ_TEST_BOOL(s == "BLOED BLOED BLOEDE OEDE LAANGWEILIG BLOED LAANGWEILIG");

    s.ReplaceFirst("BLOED BLOED BLOEDE OEDE LAANGWEILIG BLOED LAANGWEILIG", "weg");
    EZ_TEST_BOOL(s == "weg");

    s.ReplaceFirst("weg", nullptr);
    EZ_TEST_BOOL(s == "");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ReplaceLast")
  {
    ezStringBuilder s = "abc def abc def ghi abc ghi";

    s.ReplaceLast("abc", "ABC");
    EZ_TEST_BOOL(s == "abc def abc def ghi ABC ghi");

    s.ReplaceLast("abc", "ABC");
    EZ_TEST_BOOL(s == "abc def ABC def ghi ABC ghi");

    s.ReplaceLast("abc", "ABC");
    EZ_TEST_BOOL(s == "ABC def ABC def ghi ABC ghi");

    s.ReplaceLast("ghi", "GHI", s.GetData() + 24);
    EZ_TEST_BOOL(s == "ABC def ABC def GHI ABC ghi");

    s.ReplaceLast("i", "I");
    EZ_TEST_BOOL(s == "ABC def ABC def GHI ABC ghI");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ReplaceAll")
  {
    ezStringBuilder s = "abc def abc def ghi abc ghi";

    s.ReplaceAll("abc", "TEST");
    EZ_TEST_BOOL(s == "TEST def TEST def ghi TEST ghi");

    s.ReplaceAll("def", "def");
    EZ_TEST_BOOL(s == "TEST def TEST def ghi TEST ghi");

    s.ReplaceAll("def", "defdef");
    EZ_TEST_BOOL(s == "TEST defdef TEST defdef ghi TEST ghi");

    s.ReplaceAll("def", "defdef");
    EZ_TEST_BOOL(s == "TEST defdefdefdef TEST defdefdefdef ghi TEST ghi");

    s.ReplaceAll("def", " ");
    EZ_TEST_BOOL(s == "TEST      TEST      ghi TEST ghi");

    s.ReplaceAll(" ", "");
    EZ_TEST_BOOL(s == "TESTTESTghiTESTghi");

    s.ReplaceAll("TEST", "a");
    EZ_TEST_BOOL(s == "aaghiaghi");

    s.ReplaceAll("hi", "hihi");
    EZ_TEST_BOOL(s == "aaghihiaghihi");

    s.ReplaceAll("ag", " ");
    EZ_TEST_BOOL(s == "a hihi hihi");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ReplaceFirst_NoCase")
  {
    ezStringBuilder s = "abc def abc def ghi abc ghi";

    s.ReplaceFirst_NoCase("DEF", "BLOED");
    EZ_TEST_BOOL(s == "abc BLOED abc def ghi abc ghi");

    s.ReplaceFirst_NoCase("ABC", "BLOED");
    EZ_TEST_BOOL(s == "BLOED BLOED abc def ghi abc ghi");

    s.ReplaceFirst_NoCase("ABC", "BLOED", s.GetData() + 15);
    EZ_TEST_BOOL(s == "BLOED BLOED abc def ghi BLOED ghi");

    s.ReplaceFirst_NoCase("GHI", "LAANGWEILIG");
    EZ_TEST_BOOL(s == "BLOED BLOED abc def LAANGWEILIG BLOED ghi");

    s.ReplaceFirst_NoCase("GHI", "LAANGWEILIG");
    EZ_TEST_BOOL(s == "BLOED BLOED abc def LAANGWEILIG BLOED LAANGWEILIG");

    s.ReplaceFirst_NoCase("DEF", "OEDE");
    EZ_TEST_BOOL(s == "BLOED BLOED abc OEDE LAANGWEILIG BLOED LAANGWEILIG");

    s.ReplaceFirst_NoCase("ABC", "BLOEDE");
    EZ_TEST_BOOL(s == "BLOED BLOED BLOEDE OEDE LAANGWEILIG BLOED LAANGWEILIG");

    s.ReplaceFirst_NoCase("BLOED BLOED BLOEDE OEDE LAANGWEILIG BLOED LAANGWEILIG", "weg");
    EZ_TEST_BOOL(s == "weg");

    s.ReplaceFirst_NoCase("WEG", nullptr);
    EZ_TEST_BOOL(s == "");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ReplaceLast_NoCase")
  {
    ezStringBuilder s = "abc def abc def ghi abc ghi";

    s.ReplaceLast_NoCase("abc", "ABC");
    EZ_TEST_BOOL(s == "abc def abc def ghi ABC ghi");

    s.ReplaceLast_NoCase("aBc", "ABC");
    EZ_TEST_BOOL(s == "abc def abc def ghi ABC ghi");

    s.ReplaceLast_NoCase("ABC", "ABC");
    EZ_TEST_BOOL(s == "abc def abc def ghi ABC ghi");

    s.ReplaceLast_NoCase("GHI", "GHI", s.GetData() + 24);
    EZ_TEST_BOOL(s == "abc def abc def GHI ABC ghi");

    s.ReplaceLast_NoCase("I", "I");
    EZ_TEST_BOOL(s == "abc def abc def GHI ABC ghI");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ReplaceAll_NoCase")
  {
    ezStringBuilder s = "abc def abc def ghi abc ghi";

    s.ReplaceAll_NoCase("ABC", "TEST");
    EZ_TEST_BOOL(s == "TEST def TEST def ghi TEST ghi");

    s.ReplaceAll_NoCase("DEF", "def");
    EZ_TEST_BOOL(s == "TEST def TEST def ghi TEST ghi");

    s.ReplaceAll_NoCase("DEF", "defdef");
    EZ_TEST_BOOL(s == "TEST defdef TEST defdef ghi TEST ghi");

    s.ReplaceAll_NoCase("DEF", "defdef");
    EZ_TEST_BOOL(s == "TEST defdefdefdef TEST defdefdefdef ghi TEST ghi");

    s.ReplaceAll_NoCase("DEF", " ");
    EZ_TEST_BOOL(s == "TEST      TEST      ghi TEST ghi");

    s.ReplaceAll_NoCase(" ", "");
    EZ_TEST_BOOL(s == "TESTTESTghiTESTghi");

    s.ReplaceAll_NoCase("teST", "a");
    EZ_TEST_BOOL(s == "aaghiaghi");

    s.ReplaceAll_NoCase("hI", "hihi");
    EZ_TEST_BOOL(s == "aaghihiaghihi");

    s.ReplaceAll_NoCase("Ag", " ");
    EZ_TEST_BOOL(s == "a hihi hihi");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ReplaceWholeWord")
  {
    ezStringBuilder s = "abcd abc abcd abc dabc abc";

    EZ_TEST_BOOL(s.ReplaceWholeWord("abc", "def", ezStringUtils::IsWordDelimiter_English) != nullptr);
    EZ_TEST_BOOL(s == "abcd def abcd abc dabc abc");

    EZ_TEST_BOOL(s.ReplaceWholeWord("abc", "def", ezStringUtils::IsWordDelimiter_English) != nullptr);
    EZ_TEST_BOOL(s == "abcd def abcd def dabc abc");

    EZ_TEST_BOOL(s.ReplaceWholeWord("abc", "def", ezStringUtils::IsWordDelimiter_English) != nullptr);
    EZ_TEST_BOOL(s == "abcd def abcd def dabc def");

    EZ_TEST_BOOL(s.ReplaceWholeWord("abc", "def", ezStringUtils::IsWordDelimiter_English) == nullptr);
    EZ_TEST_BOOL(s == "abcd def abcd def dabc def");

    EZ_TEST_BOOL(s.ReplaceWholeWord("abcd", "def", ezStringUtils::IsWordDelimiter_English) != nullptr);
    EZ_TEST_BOOL(s == "def def abcd def dabc def");

    EZ_TEST_BOOL(s.ReplaceWholeWord("abcd", "def", ezStringUtils::IsWordDelimiter_English) != nullptr);
    EZ_TEST_BOOL(s == "def def def def dabc def");

    EZ_TEST_BOOL(s.ReplaceWholeWord("abcd", "def", ezStringUtils::IsWordDelimiter_English) == nullptr);
    EZ_TEST_BOOL(s == "def def def def dabc def");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ReplaceWholeWord_NoCase")
  {
    ezStringBuilder s = "abcd abc abcd abc dabc abc";

    EZ_TEST_BOOL(s.ReplaceWholeWord_NoCase("ABC", "def", ezStringUtils::IsWordDelimiter_English) != nullptr);
    EZ_TEST_BOOL(s == "abcd def abcd abc dabc abc");

    EZ_TEST_BOOL(s.ReplaceWholeWord_NoCase("ABC", "def", ezStringUtils::IsWordDelimiter_English) != nullptr);
    EZ_TEST_BOOL(s == "abcd def abcd def dabc abc");

    EZ_TEST_BOOL(s.ReplaceWholeWord_NoCase("ABC", "def", ezStringUtils::IsWordDelimiter_English) != nullptr);
    EZ_TEST_BOOL(s == "abcd def abcd def dabc def");

    EZ_TEST_BOOL(s.ReplaceWholeWord_NoCase("ABC", "def", ezStringUtils::IsWordDelimiter_English) == nullptr);
    EZ_TEST_BOOL(s == "abcd def abcd def dabc def");

    EZ_TEST_BOOL(s.ReplaceWholeWord_NoCase("ABCd", "def", ezStringUtils::IsWordDelimiter_English) != nullptr);
    EZ_TEST_BOOL(s == "def def abcd def dabc def");

    EZ_TEST_BOOL(s.ReplaceWholeWord_NoCase("aBCD", "def", ezStringUtils::IsWordDelimiter_English) != nullptr);
    EZ_TEST_BOOL(s == "def def def def dabc def");

    EZ_TEST_BOOL(s.ReplaceWholeWord_NoCase("ABcd", "def", ezStringUtils::IsWordDelimiter_English) == nullptr);
    EZ_TEST_BOOL(s == "def def def def dabc def");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ReplaceWholeWordAll")
  {
    ezStringBuilder s = "abcd abc abcd abc dabc abc";

    EZ_TEST_INT(s.ReplaceWholeWordAll("abc", "def", ezStringUtils::IsWordDelimiter_English), 3);
    EZ_TEST_BOOL(s == "abcd def abcd def dabc def");

    EZ_TEST_INT(s.ReplaceWholeWordAll("abc", "def", ezStringUtils::IsWordDelimiter_English), 0);
    EZ_TEST_BOOL(s == "abcd def abcd def dabc def");

    EZ_TEST_INT(s.ReplaceWholeWordAll("abcd", "def", ezStringUtils::IsWordDelimiter_English), 2);
    EZ_TEST_BOOL(s == "def def def def dabc def");

    EZ_TEST_INT(s.ReplaceWholeWordAll("abcd", "def", ezStringUtils::IsWordDelimiter_English), 0);
    EZ_TEST_BOOL(s == "def def def def dabc def");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ReplaceWholeWordAll_NoCase")
  {
    ezStringBuilder s = "abcd abc abcd abc dabc abc";

    EZ_TEST_INT(s.ReplaceWholeWordAll_NoCase("ABC", "def", ezStringUtils::IsWordDelimiter_English), 3);
    EZ_TEST_BOOL(s == "abcd def abcd def dabc def");

    EZ_TEST_INT(s.ReplaceWholeWordAll_NoCase("ABC", "def", ezStringUtils::IsWordDelimiter_English), 0);
    EZ_TEST_BOOL(s == "abcd def abcd def dabc def");

    EZ_TEST_INT(s.ReplaceWholeWordAll_NoCase("ABCd", "def", ezStringUtils::IsWordDelimiter_English), 2);
    EZ_TEST_BOOL(s == "def def def def dabc def");

    EZ_TEST_INT(s.ReplaceWholeWordAll_NoCase("ABCd", "def", ezStringUtils::IsWordDelimiter_English), 0);
    EZ_TEST_BOOL(s == "def def def def dabc def");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "teset")
  {
    const char* sz = "abc def";
    ezStringView it(sz);

    ezStringBuilder s = it;
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Split")
  {
    ezStringBuilder s = "|abc,def<>ghi|,<>jkl|mno,pqr|stu";

    ezHybridArray<ezStringView, 32> SubStrings;

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


  EZ_TEST_BLOCK(ezTestBlock::Enabled, "MakeCleanPath")
  {
    ezStringBuilder p;

    p = "C:\\temp/temp//tut";
    p.MakeCleanPath();
    EZ_TEST_BOOL(p == "C:/temp/temp/tut");

    p = "\\temp/temp//tut\\\\";
    p.MakeCleanPath();
    EZ_TEST_BOOL(p == "/temp/temp/tut/");

    p = "\\";
    p.MakeCleanPath();
    EZ_TEST_BOOL(p == "/");

    p = "file";
    p.MakeCleanPath();
    EZ_TEST_BOOL(p == "file");

    p = "C:\\temp/..//tut";
    p.MakeCleanPath();
    EZ_TEST_BOOL(p == "C:/tut");

    p = "C:\\temp/..";
    p.MakeCleanPath();
    EZ_TEST_BOOL(p == "C:/temp/..");

    p = "C:\\temp/..\\";
    p.MakeCleanPath();
    EZ_TEST_BOOL(p == "C:/");

    p = "\\//temp/../bla\\\\blub///..\\temp//tut/tat/..\\\\..\\//ploep";
    p.MakeCleanPath();
    EZ_TEST_BOOL(p == "//bla/temp/ploep");

    p = "a/b/c/../../../../e/f";
    p.MakeCleanPath();
    EZ_TEST_BOOL(p == "../e/f");

    p = "/../../a/../../e/f";
    p.MakeCleanPath();
    EZ_TEST_BOOL(p == "../../e/f");

    p = "/../../a/../../e/f/../";
    p.MakeCleanPath();
    EZ_TEST_BOOL(p == "../../e/");

    p = "/../../a/../../e/f/..";
    p.MakeCleanPath();
    EZ_TEST_BOOL(p == "../../e/f/..");

    p = "\\//temp/./bla\\\\blub///.\\temp//tut/tat/..\\.\\.\\//ploep";
    p.MakeCleanPath();
    EZ_TEST_STRING(p.GetData(), "//temp/bla/blub/temp/tut/ploep");

    p = "./";
    p.MakeCleanPath();
    EZ_TEST_STRING(p.GetData(), "");

    p = "/./././";
    p.MakeCleanPath();
    EZ_TEST_STRING(p.GetData(), "/");

    p = "./.././";
    p.MakeCleanPath();
    EZ_TEST_STRING(p.GetData(), "../");

    // more than two dots are invalid, so the should be kept as is
    p = "./..././abc/...\\def";
    p.MakeCleanPath();
    EZ_TEST_STRING(p.GetData(), ".../abc/.../def");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "PathParentDirectory")
  {
    ezStringBuilder p;

    p = "C:\\temp/temp//tut";
    p.PathParentDirectory();
    EZ_TEST_BOOL(p == "C:/temp/temp/");

    p = "C:\\temp/temp//tut\\\\";
    p.PathParentDirectory();
    EZ_TEST_BOOL(p == "C:/temp/temp/");

    p = "file";
    p.PathParentDirectory();
    EZ_TEST_BOOL(p == "");

    p = "/file";
    p.PathParentDirectory();
    EZ_TEST_BOOL(p == "/");

    p = "C:\\temp/..//tut";
    p.PathParentDirectory();
    EZ_TEST_BOOL(p == "C:/");

    p = "file";
    p.PathParentDirectory(3);
    EZ_TEST_BOOL(p == "../../");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "AppendPath")
  {
    ezStringBuilder p;

    p = "this/is\\my//path";
    p.AppendPath("orly/nowai");
    EZ_TEST_BOOL(p == "this/is\\my//path/orly/nowai");

    p = "this/is\\my//path///";
    p.AppendPath("orly/nowai");
    EZ_TEST_BOOL(p == "this/is\\my//path///orly/nowai");

    p = "";
    p.AppendPath("orly/nowai");
    EZ_TEST_BOOL(p == "orly/nowai");

    // It should be valid to append an absolute path to an empty string.
    {
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
      const char* szAbsPath = "C:\\folder";
      const char* szAbsPathAppendResult = "C:\\folder/File.ext";
#elif EZ_ENABLED(EZ_PLATFORM_OSX) || EZ_ENABLED(EZ_PLATFORM_LINUX)
      const char* szAbsPath = "/folder";
      const char* szAbsPathAppendResult = "/folder/File.ext";
#else
#error "An absolute path example must be defined for the 'AppendPath' test for each platform!"
#endif

      p = "";
      p.AppendPath(szAbsPath, "File.ext");
      EZ_TEST_BOOL(p == szAbsPathAppendResult);
    }

    p = "bla";
    p.AppendPath("");
    EZ_TEST_BOOL(p == "bla");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ChangeFileName")
  {
    ezStringBuilder p;

    p = "C:/test/test/tut.ext";
    p.ChangeFileName("bla");
    EZ_TEST_BOOL(p == "C:/test/test/bla.ext");

    p = "test/test/tut/troet.toeff";
    p.ChangeFileName("toeff");
    EZ_TEST_BOOL(p == "test/test/tut/toeff.toeff");

    p = "test/test/tut/murpf";
    p.ChangeFileName("toeff");
    EZ_TEST_BOOL(p == "test/test/tut/toeff");

    p = "test/test/tut/murpf/";
    p.ChangeFileName("toeff");
    EZ_TEST_BOOL(p == "test/test/tut/murpf/toeff"); // filename is EMPTY -> thus ADDS it

    p = "test/test/tut/murpf/.extension"; // folders that start with a dot must be considered to be empty filenames with an extension
    p.ChangeFileName("toeff");
    EZ_TEST_BOOL(p == "test/test/tut/murpf/toeff.extension");

    p = "test/test/tut/murpf/.extension/"; // folders that start with a dot ARE considered as folders, if the path ends with a slash
    p.ChangeFileName("toeff");
    EZ_TEST_BOOL(p == "test/test/tut/murpf/.extension/toeff");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ChangeFileNameAndExtension")
  {
    ezStringBuilder p;

    p = "C:/test/test/tut.ext";
    p.ChangeFileNameAndExtension("bla.pups");
    EZ_TEST_BOOL(p == "C:/test/test/bla.pups");

    p = "test/test/tut/troet.toeff";
    p.ChangeFileNameAndExtension("toeff");
    EZ_TEST_BOOL(p == "test/test/tut/toeff");

    p = "test/test/tut/murpf";
    p.ChangeFileNameAndExtension("toeff.tut");
    EZ_TEST_BOOL(p == "test/test/tut/toeff.tut");

    p = "test/test/tut/murpf/";
    p.ChangeFileNameAndExtension("toeff.blo");
    EZ_TEST_BOOL(p == "test/test/tut/murpf/toeff.blo"); // filename is EMPTY -> thus ADDS it

    p = "test/test/tut/murpf/.extension"; // folders that start with a dot must be considered to be empty filenames with an extension
    p.ChangeFileNameAndExtension("toeff.ext");
    EZ_TEST_BOOL(p == "test/test/tut/murpf/toeff.ext");

    p = "test/test/tut/murpf/.extension/"; // folders that start with a dot ARE considered as folders, if the path ends with a slash
    p.ChangeFileNameAndExtension("toeff");
    EZ_TEST_BOOL(p == "test/test/tut/murpf/.extension/toeff");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ChangeFileExtension")
  {
    ezStringBuilder p;

    p = "C:/test/test/tut.ext";
    p.ChangeFileExtension("pups");
    EZ_TEST_BOOL(p == "C:/test/test/tut.pups");

    p = "C:/test/test/tut";
    p.ChangeFileExtension("pups");
    EZ_TEST_BOOL(p == "C:/test/test/tut.pups");

    p = "C:/test/test/tut.ext";
    p.ChangeFileExtension("");
    EZ_TEST_BOOL(p == "C:/test/test/tut.");

    p = "C:/test/test/tut";
    p.ChangeFileExtension("");
    EZ_TEST_BOOL(p == "C:/test/test/tut.");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "HasAnyExtension")
  {
    ezStringBuilder p = "This/Is\\My//Path.dot\\file.extension";
    EZ_TEST_BOOL(p.HasAnyExtension());

    p = "This/Is\\My//Path.dot\\file_no_extension";
    EZ_TEST_BOOL(!p.HasAnyExtension());
    EZ_TEST_BOOL(!p.HasAnyExtension());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "HasExtension")
  {
    ezStringBuilder p;

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
    ezStringBuilder p;

    p = "This/Is\\My//Path.dot\\file.extension";
    EZ_TEST_BOOL(p.GetFileExtension() == "extension");

    p = "This/Is\\My//Path.dot\\file";
    EZ_TEST_BOOL(p.GetFileExtension() == "");

    p = "";
    EZ_TEST_BOOL(p.GetFileExtension() == "");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetFileNameAndExtension")
  {
    ezStringBuilder p;

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
    ezStringBuilder p;

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

    // so far we treat file and folders whose names start with a '.' as extensions
    p = "This/Is\\My//Path.dot\\.stupidfile";
    EZ_TEST_BOOL(p.GetFileName() == "");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetFileDirectory")
  {
    ezStringBuilder p;

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
    ezStringBuilder p;

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

#elif EZ_ENABLED(EZ_PLATFORM_OSX) || EZ_ENABLED(EZ_PLATFORM_LINUX)

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
#error "Unknown platform."
#endif
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetRootedPathRootName")
  {
    ezStringBuilder p;

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

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsPathBelowFolder")
  {
    ezStringBuilder p;

    p = "a/b\\c//d\\\\e/f";
    EZ_TEST_BOOL(!p.IsPathBelowFolder("/a/b\\c"));
    EZ_TEST_BOOL(p.IsPathBelowFolder("a/b\\c"));
    EZ_TEST_BOOL(p.IsPathBelowFolder("a/b\\c//"));
    EZ_TEST_BOOL(p.IsPathBelowFolder("a/b\\c//d/\\e\\f")); // equal paths are considered 'below'
    EZ_TEST_BOOL(!p.IsPathBelowFolder("a/b\\c//d/\\e\\f/g"));
    EZ_TEST_BOOL(p.IsPathBelowFolder("a"));
    EZ_TEST_BOOL(!p.IsPathBelowFolder("b"));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "MakeRelativeTo")
  {
    ezStringBuilder p;

    p = "a/b\\c/d\\\\e/f/g";
    EZ_TEST_BOOL(p.MakeRelativeTo("a\\b/c").Succeeded());
    EZ_TEST_BOOL(p == "d/e/f/g");
    EZ_TEST_BOOL(p.MakeRelativeTo("a\\b/c").Failed());
    EZ_TEST_BOOL(p == "d/e/f/g");

    p = "a/b\\c//d\\\\e/f/g";
    EZ_TEST_BOOL(p.MakeRelativeTo("a\\b/c").Succeeded());
    EZ_TEST_BOOL(p == "d/e/f/g");
    EZ_TEST_BOOL(p.MakeRelativeTo("a\\b/c").Failed());
    EZ_TEST_BOOL(p == "d/e/f/g");

    p = "a/b\\c/d\\\\e/f/g";
    EZ_TEST_BOOL(p.MakeRelativeTo("a\\b/c/").Succeeded());
    EZ_TEST_BOOL(p == "d/e/f/g");
    EZ_TEST_BOOL(p.MakeRelativeTo("a\\b/c/").Failed());
    EZ_TEST_BOOL(p == "d/e/f/g");

    p = "a/b\\c//d\\\\e/f/g";
    EZ_TEST_BOOL(p.MakeRelativeTo("a\\b/c/").Succeeded());
    EZ_TEST_BOOL(p == "d/e/f/g");
    EZ_TEST_BOOL(p.MakeRelativeTo("a\\b/c/").Failed());
    EZ_TEST_BOOL(p == "d/e/f/g");

    p = "a/b\\c//d\\\\e/f/g";
    EZ_TEST_BOOL(p.MakeRelativeTo("a\\b/c\\/d/\\e\\f/g").Succeeded());
    EZ_TEST_BOOL(p == "");
    EZ_TEST_BOOL(p.MakeRelativeTo("a\\b/c\\/d/\\e\\f/g").Failed());
    EZ_TEST_BOOL(p == "");

    p = "a/b\\c//d\\\\e/f/g/";
    EZ_TEST_BOOL(p.MakeRelativeTo("a\\b/c\\/d//e\\f/g\\h/i").Succeeded());
    EZ_TEST_BOOL(p == "../../");
    EZ_TEST_BOOL(p.MakeRelativeTo("a\\b/c\\/d//e\\f/g\\h/i").Failed());
    EZ_TEST_BOOL(p == "../../");

    p = "a/b\\c//d\\\\e/f/g/j/k";
    EZ_TEST_BOOL(p.MakeRelativeTo("a\\b/c\\/d//e\\f/g\\h/i").Succeeded());
    EZ_TEST_BOOL(p == "../../j/k");
    EZ_TEST_BOOL(p.MakeRelativeTo("a\\b/c\\/d//e\\f/g\\h/i").Failed());
    EZ_TEST_BOOL(p == "../../j/k");

    p = "a/b\\c//d\\\\e/f/ge";
    EZ_TEST_BOOL(p.MakeRelativeTo("a\\b/c//d/\\e\\f/g\\h/i").Succeeded());
    EZ_TEST_BOOL(p == "../../../ge");
    EZ_TEST_BOOL(p.MakeRelativeTo("a\\b/c//d/\\e\\f/g\\h/i").Failed());
    EZ_TEST_BOOL(p == "../../../ge");

    p = "a/b\\c//d\\\\e/f/g.txt";
    EZ_TEST_BOOL(p.MakeRelativeTo("a\\b/c//d//e\\f/g\\h/i").Succeeded());
    EZ_TEST_BOOL(p == "../../../g.txt");
    EZ_TEST_BOOL(p.MakeRelativeTo("a\\b/c//d//e\\f/g\\h/i").Failed());
    EZ_TEST_BOOL(p == "../../../g.txt");

    p = "a/b\\c//d\\\\e/f/g";
    EZ_TEST_BOOL(p.MakeRelativeTo("a\\b/c//d//e\\f/g\\h/i").Succeeded());
    EZ_TEST_BOOL(p == "../../");
    EZ_TEST_BOOL(p.MakeRelativeTo("a\\b/c//d//e\\f/g\\h/i").Failed());
    EZ_TEST_BOOL(p == "../../");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "MakePathSeparatorsNative")
  {
    ezStringBuilder p;
    p = "This/is\\a/temp\\\\path//to/my///file";

    p.MakePathSeparatorsNative();

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
    EZ_TEST_STRING(p.GetData(), "This\\is\\a\\temp\\path\\to\\my\\file");
#else
    EZ_TEST_STRING(p.GetData(), "This/is/a/temp/path/to/my/file");
#endif
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ReadAll")
  {
    ezMemoryStreamStorage StreamStorage;

    ezMemoryStreamWriter MemoryWriter(&StreamStorage);
    ezMemoryStreamReader MemoryReader(&StreamStorage);

    const char* szText =
        "l;kjasdflkjdfasjlk asflkj asfljwe oiweq2390432 4 @#$ otrjk3l;2rlkhitoqhrn324:R l324h32kjr hnasfhsakfh234fas1440687873242321245";

    MemoryWriter.WriteBytes(szText, ezStringUtils::GetStringElementCount(szText));

    ezStringBuilder s;
    s.ReadAll(MemoryReader);

    EZ_TEST_BOOL(s == szText);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetSubString_FromTo")
  {
    ezStringBuilder sb = "basf";

    const char* sz = "abcdefghijklmnopqrstuvwxyz";

    sb.SetSubString_FromTo(sz + 5, sz + 13);
    EZ_TEST_BOOL(sb == "fghijklm");

    sb.SetSubString_FromTo(sz + 17, sz + 30);
    EZ_TEST_BOOL(sb == "rstuvwxyz");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetSubString_ElementCount")
  {
    ezStringBuilder sb = "basf";

    ezStringUtf8 sz(L"aäbcödefügh");

    sb.SetSubString_ElementCount(sz.GetData() + 5, 5);
    EZ_TEST_BOOL(sb == ezStringUtf8(L"ödef").GetData());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetSubString_CharacterCount")
  {
    ezStringBuilder sb = "basf";

    ezStringUtf8 sz(L"aäbcödefgh");

    sb.SetSubString_CharacterCount(sz.GetData() + 5, 5);
    EZ_TEST_BOOL(sb == ezStringUtf8(L"ödefg").GetData());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "RemoveFileExtension")
  {
    ezStringBuilder sb = L"⺅⻩⽇⿕.〄㈷㑧䆴.ؼݻ༺.";

    sb.RemoveFileExtension();
    EZ_TEST_STRING_UNICODE(sb.GetData(), ezStringUtf8(L"⺅⻩⽇⿕.〄㈷㑧䆴.ؼݻ༺").GetData());

    sb.RemoveFileExtension();
    EZ_TEST_STRING_UNICODE(sb.GetData(), ezStringUtf8(L"⺅⻩⽇⿕.〄㈷㑧䆴").GetData());

    sb.RemoveFileExtension();
    EZ_TEST_STRING_UNICODE(sb.GetData(), ezStringUtf8(L"⺅⻩⽇⿕").GetData());

    sb.RemoveFileExtension();
    EZ_TEST_STRING_UNICODE(sb.GetData(), ezStringUtf8(L"⺅⻩⽇⿕").GetData());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Trim")
  {
    // Empty input
    ezStringBuilder sb = L"";
    sb.Trim(" \t");
    EZ_TEST_STRING(sb.GetData(), ezStringUtf8(L"").GetData());
    sb.Trim(nullptr, " \t");
    EZ_TEST_STRING(sb.GetData(), ezStringUtf8(L"").GetData());
    sb.Trim(" \t", nullptr);
    EZ_TEST_STRING(sb.GetData(), ezStringUtf8(L"").GetData());

    // Clear all from one side
    auto sUnicode = L"私はクリストハさんです";
    sb = sUnicode;
    sb.Trim(nullptr, ezStringUtf8(sUnicode).GetData());
    EZ_TEST_STRING(sb.GetData(), "");
    sb = sUnicode;
    sb.Trim(ezStringUtf8(sUnicode).GetData(), nullptr);
    EZ_TEST_STRING(sb.GetData(), "");

    // Clear partial side
    sb = L"ですですですAにぱにぱにぱ";
    sb.Trim(nullptr, ezStringUtf8(L"にぱ").GetData());
    EZ_TEST_STRING_UNICODE(sb.GetData(), ezStringUtf8(L"ですですですA").GetData());
    sb.Trim(ezStringUtf8(L"です").GetData(), nullptr);
    EZ_TEST_STRING_UNICODE(sb.GetData(), ezStringUtf8(L"A").GetData());

    sb = L"ですですですAにぱにぱにぱ";
    sb.Trim(ezStringUtf8(L"ですにぱ").GetData());
    EZ_TEST_STRING(sb.GetData(), ezStringUtf8(L"A").GetData());
  }
}
