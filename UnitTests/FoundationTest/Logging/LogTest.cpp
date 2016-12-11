#include <PCH.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/HTMLWriter.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/Configuration/Startup.h>

#define VerboseDebugMessage Debug

EZ_CREATE_SIMPLE_TEST_GROUP(Logging);

EZ_CREATE_SIMPLE_TEST(Logging, Log)
{
  ezLogWriter::HTML LogHTML;

  ezStringBuilder sOutputFolder1 = BUILDSYSTEM_OUTPUT_FOLDER;
  sOutputFolder1.AppendPath("FoundationTest", "Logging");

  ezOSFile::CreateDirectoryStructure(sOutputFolder1.GetData());

  ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);
  EZ_TEST_BOOL(ezFileSystem::AddDataDirectory(sOutputFolder1.GetData(), "LoggingTest", "output", ezFileSystem::AllowWrites) == EZ_SUCCESS);

  LogHTML.BeginLog(":output/Log_FoundationTest.htm", "FoundationTest");

  ezGlobalLog::AddLogWriter(ezLogWriter::Console::LogMessageHandler);
  ezGlobalLog::AddLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);
  ezGlobalLog::AddLogWriter(ezLoggingEvent::Handler(&ezLogWriter::HTML::LogMessageHandler, &LogHTML));

  ezStartup::PrintAllSubsystems();

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Output")
  {
    EZ_LOG_BLOCK("Verse 1", "Portal: Still Alive");

    ezLog::SuccessPrintf("%s", "This was a triumph.");
    ezLog::InfoPrintf("%s", "I'm making a note here:");
    ezLog::SuccessPrintf("%s", "Huge Success");
    ezLog::InfoPrintf("%s", "It's hard to overstate my satisfaction.");
    ezLog::DevPrintf("%s", "Aperture Science. We do what we must, because we can,");
    ezLog::DebugPrintf("%s", "For the good of all of us, except the ones who are dead.");

    {
      EZ_LOG_BLOCK("Verse 2");

      ezLog::DevPrintf("But there's no sense crying over every mistake.");
      ezLog::DebugPrintf("You just keep on trying 'till you run out of cake.");
      ezLog::InfoPrintf("And the science gets done, and you make a neat gun");
      ezLog::VerboseDebugMessagePrintf("for the people who are still alive.");
    }

    {
      EZ_LOG_BLOCK("Verse 3");

      ezLog::InfoPrintf("I'm not even angry.");
      ezLog::DebugPrintf("I'm being so sincere right now.");
      ezLog::DevPrintf("Even though you broke my heart and killed me.");
      ezLog::InfoPrintf("And tore me to pieces,");
      ezLog::DevPrintf("and threw every piece into a fire.");
      ezLog::InfoPrintf("As they burned it hurt because I was so happy for you.");
      ezLog::DevPrintf("Now these points of data make a beautiful line");
      ezLog::DevPrintf("and we're off the beta, we're releasing on time.");

      {
        EZ_LOG_BLOCK("Verse 4");

        ezLog::InfoPrintf("So I'm glad I got burned,");
        ezLog::DebugPrintf("think of all the things we learned");
        ezLog::VerboseDebugMessagePrintf("for the people who are still alive.");

        {
          EZ_LOG_BLOCK("Verse 5");

          ezLog::DebugPrintf("Go ahead and leave me.");
          ezLog::InfoPrintf("I think I prefer to stay inside.");
          ezLog::DevPrintf("Maybe you'll find someone else, to help you.");
          ezLog::DevPrintf("Maybe Black Mesa.");
          ezLog::InfoPrintf("That was a joke. Haha. Fat chance.");
          ezLog::SuccessPrintf("Anyway, this cake is great.");
          ezLog::SuccessPrintf("It's so delicious and moist.");
          ezLog::DevPrintf("Look at me still talking when there's science to do.");
          ezLog::DebugPrintf("When I look up there it makes me glad I'm not you.");
          ezLog::InfoPrintf("I've experiments to run,");
          ezLog::VerboseDebugMessagePrintf("there is research to be done on the people who are still alive.");
        }
      }
    }
  }

  {
    EZ_LOG_BLOCK("Verse 6", "Last One");

    ezLog::DevPrintf("And believe me I am still alive.");
    ezLog::InfoPrintf("I'm doing science and I'm still alive.");
    ezLog::SuccessPrintf("I feel fantastic and I'm still alive.");
    ezLog::DevPrintf("While you're dying I'll be still alive.");
    ezLog::DevPrintf("And when you're dead I will be, still alive.");
    ezLog::VerboseDebugMessagePrintf("Still alive, still alive.");
  }

  ezGlobalLog::RemoveLogWriter(ezLogWriter::Console::LogMessageHandler);
  ezGlobalLog::RemoveLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);
  ezGlobalLog::RemoveLogWriter(ezLoggingEvent::Handler(&ezLogWriter::HTML::LogMessageHandler, &LogHTML));

  LogHTML.EndLog();

  ezFileSystem::RemoveDataDirectoryGroup("LoggingTest");
  ezFileSystem::ClearAllDataDirectoryFactories();
}


