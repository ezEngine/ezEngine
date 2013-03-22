#include <PCH.h>

EZ_CREATE_SIMPLE_TEST(Strings, PathUtils)
{
  EZ_TEST_BLOCK(true, "IsPathSeparator")
  {
    for (int i = 0; i < 0xFFFF; ++i)
    {
      if (i == '/')
      {
        EZ_TEST(ezPathUtils::IsPathSeparator(i));
      }
      else
      if (i == '\\')
      {
        EZ_TEST(ezPathUtils::IsPathSeparator(i));
      }
      else
      {
        EZ_TEST(!ezPathUtils::IsPathSeparator(i));
      }
    }
  }

  EZ_TEST_BLOCK(true, "FindPreviousSeparator")
  {
    const char* szPath = "This/Is\\My//Path.dot\\file.extension";

    EZ_TEST(ezPathUtils::FindPreviousSeparator(szPath, szPath + 35) == szPath + 20);
    EZ_TEST(ezPathUtils::FindPreviousSeparator(szPath, szPath + 20) == szPath + 11);
    EZ_TEST(ezPathUtils::FindPreviousSeparator(szPath, szPath + 11) == szPath + 10);
    EZ_TEST(ezPathUtils::FindPreviousSeparator(szPath, szPath + 10) == szPath + 7);
    EZ_TEST(ezPathUtils::FindPreviousSeparator(szPath, szPath + 7)  == szPath + 4);
    EZ_TEST(ezPathUtils::FindPreviousSeparator(szPath, szPath + 4)  == NULL);
    EZ_TEST(ezPathUtils::FindPreviousSeparator(szPath, szPath)  == NULL);
    EZ_TEST(ezPathUtils::FindPreviousSeparator(NULL, NULL)  == NULL);
  }

  EZ_TEST_BLOCK(true, "HasAnyExtension")
  {
    EZ_TEST(ezPathUtils::HasAnyExtension("This/Is\\My//Path.dot\\file.extension"));
    EZ_TEST(!ezPathUtils::HasAnyExtension("This/Is\\My//Path.dot\\file_no_extension"));
    EZ_TEST(!ezPathUtils::HasAnyExtension(""));
  }

  EZ_TEST_BLOCK(true, "HasExtension")
  {
    EZ_TEST(ezPathUtils::HasExtension("This/Is\\My//Path.dot\\file.extension", ".Extension"));
    EZ_TEST(ezPathUtils::HasExtension("This/Is\\My//Path.dot\\file.ext", "EXT"));
    EZ_TEST(!ezPathUtils::HasExtension("This/Is\\My//Path.dot\\file.ext", "NEXT"));
    EZ_TEST(!ezPathUtils::HasExtension("This/Is\\My//Path.dot\\file.extension", ".Ext"));
    EZ_TEST(!ezPathUtils::HasExtension("This/Is\\My//Path.dot\\file.extension", "sion"));
    EZ_TEST(!ezPathUtils::HasExtension("", "ext"));
  }

  EZ_TEST_BLOCK(true, "GetFileExtension")
  {
    EZ_TEST(ezPathUtils::GetFileExtension("This/Is\\My//Path.dot\\file.extension") == "extension");
    EZ_TEST(ezPathUtils::GetFileExtension("This/Is\\My//Path.dot\\file") == "");
    EZ_TEST(ezPathUtils::GetFileExtension("") == "");
  }

  EZ_TEST_BLOCK(true, "GetFileNameAndExtension")
  {
    EZ_TEST(ezPathUtils::GetFileNameAndExtension("This/Is\\My//Path.dot\\file.extension") == "file.extension");
    EZ_TEST(ezPathUtils::GetFileNameAndExtension("This/Is\\My//Path.dot\\.extension") == ".extension");
    EZ_TEST(ezPathUtils::GetFileNameAndExtension("This/Is\\My//Path.dot\\file") == "file");
    EZ_TEST(ezPathUtils::GetFileNameAndExtension("\\file") == "file");
    EZ_TEST(ezPathUtils::GetFileNameAndExtension("") == "");
    EZ_TEST(ezPathUtils::GetFileNameAndExtension("/") == "");
    EZ_TEST(ezPathUtils::GetFileNameAndExtension("This/Is\\My//Path.dot\\") == "");
  }

  EZ_TEST_BLOCK(true, "GetFileName")
  {
    EZ_TEST(ezPathUtils::GetFileName("This/Is\\My//Path.dot\\file.extension") == "file");
    EZ_TEST(ezPathUtils::GetFileName("This/Is\\My//Path.dot\\file") == "file");
    EZ_TEST(ezPathUtils::GetFileName("\\file") == "file");
    EZ_TEST(ezPathUtils::GetFileName("") == "");
    EZ_TEST(ezPathUtils::GetFileName("/") == "");
    EZ_TEST(ezPathUtils::GetFileName("This/Is\\My//Path.dot\\") == "");

    // so far we treat file and folders whose names start with a '.' as extensions
    EZ_TEST(ezPathUtils::GetFileName("This/Is\\My//Path.dot\\.stupidfile") == "");
  }

  EZ_TEST_BLOCK(true, "GetFileDirectory")
  {
    EZ_TEST(ezPathUtils::GetFileDirectory("This/Is\\My//Path.dot\\file.extension") == "This/Is\\My//Path.dot\\");
    EZ_TEST(ezPathUtils::GetFileDirectory("This/Is\\My//Path.dot\\.extension") == "This/Is\\My//Path.dot\\");
    EZ_TEST(ezPathUtils::GetFileDirectory("This/Is\\My//Path.dot\\file") == "This/Is\\My//Path.dot\\");
    EZ_TEST(ezPathUtils::GetFileDirectory("\\file") == "\\");
    EZ_TEST(ezPathUtils::GetFileDirectory("") == "");
    EZ_TEST(ezPathUtils::GetFileDirectory("/") == "/");
    EZ_TEST(ezPathUtils::GetFileDirectory("This/Is\\My//Path.dot\\") == "This/Is\\My//Path.dot\\");
    EZ_TEST(ezPathUtils::GetFileDirectory("This") == "");
  }

  EZ_TEST_BLOCK(true, "IsAbsolutePath")
  {
    #if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
      EZ_TEST(ezPathUtils::IsAbsolutePath("C:\\test.stuff"));
      EZ_TEST(ezPathUtils::IsAbsolutePath("C:/test.stuff"));
      EZ_TEST(ezPathUtils::IsAbsolutePath("\\\\myserver\\test.stuff"));
      EZ_TEST(!ezPathUtils::IsAbsolutePath("\\myserver\\test.stuff"));
      EZ_TEST(!ezPathUtils::IsAbsolutePath("test.stuff"));
      EZ_TEST(!ezPathUtils::IsAbsolutePath("/test.stuff"));
      EZ_TEST(!ezPathUtils::IsAbsolutePath("\\test.stuff"));
      EZ_TEST(!ezPathUtils::IsAbsolutePath("..\\test.stuff"));
      EZ_TEST(!ezPathUtils::IsAbsolutePath(".\\test.stuff"));
    #elif EZ_ENABLED(EZ_PLATFORM_OSX)
      EZ_TEST(ezPathUtils::IsAbsolutePath("/usr/local/.stuff"));
      EZ_TEST(ezPathUtils::IsAbsolutePath("/file.test"));
      EZ_TEST(!ezPathUtils::IsAbsolutePath("./file.stuff"));
      EZ_TEST(!ezPathUtils::IsAbsolutePath("file.stuff"));
    #else
      #error Unknown platform.
    #endif
  }
}

