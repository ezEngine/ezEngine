#include <EditorTest/EditorTestPCH.h>

#include <EditorFramework/EditorApp/StackTraceLogParser.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)

EZ_CREATE_SIMPLE_TEST_GROUP(Platform);

EZ_CREATE_SIMPLE_TEST(Platform, StackTracer)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Parse StackTrace FileName & LineNumber")
  {
    ezStringView sFileName;
    ezInt32 lineNumber;

    // Happy path
    {
      ezStringView sLogMessage = "  C:\\dev\\ez engine\\Game\\GamePlugin\\Components\\TestComponent.cpp(238):'TestComponent::Update'";
      EZ_TEST_BOOL(ezStackTraceLogParser::ParseStackTraceFileNameAndLineNumber(sLogMessage, sFileName, lineNumber) == true);
      EZ_TEST_STRING(sFileName, "C:\\dev\\ez engine\\Game\\GamePlugin\\Components\\TestComponent.cpp");
      EZ_TEST_INT(lineNumber, 238);
    }

    // Funky file name
    {
      ezStringView sLogMessage = "  C:\\dev\\ezengine\\Game\\GamePlugin\\Components\\funkynam1 e..(238)..cpp(238):'TestComponent::Update'";
      EZ_TEST_BOOL(ezStackTraceLogParser::ParseStackTraceFileNameAndLineNumber(sLogMessage, sFileName, lineNumber) == true);
      EZ_TEST_STRING(sFileName, "C:\\dev\\ezengine\\Game\\GamePlugin\\Components\\funkynam1 e..(238)..cpp");
      EZ_TEST_INT(lineNumber, 238);
    }

    // UTF-8 Filename
    {
      ezStringView sLogMessage = "  C:\\dev\\ezengine\\你好ÖÖÜÜê\\GamePlugin\\Components\\你好ÖÖÜÜê.cpp(238):'TestComponent::Update'";
      EZ_TEST_BOOL(ezStackTraceLogParser::ParseStackTraceFileNameAndLineNumber(sLogMessage, sFileName, lineNumber) == true);
      EZ_TEST_STRING(sFileName, "C:\\dev\\ezengine\\你好ÖÖÜÜê\\GamePlugin\\Components\\你好ÖÖÜÜê.cpp");
      EZ_TEST_INT(lineNumber, 238);
    }

    // Normal Log Message
    {
      ezStringView sLogMessage = "  ezInputManager::GetInputSlotState: Input Slot 'mouse_position_x' does not exist (yet):...";
      EZ_TEST_BOOL(ezStackTraceLogParser::ParseStackTraceFileNameAndLineNumber(sLogMessage, sFileName, lineNumber) == false);
    }

    // Empty Log Message
    {
      ezStringView sLogMessage = "";
      EZ_TEST_BOOL(ezStackTraceLogParser::ParseStackTraceFileNameAndLineNumber(sLogMessage, sFileName, lineNumber) == false);
    }

    // Invalid
    {
      ezStringView sLogMessage;
      EZ_TEST_BOOL(ezStackTraceLogParser::ParseStackTraceFileNameAndLineNumber(sLogMessage, sFileName, lineNumber) == false);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Parse Assert FileName & LineNumber")
  {
    ezStringView sFileName;
    ezInt32 lineNumber;

    // Happy path
    {
      ezStringView sLogMessage = "*** Assertion ***: File: \"C:\\dev\\ez engine\\Game\\GamePlugin\\Components\\TestComponent.cpp\", Line: \"136\", Function: \"TestComponent::Update\", Expression: \"false\", Message: \"Assert!\"";
      EZ_TEST_BOOL(ezStackTraceLogParser::ParseAssertFileNameAndLineNumber(sLogMessage, sFileName, lineNumber) == true);
      EZ_TEST_STRING(sFileName, "C:\\dev\\ez engine\\Game\\GamePlugin\\Components\\TestComponent.cpp");
      EZ_TEST_INT(lineNumber, 136);
    }

    // Normal Log Message
    {
      ezStringView sLogMessage = "  ezInputManager::GetInputSlotState: Input Slot 'mouse_position_x' does not exist (yet):...";
      EZ_TEST_BOOL(ezStackTraceLogParser::ParseAssertFileNameAndLineNumber(sLogMessage, sFileName, lineNumber) == false);
    }

    // Broken log message
    {
      ezStringView sLogMessage = "*** Assertion ***: File: \"C:\\dev\\ez engine\\Game\\GamePlugin\\Components\\TestComponent.cpp\", Line: \"12";
      EZ_TEST_BOOL(ezStackTraceLogParser::ParseAssertFileNameAndLineNumber(sLogMessage, sFileName, lineNumber) == false);
    }
  }
}


#endif