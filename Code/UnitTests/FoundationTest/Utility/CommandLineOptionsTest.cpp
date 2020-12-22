#include <FoundationTestPCH.h>

#include <Foundation/Utilities/CommandLineOptions.h>

namespace
{
  class LogTestLogInterface : public ezLogInterface
  {
  public:
    virtual void HandleLogMessage(const ezLoggingEventData& le) override
    {
      switch (le.m_EventType)
      {
        case ezLogMsgType::Flush:
          m_Result.Append("[Flush]\n");
          return;
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

} // namespace

EZ_CREATE_SIMPLE_TEST(Utility, CommandLineOptions)
{
  ezCommandLineOptionDoc optDoc("__test", "-argDoc", "<doc>", "Doc argument", "no value");

  ezCommandLineOptionBool optBool1("__test", "-bool1", "bool argument 1", false);
  ezCommandLineOptionBool optBool2("__test", "-bool2", "bool argument 2", true);

  ezCommandLineOptionInt optInt1("__test", "-int1", "int argument 1", 1);
  ezCommandLineOptionInt optInt2("__test", "-int2", "int argument 2", 0, 4, 8);
  ezCommandLineOptionInt optInt3("__test", "-int3", "int argument 3", 6, -8, 8);

  ezCommandLineOptionFloat optFloat1("__test", "-float1", "float argument 1", 1);
  ezCommandLineOptionFloat optFloat2("__test", "-float2", "float argument 2", 0, 4, 8);
  ezCommandLineOptionFloat optFloat3("__test", "-float3", "float argument 3", 6, -8, 8);

  ezCommandLineOptionString optString1("__test", "-string1", "string argument 1", "default string");

  ezCommandLineOptionPath optPath1("__test", "-path1", "path argument 1", "default path");

  ezCommandLineOptionEnum optEnum1("__test", "-enum1", "enum argument 1", "A | B = 2 | C | D | E = 7", 3);

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezCommandLineOptionBool")
  {
    ezCommandLineUtils cmd;
    cmd.InjectCustomArgument("-bool1");
    cmd.InjectCustomArgument("on");

    EZ_TEST_BOOL(optBool1.GetOptionValue(ezCommandLineOption::LogMode::Never, &cmd) == true);
    EZ_TEST_BOOL(optBool2.GetOptionValue(ezCommandLineOption::LogMode::Never, &cmd) == true);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezCommandLineOptionInt")
  {
    ezCommandLineUtils cmd;
    cmd.InjectCustomArgument("-int1");
    cmd.InjectCustomArgument("3");

    cmd.InjectCustomArgument("-int2");
    cmd.InjectCustomArgument("10");

    cmd.InjectCustomArgument("-int3");
    cmd.InjectCustomArgument("-2");

    EZ_TEST_INT(optInt1.GetOptionValue(ezCommandLineOption::LogMode::Never, &cmd), 3);
    EZ_TEST_INT(optInt2.GetOptionValue(ezCommandLineOption::LogMode::Never, &cmd), 0);
    EZ_TEST_INT(optInt3.GetOptionValue(ezCommandLineOption::LogMode::Never, &cmd), -2);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezCommandLineOptionFloat")
  {
    ezCommandLineUtils cmd;
    cmd.InjectCustomArgument("-float1");
    cmd.InjectCustomArgument("3");

    cmd.InjectCustomArgument("-float2");
    cmd.InjectCustomArgument("10");

    cmd.InjectCustomArgument("-float3");
    cmd.InjectCustomArgument("-2");

    EZ_TEST_FLOAT(optFloat1.GetOptionValue(ezCommandLineOption::LogMode::Never, &cmd), 3, 0.001f);
    EZ_TEST_FLOAT(optFloat2.GetOptionValue(ezCommandLineOption::LogMode::Never, &cmd), 0, 0.001f);
    EZ_TEST_FLOAT(optFloat3.GetOptionValue(ezCommandLineOption::LogMode::Never, &cmd), -2, 0.001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezCommandLineOptionString")
  {
    ezCommandLineUtils cmd;
    cmd.InjectCustomArgument("-string1");
    cmd.InjectCustomArgument("hello");

    EZ_TEST_STRING(optString1.GetOptionValue(ezCommandLineOption::LogMode::Never, &cmd), "hello");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezCommandLineOptionPath")
  {
    ezCommandLineUtils cmd;
    cmd.InjectCustomArgument("-path1");
    cmd.InjectCustomArgument("C:/test");

    EZ_TEST_STRING(optPath1.GetOptionValue(ezCommandLineOption::LogMode::Never, &cmd), "C:/test");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezCommandLineOptionEnum")
  {
    {
      ezCommandLineUtils cmd;
      cmd.InjectCustomArgument("-enum1");
      cmd.InjectCustomArgument("A");

      EZ_TEST_INT(optEnum1.GetOptionValue(ezCommandLineOption::LogMode::Never, &cmd), 0);
    }

    {
      ezCommandLineUtils cmd;
      cmd.InjectCustomArgument("-enum1");
      cmd.InjectCustomArgument("B");

      EZ_TEST_INT(optEnum1.GetOptionValue(ezCommandLineOption::LogMode::Never, &cmd), 2);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "LogAvailableOptions")
  {
    ezCommandLineUtils cmd;

    LogTestLogInterface log;
    ezLogSystemScope _ls(&log);

    EZ_TEST_BOOL(ezCommandLineOption::LogAvailableOptions(ezCommandLineOption::LogAvailableModes::Always, "__test", &cmd));

    EZ_TEST_STRING(log.m_Result, "\
I: \n\
I: -argDoc <doc> = no value\n\
I:     Doc argument\n\
I: \n\
I: -bool1 <bool> = false\n\
I:     bool argument 1\n\
I: \n\
I: -bool2 <bool> = true\n\
I:     bool argument 2\n\
I: \n\
I: -int1 <int> = 1\n\
I:     int argument 1\n\
I: \n\
I: -int2 <int> [4 .. 8] = 0\n\
I:     int argument 2\n\
I: \n\
I: -int3 <int> [-8 .. 8] = 6\n\
I:     int argument 3\n\
I: \n\
I: -float1 <float> = 1\n\
I:     float argument 1\n\
I: \n\
I: -float2 <float> [4 .. 8] = 0\n\
I:     float argument 2\n\
I: \n\
I: -float3 <float> [-8 .. 8] = 6\n\
I:     float argument 3\n\
I: \n\
I: -string1 <string> = default string\n\
I:     string argument 1\n\
I: \n\
I: -path1 <path> = default path\n\
I:     path argument 1\n\
I: \n\
I: -enum1 <A | B | C | D | E> = C\n\
I:     enum argument 1\n\
I: \n\
I: \n\
");
  }
}
