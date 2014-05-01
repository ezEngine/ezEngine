#include <PCH.h>
#include <Foundation/Strings/String.h>

EZ_CREATE_SIMPLE_TEST(Strings, PathUtils)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsPathSeparator")
  {
    for (int i = 0; i < 0xFFFF; ++i)
    {
      if (i == '/')
      {
        EZ_TEST_BOOL(ezPathUtils::IsPathSeparator(i));
      }
      else
      if (i == '\\')
      {
        EZ_TEST_BOOL(ezPathUtils::IsPathSeparator(i));
      }
      else
      {
        EZ_TEST_BOOL(!ezPathUtils::IsPathSeparator(i));
      }
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "FindPreviousSeparator")
  {
    const char* szPath = "This/Is\\My//Path.dot\\file.extension";

    EZ_TEST_BOOL(ezPathUtils::FindPreviousSeparator(szPath, szPath + 35) == szPath + 20);
    EZ_TEST_BOOL(ezPathUtils::FindPreviousSeparator(szPath, szPath + 20) == szPath + 11);
    EZ_TEST_BOOL(ezPathUtils::FindPreviousSeparator(szPath, szPath + 11) == szPath + 10);
    EZ_TEST_BOOL(ezPathUtils::FindPreviousSeparator(szPath, szPath + 10) == szPath + 7);
    EZ_TEST_BOOL(ezPathUtils::FindPreviousSeparator(szPath, szPath + 7)  == szPath + 4);
    EZ_TEST_BOOL(ezPathUtils::FindPreviousSeparator(szPath, szPath + 4)  == nullptr);
    EZ_TEST_BOOL(ezPathUtils::FindPreviousSeparator(szPath, szPath)  == nullptr);
    EZ_TEST_BOOL(ezPathUtils::FindPreviousSeparator(nullptr, nullptr)  == nullptr);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "HasAnyExtension")
  {
    EZ_TEST_BOOL(ezPathUtils::HasAnyExtension("This/Is\\My//Path.dot\\file.extension"));
    EZ_TEST_BOOL(!ezPathUtils::HasAnyExtension("This/Is\\My//Path.dot\\file_no_extension"));
    EZ_TEST_BOOL(!ezPathUtils::HasAnyExtension(""));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "HasExtension")
  {
    EZ_TEST_BOOL(ezPathUtils::HasExtension("This/Is\\My//Path.dot\\file.extension", ".Extension"));
    EZ_TEST_BOOL(ezPathUtils::HasExtension("This/Is\\My//Path.dot\\file.ext", "EXT"));
    EZ_TEST_BOOL(!ezPathUtils::HasExtension("This/Is\\My//Path.dot\\file.ext", "NEXT"));
    EZ_TEST_BOOL(!ezPathUtils::HasExtension("This/Is\\My//Path.dot\\file.extension", ".Ext"));
    EZ_TEST_BOOL(!ezPathUtils::HasExtension("This/Is\\My//Path.dot\\file.extension", "sion"));
    EZ_TEST_BOOL(!ezPathUtils::HasExtension("", "ext"));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetFileExtension")
  {
    EZ_TEST_BOOL(ezPathUtils::GetFileExtension("This/Is\\My//Path.dot\\file.extension") == "extension");
    EZ_TEST_BOOL(ezPathUtils::GetFileExtension("This/Is\\My//Path.dot\\file") == "");
    EZ_TEST_BOOL(ezPathUtils::GetFileExtension("") == "");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetFileNameAndExtension")
  {
    EZ_TEST_BOOL(ezPathUtils::GetFileNameAndExtension("This/Is\\My//Path.dot\\file.extension") == "file.extension");
    EZ_TEST_BOOL(ezPathUtils::GetFileNameAndExtension("This/Is\\My//Path.dot\\.extension") == ".extension");
    EZ_TEST_BOOL(ezPathUtils::GetFileNameAndExtension("This/Is\\My//Path.dot\\file") == "file");
    EZ_TEST_BOOL(ezPathUtils::GetFileNameAndExtension("\\file") == "file");
    EZ_TEST_BOOL(ezPathUtils::GetFileNameAndExtension("") == "");
    EZ_TEST_BOOL(ezPathUtils::GetFileNameAndExtension("/") == "");
    EZ_TEST_BOOL(ezPathUtils::GetFileNameAndExtension("This/Is\\My//Path.dot\\") == "");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetFileName")
  {
    EZ_TEST_BOOL(ezPathUtils::GetFileName("This/Is\\My//Path.dot\\file.extension") == "file");
    EZ_TEST_BOOL(ezPathUtils::GetFileName("This/Is\\My//Path.dot\\file") == "file");
    EZ_TEST_BOOL(ezPathUtils::GetFileName("\\file") == "file");
    EZ_TEST_BOOL(ezPathUtils::GetFileName("") == "");
    EZ_TEST_BOOL(ezPathUtils::GetFileName("/") == "");
    EZ_TEST_BOOL(ezPathUtils::GetFileName("This/Is\\My//Path.dot\\") == "");

    // so far we treat file and folders whose names start with a '.' as extensions
    EZ_TEST_BOOL(ezPathUtils::GetFileName("This/Is\\My//Path.dot\\.stupidfile") == "");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetFileDirectory")
  {
    EZ_TEST_BOOL(ezPathUtils::GetFileDirectory("This/Is\\My//Path.dot\\file.extension") == "This/Is\\My//Path.dot\\");
    EZ_TEST_BOOL(ezPathUtils::GetFileDirectory("This/Is\\My//Path.dot\\.extension") == "This/Is\\My//Path.dot\\");
    EZ_TEST_BOOL(ezPathUtils::GetFileDirectory("This/Is\\My//Path.dot\\file") == "This/Is\\My//Path.dot\\");
    EZ_TEST_BOOL(ezPathUtils::GetFileDirectory("\\file") == "\\");
    EZ_TEST_BOOL(ezPathUtils::GetFileDirectory("") == "");
    EZ_TEST_BOOL(ezPathUtils::GetFileDirectory("/") == "/");
    EZ_TEST_BOOL(ezPathUtils::GetFileDirectory("This/Is\\My//Path.dot\\") == "This/Is\\My//Path.dot\\");
    EZ_TEST_BOOL(ezPathUtils::GetFileDirectory("This") == "");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsAbsolutePath")
  {
    #if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
      EZ_TEST_BOOL(ezPathUtils::IsAbsolutePath("C:\\temp.stuff"));
      EZ_TEST_BOOL(ezPathUtils::IsAbsolutePath("C:/temp.stuff"));
      EZ_TEST_BOOL(ezPathUtils::IsAbsolutePath("\\\\myserver\\temp.stuff"));
      EZ_TEST_BOOL(!ezPathUtils::IsAbsolutePath("\\myserver\\temp.stuff"));
      EZ_TEST_BOOL(!ezPathUtils::IsAbsolutePath("temp.stuff"));
      EZ_TEST_BOOL(!ezPathUtils::IsAbsolutePath("/temp.stuff"));
      EZ_TEST_BOOL(!ezPathUtils::IsAbsolutePath("\\temp.stuff"));
      EZ_TEST_BOOL(!ezPathUtils::IsAbsolutePath("..\\temp.stuff"));
      EZ_TEST_BOOL(!ezPathUtils::IsAbsolutePath(".\\temp.stuff"));
    #elif EZ_ENABLED(EZ_PLATFORM_OSX) || EZ_ENABLED(EZ_PLATFORM_LINUX)
      EZ_TEST_BOOL(ezPathUtils::IsAbsolutePath("/usr/local/.stuff"));
      EZ_TEST_BOOL(ezPathUtils::IsAbsolutePath("/file.test"));
      EZ_TEST_BOOL(!ezPathUtils::IsAbsolutePath("./file.stuff"));
      EZ_TEST_BOOL(!ezPathUtils::IsAbsolutePath("file.stuff"));
    #else
      #error Unknown platform.
    #endif
  }
}

