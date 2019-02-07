#include <FoundationTestPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/HTMLWriter.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/VisualStudioWriter.h>

EZ_CREATE_SIMPLE_TEST_GROUP(Logging);

class ezTestLogInterface : public ezLogInterface
{
public:
  virtual void HandleLogMessage(const ezLoggingEventData& le) override
  {
    switch (le.m_EventType)
    {
      case ezLogMsgType::BeginGroup:
        m_Result.Append(">", le.m_szTag, " ", le.m_szText, "\n");
        break;
      case ezLogMsgType::EndGroup:
        m_Result.Append("<", le.m_szTag, " ", le.m_szText, "\n");
        break;
      case ezLogMsgType::ErrorMsg:
        m_Result.Append("E:", le.m_szTag, " ", le.m_szText, "\n");
        break;
      case ezLogMsgType::SeriousWarningMsg:
        m_Result.Append("SW:", le.m_szTag, " ", le.m_szText, "\n");
        break;
      case ezLogMsgType::WarningMsg:
        m_Result.Append("W:", le.m_szTag, " ", le.m_szText, "\n");
        break;
      case ezLogMsgType::SuccessMsg:
        m_Result.Append("S:", le.m_szTag, " ", le.m_szText, "\n");
        break;
      case ezLogMsgType::InfoMsg:
        m_Result.Append("I:", le.m_szTag, " ", le.m_szText, "\n");
        break;
      case ezLogMsgType::DevMsg:
        m_Result.Append("E:", le.m_szTag, " ", le.m_szText, "\n");
        break;
      case ezLogMsgType::DebugMsg:
        m_Result.Append("D:", le.m_szTag, " ", le.m_szText, "\n");
        break;

      default:
        EZ_REPORT_FAILURE("Invalid msg type");
        break;
    }
  }

  ezStringBuilder m_Result;
};

EZ_CREATE_SIMPLE_TEST(Logging, Log)
{
  ezTestLogInterface log;
  ezTestLogInterface log2;
  ezLogSystemScope logScope(&log);

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Output")
  {
    EZ_LOG_BLOCK("Verse 1", "Portal: Still Alive");

    ezLog::GetThreadLocalLogSystem()->SetLogLevel(ezLogMsgType::All);

    ezLog::Success("{0}", "This was a triumph.");
    ezLog::Info("{0}", "I'm making a note here:");
    ezLog::Error("{0}", "Huge Success");
    ezLog::Info("{0}", "It's hard to overstate my satisfaction.");
    ezLog::Dev("{0}", "Aperture Science. We do what we must, because we can,");
    ezLog::Dev("{0}", "For the good of all of us, except the ones who are dead.");

    {
      EZ_LOG_BLOCK("Verse 2");

      ezLog::GetThreadLocalLogSystem()->SetLogLevel(ezLogMsgType::DevMsg);

      ezLog::Dev("But there's no sense crying over every mistake.");
      ezLog::Debug("You just keep on trying 'till you run out of cake.");
      ezLog::Info("And the science gets done, and you make a neat gun");
      ezLog::Error("for the people who are still alive.");
    }

    {
      EZ_LOG_BLOCK("Verse 3");

      ezLog::GetThreadLocalLogSystem()->SetLogLevel(ezLogMsgType::InfoMsg);

      ezLog::Info("I'm not even angry.");
      ezLog::Debug("I'm being so sincere right now.");
      ezLog::Dev("Even though you broke my heart and killed me.");
      ezLog::Info("And tore me to pieces,");
      ezLog::Dev("and threw every piece into a fire.");
      ezLog::Info("As they burned it hurt because I was so happy for you.");
      ezLog::Error("Now these points of data make a beautiful line");
      ezLog::Dev("and we're off the beta, we're releasing on time.");

      {
        EZ_LOG_BLOCK("Verse 4");

        ezLog::GetThreadLocalLogSystem()->SetLogLevel(ezLogMsgType::SuccessMsg);

        ezLog::Info("So I'm glad I got burned,");
        ezLog::Debug("think of all the things we learned");
        ezLog::Debug("for the people who are still alive.");

        {
          ezLogSystemScope logScope(&log2);
          EZ_LOG_BLOCK("Interlude");
          ezLog::Info("Well here we are again. It's always such a pleasure.");
          ezLog::Error("Remember when you tried to kill me twice?");
        }

        {
          EZ_LOG_BLOCK("Verse 5");

          ezLog::GetThreadLocalLogSystem()->SetLogLevel(ezLogMsgType::WarningMsg);

          ezLog::Debug("Go ahead and leave me.");
          ezLog::Info("I think I prefer to stay inside.");
          ezLog::Dev("Maybe you'll find someone else, to help you.");
          ezLog::Dev("Maybe Black Mesa.");
          ezLog::Info("That was a joke. Haha. Fat chance.");
          ezLog::Warning("Anyway, this cake is great.");
          ezLog::Success("It's so delicious and moist.");
          ezLog::Dev("Look at me still talking when there's science to do.");
          ezLog::Error("When I look up there it makes me glad I'm not you.");
          ezLog::Info("I've experiments to run,");
          ezLog::SeriousWarning("there is research to be done on the people who are still alive.");
        }
      }
    }
  }

  {
    EZ_LOG_BLOCK("Verse 6", "Last One");

    ezLog::GetThreadLocalLogSystem()->SetLogLevel(ezLogMsgType::ErrorMsg);

    ezLog::Dev("And believe me I am still alive.");
    ezLog::Info("I'm doing science and I'm still alive.");
    ezLog::Success("I feel fantastic and I'm still alive.");
    ezLog::Warning("While you're dying I'll be still alive.");
    ezLog::Error("And when you're dead I will be, still alive.");
    ezLog::Debug("Still alive, still alive.");
  }

  /// \todo This test will fail if EZ_COMPILE_FOR_DEVELOPMENT is disabled.
  /// We also currently don't test ezLog::Debug, because our build machines compile in release and then the text below would need to be
  /// different.

  const char* szResult = log.m_Result;
  const char* szExpected = "\
>Portal: Still Alive Verse 1\n\
S: This was a triumph.\n\
I: I'm making a note here:\n\
E: Huge Success\n\
I: It's hard to overstate my satisfaction.\n\
E: Aperture Science. We do what we must, because we can,\n\
E: For the good of all of us, except the ones who are dead.\n\
> Verse 2\n\
E: But there's no sense crying over every mistake.\n\
I: And the science gets done, and you make a neat gun\n\
E: for the people who are still alive.\n\
< Verse 2\n\
> Verse 3\n\
I: I'm not even angry.\n\
I: And tore me to pieces,\n\
I: As they burned it hurt because I was so happy for you.\n\
E: Now these points of data make a beautiful line\n\
> Verse 4\n\
> Verse 5\n\
W: Anyway, this cake is great.\n\
E: When I look up there it makes me glad I'm not you.\n\
SW: there is research to be done on the people who are still alive.\n\
< Verse 5\n\
< Verse 4\n\
< Verse 3\n\
<Portal: Still Alive Verse 1\n\
>Last One Verse 6\n\
E: And when you're dead I will be, still alive.\n\
<Last One Verse 6\n\
";

  EZ_TEST_STRING(szResult, szExpected);

  const char* szResult2 = log2.m_Result;
  const char* szExpected2 = "\
> Interlude\n\
I: Well here we are again. It's always such a pleasure.\n\
E: Remember when you tried to kill me twice?\n\
< Interlude\n\
";

  EZ_TEST_STRING(szResult2, szExpected2);

  ezLog::GetThreadLocalLogSystem()->SetLogLevel(ezLogMsgType::All);
}
