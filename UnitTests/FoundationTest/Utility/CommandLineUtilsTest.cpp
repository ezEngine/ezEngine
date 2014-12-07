#include <PCH.h>
#include <Foundation/Utilities/CommandLineUtils.h>

EZ_CREATE_SIMPLE_TEST(Utility, CommandLineUtils)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetParameterCount / GetParameter")
  {
    const int argc = 9;
    const char* argv[argc] =
    {
      "bla/blub/myprogram.exe",
      "-Test1", "true",
      "-Test2", "off",
      "-Test3",
      "-Test4", "on",
      "-Test5"
    };

    ezCommandLineUtils CmdLn;
    CmdLn.SetCommandLine(argc, argv);

    EZ_TEST_INT(CmdLn.GetParameterCount(), 8);
    EZ_TEST_STRING(CmdLn.GetParameter(0), "-Test1");
    EZ_TEST_STRING(CmdLn.GetParameter(1), "true");
    EZ_TEST_STRING(CmdLn.GetParameter(2), "-Test2");
    EZ_TEST_STRING(CmdLn.GetParameter(3), "off");
    EZ_TEST_STRING(CmdLn.GetParameter(4), "-Test3");
    EZ_TEST_STRING(CmdLn.GetParameter(5), "-Test4");
    EZ_TEST_STRING(CmdLn.GetParameter(6), "on");
    EZ_TEST_STRING(CmdLn.GetParameter(7), "-Test5");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetOptionIndex / GetStringOptionArguments  / GetStringOption")
  {
    const int argc = 15;
    const char* argv[argc] =
    {
      "bla/blub/myprogram.exe",
      "-opt1", "true", "false",
      "-opt2", "\"test2\"",
      "-opt3",
      "-opt4", "one", "two = three", "four", "   five  ", " six ",
      "-opt5",
      "-opt6"
    };

    ezCommandLineUtils CmdLn;
    CmdLn.SetCommandLine(argc, argv);

    EZ_TEST_INT(CmdLn.GetOptionIndex("-opt1"), 0);
    EZ_TEST_INT(CmdLn.GetOptionIndex("-opt2"), 3);
    EZ_TEST_INT(CmdLn.GetOptionIndex("-opt3"), 5);
    EZ_TEST_INT(CmdLn.GetOptionIndex("-opt4"), 6);
    EZ_TEST_INT(CmdLn.GetOptionIndex("-opt5"), 12);
    EZ_TEST_INT(CmdLn.GetOptionIndex("-opt6"), 13);

    EZ_TEST_INT(CmdLn.GetStringOptionArguments("-opt1"), 2);
    EZ_TEST_INT(CmdLn.GetStringOptionArguments("-opt2"), 1);
    EZ_TEST_INT(CmdLn.GetStringOptionArguments("-opt3"), 0);
    EZ_TEST_INT(CmdLn.GetStringOptionArguments("-opt4"), 5);
    EZ_TEST_INT(CmdLn.GetStringOptionArguments("-opt5"), 0);
    EZ_TEST_INT(CmdLn.GetStringOptionArguments("-opt6"), 0);

    EZ_TEST_STRING(CmdLn.GetStringOption("-opt1", 0), "true");
    EZ_TEST_STRING(CmdLn.GetStringOption("-opt1", 1), "false");
    EZ_TEST_STRING(CmdLn.GetStringOption("-opt1", 2, "end"), "end");

    EZ_TEST_STRING(CmdLn.GetStringOption("-opt2", 0), "\"test2\"");
    EZ_TEST_STRING(CmdLn.GetStringOption("-opt2", 1, "end"), "end");

    EZ_TEST_STRING(CmdLn.GetStringOption("-opt3", 0, "end"), "end");

    EZ_TEST_STRING(CmdLn.GetStringOption("-opt4", 0), "one");
    EZ_TEST_STRING(CmdLn.GetStringOption("-opt4", 1), "two = three");
    EZ_TEST_STRING(CmdLn.GetStringOption("-opt4", 2), "four");
    EZ_TEST_STRING(CmdLn.GetStringOption("-opt4", 3), "   five  ");
    EZ_TEST_STRING(CmdLn.GetStringOption("-opt4", 4), " six ");
    EZ_TEST_STRING(CmdLn.GetStringOption("-opt4", 5, "end"), "end");

    EZ_TEST_STRING(CmdLn.GetStringOption("-opt5", 0), "");

    EZ_TEST_STRING(CmdLn.GetStringOption("-opt6", 0), "");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetBoolOption")
  {
    const int argc = 9;
    const char* argv[argc] =
    {
      "bla/blub/myprogram.exe",
      "-Test1", "true",
      "-Test2", "off",
      "-Test3",
      "-Test4", "on",
      "-Test5"
    };

    ezCommandLineUtils CmdLn;
    CmdLn.SetCommandLine(argc, argv);

    // case sensitive and wrong
    EZ_TEST_BOOL(CmdLn.GetBoolOption("-test1", true, true)  == true);
    EZ_TEST_BOOL(CmdLn.GetBoolOption("-test1", false, true) == false);

    EZ_TEST_BOOL(CmdLn.GetBoolOption("-test2", true, true)  == true);
    EZ_TEST_BOOL(CmdLn.GetBoolOption("-test2", false, true) == false);

    // case insensitive and wrong
    EZ_TEST_BOOL(CmdLn.GetBoolOption("-test1", true)  == true);
    EZ_TEST_BOOL(CmdLn.GetBoolOption("-test1", false) == true);

    EZ_TEST_BOOL(CmdLn.GetBoolOption("-test2", true)  == false);
    EZ_TEST_BOOL(CmdLn.GetBoolOption("-test2", false) == false);

    // case sensitive and correct
    EZ_TEST_BOOL(CmdLn.GetBoolOption("-Test1", true)  == true);
    EZ_TEST_BOOL(CmdLn.GetBoolOption("-Test1", false) == true);

    EZ_TEST_BOOL(CmdLn.GetBoolOption("-Test2", true)  == false);
    EZ_TEST_BOOL(CmdLn.GetBoolOption("-Test2", false) == false);

    EZ_TEST_BOOL(CmdLn.GetBoolOption("-Test3", true)  == true);
    EZ_TEST_BOOL(CmdLn.GetBoolOption("-Test3", false) == true);

    EZ_TEST_BOOL(CmdLn.GetBoolOption("-Test4", true)  == true);
    EZ_TEST_BOOL(CmdLn.GetBoolOption("-Test4", false) == true);

    EZ_TEST_BOOL(CmdLn.GetBoolOption("-Test5", true)  == true);
    EZ_TEST_BOOL(CmdLn.GetBoolOption("-Test5", false) == true);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetIntOption")
  {
    const int argc = 9;
    const char* argv[argc] =
    {
      "bla/blub/myprogram.exe",
      "-Test1", "23",
      "-Test2", "42",
      "-Test3",
      "-Test4", "11",
      "-Test5"
    };

    ezCommandLineUtils CmdLn;
    CmdLn.SetCommandLine(argc, argv);

    // case sensitive and wrong
    EZ_TEST_INT(CmdLn.GetIntOption("-test1", 2, true), 2);
    EZ_TEST_INT(CmdLn.GetIntOption("-test2", 17, true), 17);

    // case insensitive and wrong
    EZ_TEST_INT(CmdLn.GetIntOption("-test1", 2), 23);
    EZ_TEST_INT(CmdLn.GetIntOption("-test2", 17), 42);

    // case sensitive and correct
    EZ_TEST_INT(CmdLn.GetIntOption("-Test1", 2), 23);
    EZ_TEST_INT(CmdLn.GetIntOption("-Test2", 3), 42);
    EZ_TEST_INT(CmdLn.GetIntOption("-Test3", 4), 4);
    EZ_TEST_INT(CmdLn.GetIntOption("-Test4", 5), 11);
    EZ_TEST_INT(CmdLn.GetIntOption("-Test5"), 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetFloatOption")
  {
    const int argc = 9;
    const char* argv[argc] =
    {
      "bla/blub/myprogram.exe",
      "-Test1", "23.45",
      "-Test2", "42.3",
      "-Test3",
      "-Test4", "11",
      "-Test5"
    };

    ezCommandLineUtils CmdLn;
    CmdLn.SetCommandLine(argc, argv);

    // case sensitive and wrong
    EZ_TEST_DOUBLE(CmdLn.GetFloatOption("-test1", 2.3, true), 2.3, 0.0);
    EZ_TEST_DOUBLE(CmdLn.GetFloatOption("-test2", 17.8, true), 17.8, 0.0);

    // case insensitive and wrong
    EZ_TEST_DOUBLE(CmdLn.GetFloatOption("-test1", 2.3), 23.45, 0.0);
    EZ_TEST_DOUBLE(CmdLn.GetFloatOption("-test2", 17.8), 42.3, 0.0);

    // case sensitive and correct
    EZ_TEST_DOUBLE(CmdLn.GetFloatOption("-Test1", 2.3), 23.45, 0.0);
    EZ_TEST_DOUBLE(CmdLn.GetFloatOption("-Test2", 3.4), 42.3, 0.0);
    EZ_TEST_DOUBLE(CmdLn.GetFloatOption("-Test3", 4.5), 4.5, 0.0);
    EZ_TEST_DOUBLE(CmdLn.GetFloatOption("-Test4", 5.6), 11, 0.0);
    EZ_TEST_DOUBLE(CmdLn.GetFloatOption("-Test5"), 0, 0.0);
  }

}


