#include <PCH.h>
#include <CoreUtils/CodeUtils/Preprocessor.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>

EZ_CREATE_SIMPLE_TEST_GROUP(CodeUtils);

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
    default:
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

    m_EventStack.PopBack();
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

  /// \todo Add tests for the following:
  /* 
    macro expansion
    macro expansion in #if
    custom defines from outside
    stringify invalid token
    concatenate invalid tokens, tokens that yield valid macro
    broken function macros (missing parenthesis etc.)
    errors after #line directive
    errors in
      #line directive
      #define
      #ifdef
      etc.

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
    too few, too many parameters
    __VA_ARGS__
    expansion that needs several iterations
    stringification of strings and special characters (\n)
    commas in macro parameters
    #undef
    unlocateable include file
    pass through #pragma
    pass through #line
    invalid #if, #else, #elif, #endif nesting
    #ifdef, #if etc.
    mathematical expressions
    boolean expressions
    bitwise expressions
    expand to self (with, without parameters, with parameters to expand)
    incorrect expressions
    */

  {
    struct PPTestSettings
    {
      PPTestSettings(const char* szFileName, bool bPassThroughLines = false, bool bPassThroughPragmas = false, bool bPassThroughUnknownCommands = false)
        : m_szFileName(szFileName), m_bPassThroughLines(bPassThroughLines), m_bPassThroughPragmas(bPassThroughPragmas), m_bPassThroughUnknownCommands(bPassThroughUnknownCommands)
      {
      }

      const char* m_szFileName;
      bool m_bPassThroughLines;
      bool m_bPassThroughPragmas;
      bool m_bPassThroughUnknownCommands;
    };

    PPTestSettings TestSettings[] =
    {
      PPTestSettings("LinePragmaPassThrough", true, true),
      PPTestSettings("Undef"),
      PPTestSettings("InvalidIf1"),
      PPTestSettings("Parameters"),
      PPTestSettings("LineControl"),
      PPTestSettings("LineControl2"),
      PPTestSettings("DefineFile"),
      PPTestSettings("DefineLine"),
      PPTestSettings("DefineDefined"),
      PPTestSettings("Stringify"),
      PPTestSettings("BuildFlags"),
      PPTestSettings("Empty"),
      PPTestSettings("Test1"),
      PPTestSettings("FailedInclude"), /// \todo Better error message
      PPTestSettings("PassThroughUnknown", false, false, true),
      PPTestSettings("IncorrectNesting1"),
      PPTestSettings("IncorrectNesting2"),
      PPTestSettings("IncorrectNesting3"),
      PPTestSettings("IncorrectNesting4"),
      PPTestSettings("IncorrectNesting5"),
      PPTestSettings("IncorrectNesting6"),
      PPTestSettings("IncorrectNesting7"),
      PPTestSettings("Error"),
      PPTestSettings("Warning"),
      PPTestSettings("Expressions"),
      PPTestSettings("ExpressionsBit"),
      PPTestSettings("ExpandSelf"),
      PPTestSettings("InvalidLogic1"),
      PPTestSettings("InvalidLogic2"),
      PPTestSettings("InvalidLogic3"),
      PPTestSettings("InvalidLogic4"),
      PPTestSettings("InvalidExpandSelf1"),
      PPTestSettings("InvalidExpandSelf2"),
    };

    ezStringBuilder sOutput;
    ezStringBuilder fileName;
    ezStringBuilder fileNameOut;
    ezStringBuilder fileNameExp;

    for (int i = 0; i < EZ_ARRAY_SIZE(TestSettings); i++)
    {
      EZ_TEST_BLOCK(ezTestBlock::Enabled, TestSettings[i].m_szFileName)
      {
        Logger log;

        ezPreprocessor pp;
        pp.SetLogInterface(&log);
        pp.SetPassThroughLine(TestSettings[i].m_bPassThroughLines);
        pp.SetPassThroughPragma(TestSettings[i].m_bPassThroughPragmas);
        pp.SetFileLocatorFunction(FileLocator);
        pp.SetCustomFileCache(&SharedCache);
        pp.m_ProcessingEvents.AddEventHandler(ezDelegate<void (const ezPreprocessor::ProcessingEvent&)>(&Logger::EventHandler, &log));
        pp.AddCustomDefine("PP_OBJ");
        pp.AddCustomDefine("PP_FUNC(a) a");
        pp.SetPassThroughUnknownCmdsCB([](const char* s)->bool { return ezStringUtils::IsEqual(s, "version"); });//TestSettings[i].m_bPassThroughUnknownCommands);

        {
          fileName.Format("Preprocessor/%s.txt", TestSettings[i].m_szFileName);
          fileNameExp.Format("Preprocessor/%s - Expected.txt", TestSettings[i].m_szFileName);
          fileNameOut.Format("Preprocessor/%s - Result.txt", TestSettings[i].m_szFileName);

          EZ_TEST_BOOL_MSG(ezFileSystem::ExistsFile(fileName.GetData()), "File does not exist: '%s'", fileName.GetData());

          ezFileWriter fout;
          EZ_VERIFY(fout.Open(fileNameOut.GetData()).Succeeded(), "Could not create output file '%s'", fileNameOut.GetData());

          if (pp.Process(fileName.GetData(), sOutput) == EZ_SUCCESS)
          {
            ezString sError = "Processing succeeded\r\n";
            fout.WriteBytes(sError.GetData(), sError.GetElementCount());
            fout.WriteBytes(sOutput.GetData(), sOutput.GetElementCount());

            if (!log.m_sOutput.IsEmpty())
              fout.WriteBytes("\r\n", 2);
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


