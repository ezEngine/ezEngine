#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/System/EnvironmentVariableUtils.h>

static ezUInt32 uiVersionForVariableSetting = 0;

EZ_CREATE_SIMPLE_TEST(Utility, EnvironmentVariableUtils)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetValueString / GetValueInt")
  {
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)

    // Windows will have "NUMBER_OF_PROCESSORS" and "USERNAME" set, let's see if we can get them
    EZ_TEST_BOOL(ezEnvironmentVariableUtils::IsVariableSet("NUMBER_OF_PROCESSORS"));

    ezInt32 iNumProcessors = ezEnvironmentVariableUtils::GetValueInt("NUMBER_OF_PROCESSORS", -23);
    EZ_TEST_BOOL(iNumProcessors > 0);

    EZ_TEST_BOOL(ezEnvironmentVariableUtils::IsVariableSet("USERNAME"));
    ezString szUserName = ezEnvironmentVariableUtils::GetValueString("USERNAME");
    EZ_TEST_BOOL(szUserName.GetElementCount() > 0);

#elif EZ_ENABLED(EZ_PLATFORM_OSX) || EZ_ENABLED(EZ_PLATFORM_LINUX)

    // Mac OS & Linux will have "USER" set
    EZ_TEST_BOOL(ezEnvironmentVariableUtils::IsVariableSet("USER"));
    ezString szUserName = ezEnvironmentVariableUtils::GetValueString("USER");
    EZ_TEST_BOOL(szUserName.GetElementCount() > 0);

#endif
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsVariableSet/SetValue")
  {
    ezStringBuilder szVarName;
    szVarName.SetFormat("EZ_THIS_SHOULDNT_EXIST_NOW_OR_THIS_TEST_WILL_FAIL_{0}", uiVersionForVariableSetting++);

    EZ_TEST_BOOL(!ezEnvironmentVariableUtils::IsVariableSet(szVarName));

    ezEnvironmentVariableUtils::SetValueString(szVarName, "NOW_IT_SHOULD_BE").IgnoreResult();
    EZ_TEST_BOOL(ezEnvironmentVariableUtils::IsVariableSet(szVarName));

    EZ_TEST_STRING(ezEnvironmentVariableUtils::GetValueString(szVarName), "NOW_IT_SHOULD_BE");

    // Test overwriting the same value again
    ezEnvironmentVariableUtils::SetValueString(szVarName, "NOW_IT_SHOULD_BE_SOMETHING_ELSE").IgnoreResult();
    EZ_TEST_STRING(ezEnvironmentVariableUtils::GetValueString(szVarName), "NOW_IT_SHOULD_BE_SOMETHING_ELSE");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Variable with very long value")
  {
    // The Windows implementation has a 64 wchar_t buffer for example. Let's try setting a really
    // long variable and getting it back
    const char* szLongVariable =
      "SOME REALLY LONG VALUE, LETS TEST SOME LIMITS WE MIGHT HIT - 012456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ abcdefghijklmnopqrstuvwxyz";

    ezStringBuilder szVarName;
    szVarName.SetFormat("EZ_LONG_VARIABLE_TEST_{0}", uiVersionForVariableSetting++);

    EZ_TEST_BOOL(!ezEnvironmentVariableUtils::IsVariableSet(szVarName));

    ezEnvironmentVariableUtils::SetValueString(szVarName, szLongVariable).IgnoreResult();
    EZ_TEST_BOOL(ezEnvironmentVariableUtils::IsVariableSet(szVarName));

    EZ_TEST_STRING(ezEnvironmentVariableUtils::GetValueString(szVarName), szLongVariable);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Unsetting variables")
  {
    const char* szVarName = "EZ_TEST_HELLO_WORLD";
    EZ_TEST_BOOL(!ezEnvironmentVariableUtils::IsVariableSet(szVarName));

    ezEnvironmentVariableUtils::SetValueString(szVarName, "TEST").IgnoreResult();

    EZ_TEST_BOOL(ezEnvironmentVariableUtils::IsVariableSet(szVarName));

    ezEnvironmentVariableUtils::UnsetVariable(szVarName).IgnoreResult();
    EZ_TEST_BOOL(!ezEnvironmentVariableUtils::IsVariableSet(szVarName));
  }
}
