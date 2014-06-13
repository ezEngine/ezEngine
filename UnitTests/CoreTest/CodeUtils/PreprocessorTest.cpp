#include <PCH.h>
#include <CoreUtils/CodeUtils/Preprocessor.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>


EZ_CREATE_SIMPLE_TEST_GROUP(CodeUtils);

static void Test(const char* szFile)
{

}

ezResult FileOpen(const char* szAbsoluteFile, ezDynamicArray<ezUInt8>& FileContent)
{
  ezFileReader r;
  if (r.Open(szAbsoluteFile).Failed())
    return EZ_FAILURE;

  ezUInt8 Temp[4096];

  while (ezUInt64 uiRead = r.ReadBytes(Temp, 4096))
  {
    FileContent.PushBackRange(ezArrayPtr<ezUInt8>(Temp, (ezUInt32) uiRead));
  }

  return EZ_SUCCESS;
}

ezResult FileLocator(const char* szCurAbsoluteFile, const char* szIncludeFile, ezPreprocessor::IncludeType IncType, ezString& out_sAbsoluteFilePath)
{
  ezStringBuilder s;

  if (IncType == ezPreprocessor::RelativeInclude)
  {
    s = szCurAbsoluteFile;
    s.PathParentDirectory();
    s.AppendPath(szIncludeFile);
    s.MakeCleanPath();
  }
  else if (IncType == ezPreprocessor::GlobalInclude)
  {
    s = "Preprocessor";
    s.AppendPath(szIncludeFile);
    s.MakeCleanPath();
  }
  else
    s = szIncludeFile;

  out_sAbsoluteFilePath = s;
  return EZ_SUCCESS;
}

class Logger : public ezLogInterface
{
public:
  virtual void HandleLogMessage(const ezLoggingEventData& le) override
  {
    m_sOutput.AppendFormat("Log: '%s'\r\n", le.m_szText);
  }

  void EventHandler(const ezPreprocessor::ProcessingEvent& ed)
  {
    switch (ed.m_Type)
    {
    case ezPreprocessor::ProcessingEvent::Error:
    case ezPreprocessor::ProcessingEvent::Warning:
      m_EventStack.PushBack(ed);
      break;
    case ezPreprocessor::ProcessingEvent::BeginExpansion:
      m_EventStack.PushBack(ed);
      return;
    case ezPreprocessor::ProcessingEvent::EndExpansion:
      m_EventStack.PopBack();
      return;
    }

    for (ezUInt32 i = 0; i < m_EventStack.GetCount(); ++i)
    {
      const ezPreprocessor::ProcessingEvent& event = m_EventStack[i];

      if (event.m_pToken != nullptr)
        m_sOutput.AppendFormat("%s: Line %u [%u]: ", event.m_pToken->m_File.GetString().GetData(), event.m_pToken->m_uiLine, event.m_pToken->m_uiColumn);

      switch (event.m_Type)
      {
      case ezPreprocessor::ProcessingEvent::Error:
        m_sOutput.Append("Error: ");
        break;
      case ezPreprocessor::ProcessingEvent::Warning:
        m_sOutput.Append("Warning: ");
        break;
      case ezPreprocessor::ProcessingEvent::BeginExpansion:
        m_sOutput.AppendFormat("In Macro: '%s'", ezString(event.m_pToken->m_DataView).GetData());
        break;
      case ezPreprocessor::ProcessingEvent::EndExpansion:
        break;
      }

      m_sOutput.AppendFormat("%s\r\n", event.m_szInfo);
    }
  }

  ezDeque<ezPreprocessor::ProcessingEvent> m_EventStack;
  ezStringBuilder m_sOutput;
};


EZ_CREATE_SIMPLE_TEST(CodeUtils, Preprocessor)
{
  ezStringBuilder sReadDir = BUILDSYSTEM_OUTPUT_FOLDER;
  sReadDir.AppendPath("../../Shared/UnitTests/CoreTest");

  ezStringBuilder sWriteDir = BUILDSYSTEM_OUTPUT_FOLDER;
  sWriteDir.AppendPath("CoreTest");

  EZ_TEST_BOOL(ezOSFile::CreateDirectoryStructure(sWriteDir.GetData()) == EZ_SUCCESS);

  ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);
  EZ_TEST_BOOL(ezFileSystem::AddDataDirectory(sReadDir.GetData(), ezFileSystem::ReadOnly, "PreprocessorTest") == EZ_SUCCESS);
  EZ_TEST_BOOL(ezFileSystem::AddDataDirectory(sWriteDir.GetData(), ezFileSystem::AllowWrites, "PreprocessorTest") == EZ_SUCCESS);

  ezTokenizedFileCache SharedCache;

  /* Stuff to test:
    #ifdef, #if etc.
      macro expansion
      mathematical expressions
    #error
    #warning
    __VA_ARGS__
    too few, too many parameters
    custom defines from outside
    expand to self (with, without parameters, with parameters to expand)
    expansion that needs several iterations
    stringify invalid token
    concatenate invalid tokens, tokens that yield valid macro
    broken function macros (missing parenthesis etc.)
    stringification of strings and special characters (\n)
    errors after #line directive
    errors in
      #line directive
      #define
      #ifdef
      etc.
    commas in macro parameters
    unlocateable include file
    pass through #pragma
    pass through #line
    invalid #if, #else, #elif, #endif nesting
    #undef


    Done:
    #pragma once
    #include "" and <>
    #define defined / __LINE__ / __FILE__
    #line, __LINE__, __FILE__
    stringification with comments, newlines and spaces
    concatenation (maybe more?)
    bad #include
    comments
    newlines in some weird places


  */
  
  {
    const char* szTestFiles[] =
    {
      "LineControl", /// \todo Newline after #include ?
      "DefineFile",
      "DefineLine",
      "DefineDefined",
      "Stringify",
      "BuildFlags",
      "Empty",
      "Test1", 
      "FailedInclude", /// \todo Better error message
      
    };

    ezStringBuilder sOutput;
    ezStringBuilder fileName;
    ezStringBuilder fileNameOut;
    ezStringBuilder fileNameExp;

    for (int i = 0; i < EZ_ARRAY_SIZE(szTestFiles); i++)
    {
      EZ_TEST_BLOCK(ezTestBlock::Enabled, szTestFiles[i])
      {
        Logger log;

        ezPreprocessor pp;
        pp.SetLogInterface(&log);
        pp.SetPassThroughLine(false);
        pp.SetPassThroughPragma(true);
        pp.SetFileCallbacks(FileOpen, FileLocator);
        pp.SetCustomFileCache(&SharedCache);
        pp.m_ProcessingEvents.AddEventHandler(ezDelegate<void (const ezPreprocessor::ProcessingEvent&)>(&Logger::EventHandler, &log));

        {
          fileName.Format("Preprocessor/%s.txt", szTestFiles[i]);
          fileNameExp.Format("Preprocessor/%s - Expected.txt", szTestFiles[i]);
          fileNameOut.Format("Preprocessor/%s - Result.txt", szTestFiles[i]);

          EZ_TEST_BOOL_MSG(ezFileSystem::ExistsFile(fileName.GetData()), "File does not exist: '%s'", fileName.GetData());

          ezFileWriter fout;
          EZ_VERIFY(fout.Open(fileNameOut.GetData()).Succeeded(), "Could not create output file '%s'", fileNameOut.GetData());

          if (pp.Process(fileName.GetData(), sOutput) == EZ_SUCCESS)
          {
            ezString sError = "Processing succeeded\r\n";
            fout.WriteBytes(sError.GetData(), sError.GetElementCount());
            fout.WriteBytes(sOutput.GetData(), sOutput.GetElementCount());
          }
          else
          {
            ezString sError = "Processing failed\r\n";
            fout.WriteBytes(sError.GetData(), sError.GetElementCount());
          }

          fout.WriteBytes(log.m_sOutput.GetData(), log.m_sOutput.GetElementCount());

          EZ_TEST_BOOL_MSG(ezFileSystem::ExistsFile(fileNameOut.GetData()), "Output file is missing: '%s'", fileNameOut.GetData());
        }

        EZ_TEST_FILES(fileNameOut.GetData(), fileNameExp.GetData(), "");
      }
    }
  }


  ezFileSystem::RemoveDataDirectoryGroup("PreprocessorTest");
}


