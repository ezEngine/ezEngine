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
    // pass the absolute path to the directory that should be scanned as the first parameter to this application
    if (GetArgumentCount() >= 2)
      m_szSearchDir = GetArgument(1);

    // Since we want to read/write files through the filesystem, we need to set that up too
    // First add a Factory that can create data directory types that handle normal folders
    ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);

    // Then add a folder as a data directory (the previously registered Factory will take care of creating the proper handler)
    // As we only need access to files through global paths, we add the "empty data directory"
    // This data dir will manage all accesses through absolute paths, unless any other data directory can handle them
    // since we don't add any further data dirs, this is it
    ezFileSystem::AddDataDirectory("");

    // The console log writer will pass all log messages to the standard console window
    ezGlobalLog::AddLogWriter(ezLogWriter::Console::LogMessageHandler);
    // The Visual Studio log writer will pass all messages to the output window in VS
    ezGlobalLog::AddLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);
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
      ezLog::Dev("File is entirely empty.");
      return EZ_SUCCESS;
    }

    FileContent.PushBack(0);

    sOut = (const char*) &FileContent[0];
    sOut.ReplaceAll("\r", "");

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

  ezString GetLibraryMarkerName(const char* szFile)
  {
    return ezPathUtils::GetFileName(m_szSearchDir).GetData();
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

    if (sFileContent.FindSubString("EZ_STATICLINK_LIBRARY"))
    {
      m_sRefPointGroupFile = szFile;
      return;
    }

    ezString sLibraryMarker = GetLibraryMarkerName(szFile);
    ezString sFileMarker = GetFileMarkerName(szFile);

    ezStringBuilder sNewMarker;
    sNewMarker.Format("EZ_STATICLINK_FILE(%s, %s);", sLibraryMarker.GetData(), sFileMarker.GetData());

    m_AllRefPoints.Insert(sFileMarker.GetData());

    const char* szMarker = sFileContent.FindSubString("EZ_STATICLINK_FILE");
    if (szMarker != NULL)
    {
      const char* szMarkerEnd = szMarker;

      while (*szMarkerEnd != '\0' && *szMarkerEnd != '\n')
        ++szMarkerEnd;

      sFileContent.ReplaceSubString(szMarker, szMarkerEnd, sNewMarker.GetData());
    }
    else
    {
      sFileContent.AppendFormat("\n\n%s\n\n", sNewMarker.GetData());
    }

    OverwriteFile(szFile, sFileContent);
  }

  void RewriteRefPointGroup(const char* szFile)
  {
    EZ_LOG_BLOCK("RewriteRefPointGroup", szFile);

    ezStringBuilder sFileContent;
    if (ReadEntireFile(szFile, sFileContent) == EZ_FAILURE)
      return;

    const char* szMarker = sFileContent.FindWholeWord("EZ_STATICLINK_FILE", ezStringUtils::IsIdentifierDelimiter_C_Code);
    if (szMarker != NULL)
    {
      const char* szMarkerEnd = szMarker;

      while (*szMarkerEnd != '\0' && *szMarkerEnd != '\n')
        ++szMarkerEnd;

      // no ref point allowed in a file that has already a ref point group
      sFileContent.Remove(szMarker, szMarkerEnd);
    }

    ezStringBuilder sNewGroupMarker;
    sNewGroupMarker.Format("EZ_STATICLINK_LIBRARY(%s)\n{\nif(bReturn)\n  return;\n\n", GetLibraryMarkerName(szFile).GetData());

    {
      auto it = m_AllRefPoints.GetIterator();

      while (it.IsValid())
      {
        sNewGroupMarker.AppendFormat("  EZ_STATICLINK_REFERENCE(%s);\n", it.Key().GetData());
        ++it;
      }
    }

    sNewGroupMarker.Append("}\n");

    const char* szGroupMarker = sFileContent.FindSubString("EZ_STATICLINK_LIBRARY");

    if (szGroupMarker != NULL)
    {
      const char* szMarkerEnd = szGroupMarker;

      while (*szMarkerEnd != '\0' && *szMarkerEnd != '}')
        ++szMarkerEnd;

      if (*szMarkerEnd == '}')
        ++szMarkerEnd;
      if (*szMarkerEnd == '\n')
        ++szMarkerEnd;

      // no ref point allowed in a file that has already a ref point group
      sFileContent.ReplaceSubString(szGroupMarker, szMarkerEnd, sNewGroupMarker.GetData());
    }
    else
      sFileContent.AppendFormat("\n\n%s\n\n", sNewGroupMarker.GetData());

    OverwriteFile(szFile, sFileContent);
  }


  virtual ezApplication::ApplicationExecution Run() EZ_OVERRIDE
  {
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

        if (!sExt.IsEqual_NoCase("cpp"))
          continue;

        ezLog::Info("Found CPP File \"%s\"", b.GetData());

        InsertRefPoint(b.GetData());
      }
      while (it.Next() == EZ_SUCCESS);
    }
    else
      ezLog::Error("Could not search the directory '%s'", m_szSearchDir);

    if (!m_sRefPointGroupFile.IsEmpty())
    {
      ezLog::Info("Found RefPoint Group File: \"%s\"", m_sRefPointGroupFile.GetData());
      RewriteRefPointGroup(m_sRefPointGroupFile.GetData());
    }

    return ezApplication::Quit;
  }
};


EZ_CONSOLEAPP_ENTRY_POINT(ezStaticLinkerApp);
