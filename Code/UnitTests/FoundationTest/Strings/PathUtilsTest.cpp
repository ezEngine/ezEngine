#include <FoundationTest/FoundationTestPCH.h>

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
      else if (i == '\\')
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
    EZ_TEST_BOOL(ezPathUtils::FindPreviousSeparator(szPath, szPath + 7) == szPath + 4);
    EZ_TEST_BOOL(ezPathUtils::FindPreviousSeparator(szPath, szPath + 4) == nullptr);
    EZ_TEST_BOOL(ezPathUtils::FindPreviousSeparator(szPath, szPath) == nullptr);
    EZ_TEST_BOOL(ezPathUtils::FindPreviousSeparator(nullptr, nullptr) == nullptr);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetFileExtension")
  {
    EZ_TEST_BOOL(ezPathUtils::GetFileExtension("This/Is\\My//Path.dot\\file.extension") == "extension");
    EZ_TEST_BOOL(ezPathUtils::GetFileExtension("This/Is\\My//Path.dot\\file") == "");
    EZ_TEST_BOOL(ezPathUtils::GetFileExtension("") == "");

    EZ_TEST_BOOL(ezPathUtils::GetFileExtension("/foo/bar.txt") == "txt");
    EZ_TEST_BOOL(ezPathUtils::GetFileExtension("/foo/bar.") == "");
    EZ_TEST_BOOL(ezPathUtils::GetFileExtension("/foo/bar") == "");
    EZ_TEST_BOOL(ezPathUtils::GetFileExtension("/foo/bar.txt/bar.cc") == "cc");
    EZ_TEST_BOOL(ezPathUtils::GetFileExtension("/foo/bar.txt/bar.") == "");
    EZ_TEST_BOOL(ezPathUtils::GetFileExtension("/foo/bar.txt/bar") == "");
    EZ_TEST_BOOL(ezPathUtils::GetFileExtension("/foo/.") == "");
    EZ_TEST_BOOL(ezPathUtils::GetFileExtension("/foo/..") == "");
    EZ_TEST_BOOL(ezPathUtils::GetFileExtension("/foo/.hidden") == "");
    EZ_TEST_BOOL(ezPathUtils::GetFileExtension("/foo/..bar") == "");

    EZ_TEST_BOOL(ezPathUtils::GetFileExtension("foo.bar.baz.tar") == "tar");
    EZ_TEST_BOOL(ezPathUtils::GetFileExtension("foo.bar.baz") == "baz");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "HasAnyExtension")
  {
    EZ_TEST_BOOL(ezPathUtils::HasAnyExtension("This/Is\\My//Path.dot\\file.extension"));
    EZ_TEST_BOOL(!ezPathUtils::HasAnyExtension("This/Is\\My//Path.dot\\file_no_extension"));
    EZ_TEST_BOOL(!ezPathUtils::HasAnyExtension(""));

    EZ_TEST_BOOL(ezPathUtils::HasAnyExtension("/foo/bar.txt"));
    EZ_TEST_BOOL(!ezPathUtils::HasAnyExtension("/foo/bar."));
    EZ_TEST_BOOL(!ezPathUtils::HasAnyExtension("/foo/bar"));
    EZ_TEST_BOOL(ezPathUtils::HasAnyExtension("/foo/bar.txt/bar.cc"));
    EZ_TEST_BOOL(!ezPathUtils::HasAnyExtension("/foo/bar.txt/bar."));
    EZ_TEST_BOOL(!ezPathUtils::HasAnyExtension("/foo/bar.txt/bar"));
    EZ_TEST_BOOL(!ezPathUtils::HasAnyExtension("."));
    EZ_TEST_BOOL(!ezPathUtils::HasAnyExtension(".."));
    EZ_TEST_BOOL(!ezPathUtils::HasAnyExtension("/foo/."));
    EZ_TEST_BOOL(!ezPathUtils::HasAnyExtension("/foo/.."));
    EZ_TEST_BOOL(!ezPathUtils::HasAnyExtension("/foo/.hidden"));
    EZ_TEST_BOOL(!ezPathUtils::HasAnyExtension("/foo/..bar"));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "HasExtension")
  {
    EZ_TEST_BOOL(ezPathUtils::HasExtension("This/Is\\My//Path.dot\\file.extension", ".Extension"));
    EZ_TEST_BOOL(ezPathUtils::HasExtension("This/Is\\My//Path.dot\\file.ext", "EXT"));
    EZ_TEST_BOOL(!ezPathUtils::HasExtension("This/Is\\My//Path.dot\\file.ext", "NEXT"));
    EZ_TEST_BOOL(!ezPathUtils::HasExtension("This/Is\\My//Path.dot\\file.extension", ".Ext"));
    EZ_TEST_BOOL(!ezPathUtils::HasExtension("This/Is\\My//Path.dot\\file.extension", "sion"));
    EZ_TEST_BOOL(!ezPathUtils::HasExtension("", "ext"));

    EZ_TEST_BOOL(ezPathUtils::HasExtension("/foo/bar.txt", "txt"));
    EZ_TEST_BOOL(ezPathUtils::HasExtension("/foo/bar.", ""));
    EZ_TEST_BOOL(ezPathUtils::HasExtension("/foo/bar", ""));
    EZ_TEST_BOOL(ezPathUtils::HasExtension("/foo/bar.txt/bar.cc", "cc"));
    EZ_TEST_BOOL(ezPathUtils::HasExtension("/foo/bar.txt/bar.", ""));
    EZ_TEST_BOOL(ezPathUtils::HasExtension("/foo/bar.txt/bar", ""));
    EZ_TEST_BOOL(ezPathUtils::HasExtension("/foo/.", ""));
    EZ_TEST_BOOL(ezPathUtils::HasExtension("/foo/..", ""));
    EZ_TEST_BOOL(ezPathUtils::HasExtension("/foo/.hidden", ""));
    EZ_TEST_BOOL(ezPathUtils::HasExtension("/foo/..bar", ""));
    EZ_TEST_BOOL(!ezPathUtils::HasExtension(".file", ".file"));
    EZ_TEST_BOOL(!ezPathUtils::HasExtension(".file", "file"));
    EZ_TEST_BOOL(!ezPathUtils::HasExtension("folder/.file", ".file"));
    EZ_TEST_BOOL(!ezPathUtils::HasExtension("folder/.file", "file"));

    EZ_TEST_BOOL(ezPathUtils::HasExtension("foo.bar.baz.tar", "tar"));
    EZ_TEST_BOOL(ezPathUtils::HasExtension("foo.bar.baz", "baz"));

    EZ_TEST_BOOL(ezPathUtils::HasExtension("file.txt", "txt"));
    EZ_TEST_BOOL(ezPathUtils::HasExtension("file.txt", ".txt"));
    EZ_TEST_BOOL(ezPathUtils::HasExtension("file.a.b", ".b"));
    EZ_TEST_BOOL(ezPathUtils::HasExtension("file.a.b", "a.b"));
    EZ_TEST_BOOL(ezPathUtils::HasExtension("file.a.b", ".a.b"));
    EZ_TEST_BOOL(!ezPathUtils::HasExtension("file.a.b", "file.a.b"));
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
    EZ_TEST_BOOL(ezPathUtils::GetFileNameAndExtension("file") == "file");
    EZ_TEST_BOOL(ezPathUtils::GetFileNameAndExtension("file.ext") == "file.ext");
    EZ_TEST_BOOL(ezPathUtils::GetFileNameAndExtension(".stupidfile") == ".stupidfile");
    EZ_TEST_BOOL(ezPathUtils::GetFileNameAndExtension("folder/.") == ".");
    EZ_TEST_BOOL(ezPathUtils::GetFileNameAndExtension("folder/..") == "..");
    EZ_TEST_BOOL(ezPathUtils::GetFileNameAndExtension(".") == ".");
    EZ_TEST_BOOL(ezPathUtils::GetFileNameAndExtension("..") == "..");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetFileName")
  {
    EZ_TEST_BOOL(ezPathUtils::GetFileName("This/Is\\My//Path.dot\\file.extension") == "file");
    EZ_TEST_BOOL(ezPathUtils::GetFileName("This/Is\\My//Path.dot\\file") == "file");
    EZ_TEST_BOOL(ezPathUtils::GetFileName("\\file") == "file");
    EZ_TEST_BOOL(ezPathUtils::GetFileName("") == "");
    EZ_TEST_BOOL(ezPathUtils::GetFileName("/") == "");
    EZ_TEST_BOOL(ezPathUtils::GetFileName("This/Is\\My//Path.dot\\") == "");

    EZ_TEST_BOOL(ezPathUtils::GetFileName("This/Is\\My//Path.dot\\.stupidfile") == ".stupidfile");
    EZ_TEST_BOOL(ezPathUtils::GetFileName(".stupidfile") == ".stupidfile");

    EZ_TEST_BOOL(ezPathUtils::GetFileName("File.ext") == "File");
    EZ_TEST_BOOL(ezPathUtils::GetFileName("File.") == "File.");
    EZ_TEST_BOOL(ezPathUtils::GetFileName("File.ext.") == "File.ext.");

    EZ_TEST_BOOL(ezPathUtils::GetFileName("folder/.") == ".");
    EZ_TEST_BOOL(ezPathUtils::GetFileName("folder/..") == "..");
    EZ_TEST_BOOL(ezPathUtils::GetFileName(".") == ".");
    EZ_TEST_BOOL(ezPathUtils::GetFileName("..") == "..");
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
#elif EZ_ENABLED(EZ_PLATFORM_OSX) || EZ_ENABLED(EZ_PLATFORM_LINUX) || EZ_ENABLED(EZ_PLATFORM_ANDROID) || EZ_ENABLED(EZ_PLATFORM_WEB)
    EZ_TEST_BOOL(ezPathUtils::IsAbsolutePath("/usr/local/.stuff"));
    EZ_TEST_BOOL(ezPathUtils::IsAbsolutePath("/file.test"));
    EZ_TEST_BOOL(!ezPathUtils::IsAbsolutePath("./file.stuff"));
    EZ_TEST_BOOL(!ezPathUtils::IsAbsolutePath("file.stuff"));
#else
#  error "Unknown platform."
#endif
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetRootedPathParts")
  {
    ezStringView root, relPath;
    ezPathUtils::GetRootedPathParts(":MyRoot\\folder\\file.txt", root, relPath);
    EZ_TEST_BOOL(ezPathUtils::GetRootedPathRootName(":MyRoot\\folder\\file.txt") == root);
    EZ_TEST_BOOL(root == "MyRoot");
    EZ_TEST_BOOL(relPath == "folder\\file.txt");

    ezPathUtils::GetRootedPathParts("folder\\file2.txt", root, relPath);
    EZ_TEST_BOOL(root.IsEmpty());
    EZ_TEST_BOOL(relPath == "folder\\file2.txt");

    ezPathUtils::GetRootedPathParts(":root", root, relPath);
    EZ_TEST_BOOL(root == "root");
    EZ_TEST_BOOL(relPath == "");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsSubPath")
  {
    EZ_TEST_BOOL(ezPathUtils::IsSubPath("C:/DataDir", "C:/DataDir/SomeFolder"));
    EZ_TEST_BOOL(ezPathUtils::IsSubPath("C:/DataDir/", "C:/DataDir/SomeFolder"));
    EZ_TEST_BOOL(ezPathUtils::IsSubPath("C:/DataDir", "C:/DataDir"));
    EZ_TEST_BOOL(ezPathUtils::IsSubPath("C:/DataDir", "C:/DataDir/"));
    EZ_TEST_BOOL(ezPathUtils::IsSubPath("C:/DataDir/", "C:/DataDir/"));
    EZ_TEST_BOOL(!ezPathUtils::IsSubPath("C:/DataDir", "C:/DataDir2"));

    EZ_TEST_BOOL(ezPathUtils::IsSubPath("C:\\DataDir", "C:/DataDir/SomeFolder"));
    EZ_TEST_BOOL(ezPathUtils::IsSubPath("C:\\DataDir", "C:/DataDir"));
    EZ_TEST_BOOL(ezPathUtils::IsSubPath("C:\\DataDir", "C:/DataDir/"));
    EZ_TEST_BOOL(ezPathUtils::IsSubPath("C:\\DataDir\\", "C:/DataDir/"));
    EZ_TEST_BOOL(!ezPathUtils::IsSubPath("C:\\DataDir", "C:/DataDir2"));

    EZ_TEST_BOOL(!ezPathUtils::IsSubPath("C:\\DataDiR", "C:/DataDir/SomeFolder"));
    EZ_TEST_BOOL(!ezPathUtils::IsSubPath("C:\\DataDiR", "C:/DataDir"));
    EZ_TEST_BOOL(!ezPathUtils::IsSubPath("C:\\DataDiR", "C:/DataDir/"));
    EZ_TEST_BOOL(!ezPathUtils::IsSubPath("C:\\DataDiR", "C:/DataDir2"));

    EZ_TEST_BOOL(!ezPathUtils::IsSubPath("C:/DataDir/SomeFolder", "C:/DataDir"));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsSubPath_NoCase")
  {
    EZ_TEST_BOOL(ezPathUtils::IsSubPath_NoCase("C:/DataDir", "C:/DataDir/SomeFolder"));
    EZ_TEST_BOOL(ezPathUtils::IsSubPath_NoCase("C:/DataDir/", "C:/DataDir/SomeFolder"));
    EZ_TEST_BOOL(ezPathUtils::IsSubPath_NoCase("C:/DataDir", "C:/DataDir"));
    EZ_TEST_BOOL(ezPathUtils::IsSubPath_NoCase("C:/DataDir/", "C:/DataDir"));
    EZ_TEST_BOOL(ezPathUtils::IsSubPath_NoCase("C:/DataDir", "C:/DataDir/"));
    EZ_TEST_BOOL(ezPathUtils::IsSubPath_NoCase("C:/DataDir/", "C:/DataDir/"));
    EZ_TEST_BOOL(!ezPathUtils::IsSubPath_NoCase("C:/DataDir", "C:/DataDir2"));

    EZ_TEST_BOOL(ezPathUtils::IsSubPath_NoCase("C:\\DataDir", "C:/DataDir/SomeFolder"));
    EZ_TEST_BOOL(ezPathUtils::IsSubPath_NoCase("C:\\DataDir", "C:/DataDir"));
    EZ_TEST_BOOL(ezPathUtils::IsSubPath_NoCase("C:\\DataDir\\", "C:/DataDir"));
    EZ_TEST_BOOL(ezPathUtils::IsSubPath_NoCase("C:\\DataDir", "C:/DataDir/"));
    EZ_TEST_BOOL(!ezPathUtils::IsSubPath_NoCase("C:\\DataDir", "C:/DataDir2"));

    EZ_TEST_BOOL(ezPathUtils::IsSubPath_NoCase("C:\\DataDiR", "C:/DataDir/SomeFolder"));
    EZ_TEST_BOOL(ezPathUtils::IsSubPath_NoCase("C:\\DataDiR", "C:/DataDir"));
    EZ_TEST_BOOL(ezPathUtils::IsSubPath_NoCase("C:\\DataDiR", "C:/DataDir/"));
    EZ_TEST_BOOL(!ezPathUtils::IsSubPath_NoCase("C:\\DataDiR", "C:/DataDir2"));

    EZ_TEST_BOOL(!ezPathUtils::IsSubPath_NoCase("C:/DataDir/SomeFolder", "C:/DataDir"));
  }
}
