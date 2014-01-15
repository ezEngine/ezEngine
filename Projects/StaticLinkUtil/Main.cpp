#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Strings/PathUtils.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Logging/HTMLWriter.h>
#include <Core/Application/Application.h>

/* When statically linking libraries into an application the linker will only pull in all the functions and variables that are inside
   translation units (CPP files) that somehow get referenced.

   In ez a lot of stuff happens automatically (e.g. types register themselves etc.), which is accomplished through global variables
   that execute code in their constructor during the applications startup phase. This only works when those global variables are actually
   put into the application by the linker. If the linker does not do that, functionality will not work as intended.

   Contrary to common conception, the linker is NOT ALLOWED to optimize away global variables. The only reason for not including a global
   variable into the final binary, is when the entire translation unit where a variable is defined in, is never referenced and thus never
   even looked at by the linker.

   To fix this, this tool inserts macros into each and every file which reference each other. Afterwards every file in a library will
   have reference every other file in that same library and thus once a library is used in any way in some program, the entire library
   will be pulled in and will then work as intended.

   These references are accomplished through empty functions that are called in one central location (where EZ_STATICLINK_LIBRARY is defined),
   though the code actually never really calls those functions, but it is enough to force the linker to look at all the other files.

   Usage of this tool:

   Call this tool with the path to the root folder of some library as the sole command line argument:

   StaticLinkUtil.exe "C:\ezEngine\Trunk\Code\Engine\Foundation"

   This will iterate over all files below that folder and insert the proper macros.
   Also make sure that exactly one file in each library contains the text 'EZ_STATICLINK_LIBRARY();'

   The parameters and function body will be automatically generated and later updated, you do not need to provide more.
*/

class ezStaticLinkerApp : public ezApplication
{
private:
  const char* m_szSearchDir;

public:
  ezStaticLinkerApp()
  {
    m_szSearchDir = "";
  }

  virtual void AfterEngineInit() EZ_OVERRIDE
  {
    EZ_ASSERT_ALWAYS(GetArgumentCount() == 2, "This tool requires exactly one command-line argument: An absolute path to the top-level folder of a library.");

    // pass the absolute path to the directory that should be scanned as the first parameter to this application
    m_szSearchDir = GetArgument(1);

    EZ_ASSERT_ALWAYS(ezPathUtils::IsAbsolutePath(m_szSearchDir), "The given path is not absolute: '%s'", m_szSearchDir);

    // Add standard folder factory
    ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);

    // Add the empty data directory to access files via absolute paths
    ezFileSystem::AddDataDirectory("");

    ezGlobalLog::AddLogWriter(ezLogWriter::Console::LogMessageHandler);
    ezGlobalLog::AddLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);
  }

  virtual void BeforeEngineShutdown()
  {
    ezGlobalLog::RemoveLogWriter(ezLogWriter::Console::LogMessageHandler);
    ezGlobalLog::RemoveLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);
  }

  ezString GetLibraryMarkerName()
  {
    return ezPathUtils::GetFileName(m_szSearchDir).GetData();
  }

  ezSet<ezHybridString<32, ezStaticAllocatorWrapper>, ezCompareHelper<ezHybridString<32, ezStaticAllocatorWrapper>>, ezStaticAllocatorWrapper> m_AllRefPoints;
  ezHybridString<32, ezStaticAllocatorWrapper> m_sRefPointGroupFile;

  ezResult ReadEntireFile(const char* szFile, ezStringBuilder& sOut)
  {
    sOut.Clear();

    ezFileReader File;
    if (File.Open(szFile) == EZ_FAILURE)
    {
      ezLog::Error("Could not open the file for reading.");
      return EZ_FAILURE;
    }

    ezDynamicArray<ezUInt8> FileContent;

    ezUInt8 Temp[1024];
    ezUInt64 uiRead = File.ReadBytes(Temp, 1024);

    while (uiRead > 0)
    {
      FileContent.PushBackRange(ezArrayPtr<ezUInt8>(Temp, (ezUInt32) uiRead));

      uiRead = File.ReadBytes(Temp, 1024);
    }

    if (FileContent.IsEmpty())
    {
      ezLog::Dev("File is empty.");
      return EZ_SUCCESS;
    }

    FileContent.PushBack(0);

    if (!ezUnicodeUtils::IsValidUtf8((const char*) &FileContent[0]))
    {
      ezLog::Error("The file \"%s\" contains characters that are not valid Utf8. This often happens when you type special characters in an editor that does not save the file in Utf8 encoding.");
      return EZ_FAILURE;
    }

    sOut = (const char*) &FileContent[0];
    sOut.ReplaceAll("\r", "");

    if (!sOut.EndsWith("\n"))
      sOut.Append("\n");

    if (!sOut.EndsWith("\n\n"))
      sOut.Append("\n");

    return EZ_SUCCESS;
  }

  ezResult OverwriteFile(const char* szFile, const ezStringBuilder& sFileContent)
  {
    ezFileWriter FileOut;
    if (FileOut.Open(szFile) == EZ_FAILURE)
    {
      ezLog::Error("Could not open the file for writing.");
      return EZ_FAILURE;
    }

    FileOut.WriteBytes(sFileContent.GetData(), sFileContent.GetElementCount());

    return EZ_SUCCESS;
  }

  // I want better allocators!
  ezSet<ezHybridString<32, ezStaticAllocatorWrapper>, ezCompareHelper<ezHybridString<32, ezStaticAllocatorWrapper>>, ezStaticAllocatorWrapper> m_GlobalIncludes;

  void FindIncludes(ezStringBuilder& sFileContent)
  {
    const char* szStartPos = sFileContent.GetData();
    const ezString sLibraryName = GetLibraryMarkerName();

    while (true)
    {
    const char* szI = sFileContent.FindSubString("#i", szStartPos);

    if (szI == NULL)
      return;

    szStartPos = szI + 1;

      if (ezStringUtils::IsEqualN(szI, "#if", 3))
      {
        szStartPos = sFileContent.FindSubString("#endif", szStartPos);

        if (szStartPos == NULL)
          return;

        ++szStartPos;
        continue; // next search will be for #i again
      }

      if (ezStringUtils::IsEqualN(szI, "#include", 8))
      {
        szI += 8; // skip the "#include" string

        const char* szLineEnd = ezStringUtils::FindSubString(szI, "\n");

        ezStringIterator si(szI, szLineEnd, szI);

        ezStringBuilder sInclude = si;

        if (sInclude.ReplaceAll("\\", "/") > 0)
        {
          ezLog::Info("Replacing backslashes in #include path with front slashes: '%s'", sInclude.GetData());
          sFileContent.ReplaceSubString(szI, szLineEnd, sInclude.GetData());
        }

        while (sInclude.StartsWith(" ") || sInclude.StartsWith("\t") || sInclude.StartsWith("<"))
          sInclude.Shrink(1, 0);

        while (sInclude.EndsWith(" ") || sInclude.EndsWith("\t") || sInclude.EndsWith(">"))
          sInclude.Shrink(0, 1);

        // ignore relative includes, they will not work as expected from the PCH
        if (sInclude.StartsWith("\""))
          continue;

        // ignore includes into the own library
        if (sInclude.StartsWith(sLibraryName.GetData()))
          continue;

        ezLog::Dev("Found Include: '%s'", sInclude.GetData());

        m_GlobalIncludes.Insert(sInclude);
      }
    }
  }

  bool ReplaceLine(ezStringBuilder& sFile, const char* szLineStart)
  {
    const char* szStart = sFile.FindSubString(szLineStart);

    if (szStart == NULL)
      return false;

    const char* szEnd = sFile.FindSubString("\n", szStart);

    if (szEnd == NULL)
      szEnd = sFile.GetData() + sFile.GetElementCount();

    sFile.ReplaceSubString(szStart, szEnd, "");

    return true;
  }

  void RewritePrecompiledHeaderIncludes()
  {
    ezStringBuilder sPCHFile = m_szSearchDir;
    sPCHFile.AppendPath("PCH.h");

    {
      ezFileReader File;
      if (File.Open(sPCHFile.GetData()) == EZ_FAILURE)
      {
        ezLog::Warning("This project has no PCH file.");
        return;
      }
    }

    ezLog::Info("Rewriting PCH: '%s'", sPCHFile.GetData());

    ezStringBuilder sFileContent;
    if (ReadEntireFile(sPCHFile.GetData(), sFileContent) == EZ_FAILURE)
      return;

    while (ReplaceLine(sFileContent, "#include"))
    {
      // do this
    }

    ezStringBuilder sAllIncludes;

    for (auto it = m_GlobalIncludes.GetIterator(); it.IsValid(); ++it)
    {
      sAllIncludes.AppendFormat("#include <%s>\n", it.Key().GetData());
    }

    sAllIncludes.ReplaceAll("\\", "/");

    sFileContent.Prepend(sAllIncludes.GetData());

    while (sFileContent.EndsWith("\n\n\n\n\n"))
      sFileContent.Shrink(0, 1);

    OverwriteFile(sPCHFile.GetData(), sFileContent);
  }

  void FixFileContents(const char* szFile)
  {
    ezStringBuilder sFileContent;
    if (ReadEntireFile(szFile, sFileContent) == EZ_FAILURE)
      return;

    if (ezStringUtils::EndsWith(szFile, "/PCH.h"))
      ezLog::Dev("Skipping PCH for #include search: '%s'", szFile);
    else
      FindIncludes(sFileContent);

    // rewrite the entire file
    OverwriteFile(szFile, sFileContent);
  }

  ezString GetFileMarkerName(const char* szFile)
  {
    ezStringBuilder sRel = szFile;
    sRel.MakeRelativePath(m_szSearchDir);
    
    ezStringBuilder sRefPointName = ezPathUtils::GetFileName(m_szSearchDir);
    sRefPointName.Append("_");
    sRefPointName.Append(sRel.GetData());
    sRefPointName.ReplaceAll("\\", "_");
    sRefPointName.ReplaceAll("/", "_");
    sRefPointName.ReplaceAll(".cpp", "");

    return sRefPointName;
  }

  void InsertRefPoint(const char* szFile)
  {
    EZ_LOG_BLOCK("InsertRefPoint", szFile);

    ezStringBuilder sFileContent;
    if (ReadEntireFile(szFile, sFileContent) == EZ_FAILURE)
      return;

    // if we find this macro in here, we don't need to insert EZ_STATICLINK_FILE in this file
    // but once we are done with all files, we want to come back to this file and rewrite the EZ_STATICLINK_LIBRARY
    // part such that it will reference all the other files
    if (sFileContent.FindSubString("EZ_STATICLINK_LIBRARY"))
    {
      ezLog::Info("Found macro 'EZ_STATICLINK_LIBRARY' in file '%s'.", &szFile[ezStringUtils::GetStringElementCount(m_szSearchDir) + 1]);

      if (!m_sRefPointGroupFile.IsEmpty())
        ezLog::Error("The macro 'EZ_STATICLINK_LIBRARY' was already found in file '%s' before. You cannot have this macro twice in the same library!", m_sRefPointGroupFile.GetData());
      else
        m_sRefPointGroupFile = szFile;

      return;
    }

    ezString sLibraryMarker = GetLibraryMarkerName();
    ezString sFileMarker = GetFileMarkerName(szFile);

    ezStringBuilder sNewMarker;
    sNewMarker.Format("EZ_STATICLINK_FILE(%s, %s);", sLibraryMarker.GetData(), sFileMarker.GetData());

    m_AllRefPoints.Insert(sFileMarker.GetData());

    const char* szMarker = sFileContent.FindSubString("EZ_STATICLINK_FILE");

    // if the marker already exists, replace it with the updated string
    if (szMarker != NULL)
    {
      const char* szMarkerEnd = szMarker;

      while (*szMarkerEnd != '\0' && *szMarkerEnd != '\n')
        ++szMarkerEnd;

      sFileContent.ReplaceSubString(szMarker, szMarkerEnd, sNewMarker.GetData());
    }
    else
    {
      // otherwise insert it at the end of the file
      sFileContent.AppendFormat("\n\n%s\n\n", sNewMarker.GetData());
    }

    // rewrite the entire file
    OverwriteFile(szFile, sFileContent);
  }

  void RewriteRefPointGroup(const char* szFile)
  {
    EZ_LOG_BLOCK("RewriteRefPointGroup", szFile);

    ezLog::Info("Replacing macro EZ_STATICLINK_LIBRARY in file '%s'.", m_sRefPointGroupFile.GetData());

    ezStringBuilder sFileContent;
    if (ReadEntireFile(szFile, sFileContent) == EZ_FAILURE)
      return;

    // remove all instances of EZ_STATICLINK_FILE from this file, it already contains EZ_STATICLINK_LIBRARY
    const char* szMarker = sFileContent.FindSubString("EZ_STATICLINK_FILE");
    while (szMarker != NULL)
    {
      ezLog::Info("Found macro EZ_STATICLINK_FILE inside the same file where EZ_STATICLINK_LIBRARY is located. Removing it.");

      const char* szMarkerEnd = szMarker;

      while (*szMarkerEnd != '\0' && *szMarkerEnd != '\n')
        ++szMarkerEnd;

      // no ref point allowed in a file that has already a ref point group
      sFileContent.Remove(szMarker, szMarkerEnd);

      szMarker = sFileContent.FindSubString("EZ_STATICLINK_FILE");
    }

    ezStringBuilder sNewGroupMarker;

    // generate the code that should be inserted into this file
    // this code will reference all the other files in the library
    {
      sNewGroupMarker.Format("EZ_STATICLINK_LIBRARY(%s)\n{\n  if(bReturn)\n    return;\n\n", GetLibraryMarkerName().GetData());

      auto it = m_AllRefPoints.GetIterator();

      while (it.IsValid())
      {
        sNewGroupMarker.AppendFormat("  EZ_STATICLINK_REFERENCE(%s);\n", it.Key().GetData());
        ++it;
      }

      sNewGroupMarker.Append("}\n");
    }

    const char* szGroupMarker = sFileContent.FindSubString("EZ_STATICLINK_LIBRARY");

    if (szGroupMarker != NULL)
    {
      // if we could find the macro EZ_STATICLINK_LIBRARY, just replace it with the new code

      const char* szMarkerEnd = szGroupMarker;

      bool bFoundOpenBraces = false;

      while (*szMarkerEnd != '\0' && *szMarkerEnd != '}')
      {
        ++szMarkerEnd;

        if (*szMarkerEnd == '{')
          bFoundOpenBraces = true;
        if (!bFoundOpenBraces && *szMarkerEnd == ';')
          break;
      }

      if (*szMarkerEnd == '}' || *szMarkerEnd == ';')
        ++szMarkerEnd;
      if (*szMarkerEnd == '\n')
        ++szMarkerEnd;

      // now replace the existing EZ_STATICLINK_LIBRARY and its code block with the new block
      sFileContent.ReplaceSubString(szGroupMarker, szMarkerEnd, sNewGroupMarker.GetData());
    }
    else
    {
      // if we can't find the macro, append it to the end of the file
      // this can only happen, if we ever extend this tool such that it picks one file to auto-insert this macro
      sFileContent.AppendFormat("\n\n%s\n\n", sNewGroupMarker.GetData());
    }

    OverwriteFile(szFile, sFileContent);
  }


  virtual ezApplication::ApplicationExecution Run() EZ_OVERRIDE
  {
    const ezUInt32 uiSearchDirLength = ezStringUtils::GetStringElementCount(m_szSearchDir) + 1;

    // get a directory iterator for the search directory
    ezFileSystemIterator it;
    if (it.StartSearch(m_szSearchDir, true, false) == EZ_SUCCESS)
    {
      ezStringBuilder b, sExt;

      // while there are additional files / folders
      do
      {
        // build the absolute path to the current file
        b = it.GetCurrentPath();
        b.AppendPath(it.GetStats().m_sFileName.GetData());

        // file extensions are always converted to lower-case actually
        sExt = b.GetFileExtension();

        if (sExt.IsEqual_NoCase("h") || sExt.IsEqual_NoCase("inl"))
        {
          EZ_LOG_BLOCK("Header", &b.GetData()[uiSearchDirLength]);
          ezLog::Info("Header: '%s'", &b.GetData()[uiSearchDirLength]);
          FixFileContents(b.GetData());
          continue;
        }

        if (sExt.IsEqual_NoCase("cpp"))
        {
          EZ_LOG_BLOCK("Source", &b.GetData()[uiSearchDirLength]);
          ezLog::Info("CPP: '%s'", &b.GetData()[uiSearchDirLength]);
          FixFileContents(b.GetData());

          InsertRefPoint(b.GetData());
          continue;
        }
      }
      while (it.Next() == EZ_SUCCESS);
    }
    else
      ezLog::Error("Could not search the directory '%s'", m_szSearchDir);

    if (!m_sRefPointGroupFile.IsEmpty())
      RewriteRefPointGroup(m_sRefPointGroupFile.GetData());
    else
      ezLog::Error("The macro EZ_STATICLINK_LIBRARY was not found in any cpp file in this library. It is required that it exists in exactly one file, otherwise the generated code will not compile.");

    RewritePrecompiledHeaderIncludes();

    return ezApplication::Quit;
  }
};

EZ_CONSOLEAPP_ENTRY_POINT(ezStaticLinkerApp);
