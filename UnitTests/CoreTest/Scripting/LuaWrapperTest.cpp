#include <PCH.h>
#include <CoreUtils/Scripting/LuaWrapper.h>

EZ_CREATE_SIMPLE_TEST_GROUP(Scripting);

static const char* g_Script = "\
globaltable = true;\n\
function f_globaltable()\n\
end\n\
function f_NotWorking()\n\
  DoNothing();\n\
end\n\
intvar1 = 4;\n\
intvar2 = 7;\n\
floatvar1 = 4.3;\n\
floatvar2 = 7.3;\n\
boolvar1 = true;\n\
boolvar2 = false;\n\
stringvar1 = \"zweiundvierzig\";\n\
stringvar2 = \"OhWhatsInHere\";\n\
\n\
\n\
function f1()\n\
end\n\
\n\
function f2()\n\
end\n\
\n\
MyTable =\n\
{\n\
  table1 = true;\n\
  \n\
  f_table1 = function()\n\
  end;\n\
  intvar1 = 14;\n\
  intvar2 = 17;\n\
  floatvar1 = 14.3;\n\
  floatvar2 = 17.3;\n\
  boolvar1 = false;\n\
  boolvar2 = true;\n\
  stringvar1 = \"+zweiundvierzig\";\n\
  stringvar2 = \"+OhWhatsInHere\";\n\
  \n\
  SubTable =\n\
  {\n\
    table2 = true;\n\
    f_table2 = function()\n\
    end;\n\
    intvar1 = 24;\n\
  };\n\
};\n\
\n\
";

class ScriptLog : public ezLogInterface
{
public:

  virtual void HandleLogMessage(const ezLoggingEventData& le) override
  {
    EZ_TEST_FAILURE("Script Error", le.m_szText);
    EZ_TEST_DEBUG_BREAK;
  }
};

class ScriptLogIgnore : public ezLogInterface
{
public:
  static ezInt32 g_iErrors;

  virtual void HandleLogMessage(const ezLoggingEventData& le) override
  {
    switch (le.m_EventType)
    {
    case ezLogMsgType::ErrorMsg:
    case ezLogMsgType::SeriousWarningMsg:
    case ezLogMsgType::WarningMsg:
      ++g_iErrors;
    default:
      break;
    }
  }
};

ezInt32 ScriptLogIgnore::g_iErrors = 0;

int MyFunc1(lua_State* state)
{
  ezLuaWrapper s(state);

  EZ_TEST_INT(s.GetNumberOfFunctionParameters(), 0);

  return s.ReturnToScript();
}

int MyFunc2(lua_State* state)
{
  ezLuaWrapper s(state);

  EZ_TEST_INT(s.GetNumberOfFunctionParameters(), 6);
  EZ_TEST_BOOL(s.IsParameterBool(0));
  EZ_TEST_BOOL(s.IsParameterFloat(1));
  EZ_TEST_BOOL(s.IsParameterInt(2));
  EZ_TEST_BOOL(s.IsParameterNil(3));
  EZ_TEST_BOOL(s.IsParameterString(4));
  EZ_TEST_BOOL(s.IsParameterString(5));

  EZ_TEST_BOOL(s.GetBoolParameter(0) == true);
  EZ_TEST_FLOAT(s.GetFloatParameter(1), 2.3f, 0.0001f);
  EZ_TEST_INT(s.GetIntParameter(2), 42);
  EZ_TEST_STRING(s.GetStringParameter(4), "test");
  EZ_TEST_STRING(s.GetStringParameter(5), "tut");

  return s.ReturnToScript();
}

int MyFunc3(lua_State* state)
{
  ezLuaWrapper s(state);

  EZ_TEST_INT(s.GetNumberOfFunctionParameters(), 0);

  s.PushReturnValue(false);
  s.PushReturnValue(2.3f);
  s.PushReturnValue(42);
  s.PushReturnValueNil();
  s.PushReturnValue("test");
  s.PushReturnValue("tuttut", 3);

  return s.ReturnToScript();
}

int MyFunc4(lua_State* state)
{
  ezLuaWrapper s(state);

  EZ_TEST_INT(s.GetNumberOfFunctionParameters(), 1);

  EZ_TEST_BOOL(s.IsParameterTable(0));

  EZ_TEST_BOOL(s.OpenTableFromParameter(0) == EZ_SUCCESS);

  EZ_TEST_BOOL(s.IsVariableAvailable("table1") == true);

  s.CloseAllTables();

  return s.ReturnToScript();
}

EZ_CREATE_SIMPLE_TEST(Scripting, LuaWrapper)
{
  ScriptLog Log;
  ScriptLogIgnore LogIgnore;

  ezLuaWrapper sMain;
  EZ_TEST_BOOL(sMain.ExecuteString(g_Script, "MainScript", &Log) == EZ_SUCCESS);

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ExecuteString")
  {
    ezLuaWrapper s;
    ScriptLogIgnore::g_iErrors = 0;

    EZ_TEST_BOOL(s.ExecuteString(" pups ", "FailToCompile", &LogIgnore) == EZ_FAILURE);
    EZ_TEST_INT(ScriptLogIgnore::g_iErrors, 1);

    EZ_TEST_BOOL(s.ExecuteString(" pups(); ", "FailToExecute", &LogIgnore) == EZ_FAILURE);
    EZ_TEST_INT(ScriptLogIgnore::g_iErrors, 2);

    EZ_TEST_BOOL(s.ExecuteString(g_Script, "MainScript", &Log) == EZ_SUCCESS);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Clear")
  {
    ezLuaWrapper s;
    EZ_TEST_BOOL(s.ExecuteString(g_Script, "MainScript", &Log) == EZ_SUCCESS);

    EZ_TEST_BOOL(s.IsVariableAvailable("globaltable") == true);
    EZ_TEST_BOOL(s.IsVariableAvailable("boolvar1") == true);
    EZ_TEST_BOOL(s.IsVariableAvailable("boolvar2") == true);
    EZ_TEST_BOOL(s.IsVariableAvailable("intvar1") == true);
    EZ_TEST_BOOL(s.IsVariableAvailable("intvar2") == true);
    EZ_TEST_BOOL(s.IsVariableAvailable("floatvar1") == true);
    EZ_TEST_BOOL(s.IsVariableAvailable("floatvar2") == true);
    EZ_TEST_BOOL(s.IsVariableAvailable("stringvar1") == true);
    EZ_TEST_BOOL(s.IsVariableAvailable("stringvar2") == true);

    s.Clear();

    // after clearing the script, these variables should not be available anymore
    EZ_TEST_BOOL(s.IsVariableAvailable("globaltable") == false);
    EZ_TEST_BOOL(s.IsVariableAvailable("boolvar1") == false);
    EZ_TEST_BOOL(s.IsVariableAvailable("boolvar2") == false);
    EZ_TEST_BOOL(s.IsVariableAvailable("intvar1") == false);
    EZ_TEST_BOOL(s.IsVariableAvailable("intvar2") == false);
    EZ_TEST_BOOL(s.IsVariableAvailable("floatvar1") == false);
    EZ_TEST_BOOL(s.IsVariableAvailable("floatvar2") == false);
    EZ_TEST_BOOL(s.IsVariableAvailable("stringvar1") == false);
    EZ_TEST_BOOL(s.IsVariableAvailable("stringvar2") == false);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsVariableAvailable (Global)")
  {
    EZ_TEST_BOOL(sMain.IsVariableAvailable("globaltable") == true);
    EZ_TEST_BOOL(sMain.IsVariableAvailable("nonexisting1") == false);
    EZ_TEST_BOOL(sMain.IsVariableAvailable("boolvar1") == true);
    EZ_TEST_BOOL(sMain.IsVariableAvailable("boolvar2") == true);
    EZ_TEST_BOOL(sMain.IsVariableAvailable("nonexisting2") == false);
    EZ_TEST_BOOL(sMain.IsVariableAvailable("intvar1") == true);
    EZ_TEST_BOOL(sMain.IsVariableAvailable("intvar2") == true);
    EZ_TEST_BOOL(sMain.IsVariableAvailable("nonexisting3") == false);
    EZ_TEST_BOOL(sMain.IsVariableAvailable("floatvar1") == true);
    EZ_TEST_BOOL(sMain.IsVariableAvailable("floatvar2") == true);
    EZ_TEST_BOOL(sMain.IsVariableAvailable("nonexisting4") == false);
    EZ_TEST_BOOL(sMain.IsVariableAvailable("stringvar1") == true);
    EZ_TEST_BOOL(sMain.IsVariableAvailable("stringvar2") == true);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "IsFunctionAvailable (Global)")
  {
    EZ_TEST_BOOL(sMain.IsFunctionAvailable("nonexisting1") == false);
    EZ_TEST_BOOL(sMain.IsFunctionAvailable("f1") == true);
    EZ_TEST_BOOL(sMain.IsFunctionAvailable("f2") == true);
    EZ_TEST_BOOL(sMain.IsFunctionAvailable("nonexisting2") == false);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetIntVariable (Global)")
  {
    EZ_TEST_INT(sMain.GetIntVariable("nonexisting1", 13), 13);
    EZ_TEST_INT(sMain.GetIntVariable("intvar1", 13), 4);
    EZ_TEST_INT(sMain.GetIntVariable("intvar2", 13), 7);
    EZ_TEST_INT(sMain.GetIntVariable("nonexisting2", 14), 14);
    EZ_TEST_INT(sMain.GetIntVariable("intvar1", 13), 4);
    EZ_TEST_INT(sMain.GetIntVariable("intvar2", 13), 7);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetIntVariable (Table)")
  {
    EZ_TEST_BOOL(sMain.OpenTable("MyTable") == EZ_SUCCESS);

    EZ_TEST_INT(sMain.GetIntVariable("nonexisting1", 13), 13);
    EZ_TEST_INT(sMain.GetIntVariable("intvar1", 13), 14);
    EZ_TEST_INT(sMain.GetIntVariable("intvar2", 13), 17);
    EZ_TEST_INT(sMain.GetIntVariable("nonexisting2", 14), 14);
    EZ_TEST_INT(sMain.GetIntVariable("intvar1", 13), 14);
    EZ_TEST_INT(sMain.GetIntVariable("intvar2", 13), 17);

    sMain.CloseTable();
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetFloatVariable (Global)")
  {
    EZ_TEST_INT(sMain.GetFloatVariable("nonexisting1", 13), 13);
    EZ_TEST_INT(sMain.GetFloatVariable("floatvar1", 13), 4.3f);
    EZ_TEST_INT(sMain.GetFloatVariable("floatvar2", 13), 7.3f);
    EZ_TEST_INT(sMain.GetFloatVariable("nonexisting2", 14), 14);
    EZ_TEST_INT(sMain.GetFloatVariable("floatvar1", 13), 4.3f);
    EZ_TEST_INT(sMain.GetFloatVariable("floatvar2", 13), 7.3f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetFloatVariable (Table)")
  {
    EZ_TEST_BOOL(sMain.OpenTable("MyTable") == EZ_SUCCESS);

    EZ_TEST_INT(sMain.GetFloatVariable("nonexisting1", 13), 13);
    EZ_TEST_INT(sMain.GetFloatVariable("floatvar1", 13), 14.3f);
    EZ_TEST_INT(sMain.GetFloatVariable("floatvar2", 13), 17.3f);
    EZ_TEST_INT(sMain.GetFloatVariable("nonexisting2", 14), 14);
    EZ_TEST_INT(sMain.GetFloatVariable("floatvar1", 13), 14.3f);
    EZ_TEST_INT(sMain.GetFloatVariable("floatvar2", 13), 17.3f);

    sMain.CloseTable();
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetBoolVariable (Global)")
  {
    EZ_TEST_BOOL(sMain.GetBoolVariable("nonexisting1", true) == true);
    EZ_TEST_BOOL(sMain.GetBoolVariable("boolvar1", false) == true);
    EZ_TEST_BOOL(sMain.GetBoolVariable("boolvar2", true) == false);
    EZ_TEST_BOOL(sMain.GetBoolVariable("nonexisting2", false) == false);
    EZ_TEST_BOOL(sMain.GetBoolVariable("boolvar1", false) == true);
    EZ_TEST_BOOL(sMain.GetBoolVariable("boolvar2", true) == false);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetBoolVariable (Table)")
  {
    EZ_TEST_BOOL(sMain.OpenTable("MyTable") == EZ_SUCCESS);

    EZ_TEST_BOOL(sMain.GetBoolVariable("nonexisting1", true) == true);
    EZ_TEST_BOOL(sMain.GetBoolVariable("boolvar1", true) == false);
    EZ_TEST_BOOL(sMain.GetBoolVariable("boolvar2", false) == true);
    EZ_TEST_BOOL(sMain.GetBoolVariable("nonexisting2", false) == false);
    EZ_TEST_BOOL(sMain.GetBoolVariable("boolvar1", true) == false);
    EZ_TEST_BOOL(sMain.GetBoolVariable("boolvar2", false) == true);

    sMain.CloseTable();
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetStringVariable (Global)")
  {
    EZ_TEST_STRING(sMain.GetStringVariable("nonexisting1", "a"), "a");
    EZ_TEST_STRING(sMain.GetStringVariable("stringvar1", "a"), "zweiundvierzig");
    EZ_TEST_STRING(sMain.GetStringVariable("stringvar2", "a"), "OhWhatsInHere");
    EZ_TEST_STRING(sMain.GetStringVariable("nonexisting2", "b"), "b");
    EZ_TEST_STRING(sMain.GetStringVariable("stringvar1", "a"), "zweiundvierzig");
    EZ_TEST_STRING(sMain.GetStringVariable("stringvar2", "a"), "OhWhatsInHere");  
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetStringVariable (Table)")
  {
    EZ_TEST_BOOL(sMain.OpenTable("MyTable") == EZ_SUCCESS);

    EZ_TEST_STRING(sMain.GetStringVariable("nonexisting1", "a"), "a");
    EZ_TEST_STRING(sMain.GetStringVariable("stringvar1", "a"), "+zweiundvierzig");
    EZ_TEST_STRING(sMain.GetStringVariable("stringvar2", "a"), "+OhWhatsInHere");
    EZ_TEST_STRING(sMain.GetStringVariable("nonexisting2", "b"), "b");
    EZ_TEST_STRING(sMain.GetStringVariable("stringvar1", "a"), "+zweiundvierzig");
    EZ_TEST_STRING(sMain.GetStringVariable("stringvar2", "a"), "+OhWhatsInHere");

    sMain.CloseTable();
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetVariable (int, Global)")
  {
    EZ_TEST_INT(sMain.GetIntVariable("intvar1", 13), 4);
    sMain.SetVariable("intvar1", 27);
    EZ_TEST_INT(sMain.GetIntVariable("intvar1", 13), 27);
    sMain.SetVariable("intvar1", 4);
    EZ_TEST_INT(sMain.GetIntVariable("intvar1", 13), 4);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetVariable (int, Table)")
  {
    EZ_TEST_BOOL(sMain.OpenTable("MyTable") == EZ_SUCCESS);

    EZ_TEST_INT(sMain.GetIntVariable("intvar1", 13), 14);
    sMain.SetVariable("intvar1", 127);
    EZ_TEST_INT(sMain.GetIntVariable("intvar1", 13), 127);
    sMain.SetVariable("intvar1", 14);
    EZ_TEST_INT(sMain.GetIntVariable("intvar1", 13), 14);

    sMain.CloseTable();
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetVariable (float, Global)")
  {
    EZ_TEST_INT(sMain.GetFloatVariable("floatvar1", 13), 4.3f);
    sMain.SetVariable("floatvar1", 27.3f);
    EZ_TEST_INT(sMain.GetFloatVariable("floatvar1", 13), 27.3f);
    sMain.SetVariable("floatvar1", 4.3f);
    EZ_TEST_INT(sMain.GetFloatVariable("floatvar1", 13), 4.3f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetVariable (float, Table)")
  {
    EZ_TEST_BOOL(sMain.OpenTable("MyTable") == EZ_SUCCESS);

    EZ_TEST_INT(sMain.GetFloatVariable("floatvar1", 13), 14.3f);
    sMain.SetVariable("floatvar1", 127.3f);
    EZ_TEST_INT(sMain.GetFloatVariable("floatvar1", 13), 127.3f);
    sMain.SetVariable("floatvar1", 14.3f);
    EZ_TEST_INT(sMain.GetFloatVariable("floatvar1", 13), 14.3f);

    sMain.CloseTable();
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetVariable (bool, Global)")
  {
    EZ_TEST_INT(sMain.GetBoolVariable("boolvar1", false), true);
    sMain.SetVariable("boolvar1", false);
    EZ_TEST_INT(sMain.GetBoolVariable("boolvar1", true), false);
    sMain.SetVariable("boolvar1", true);
    EZ_TEST_INT(sMain.GetBoolVariable("boolvar1", false), true);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetVariable (bool, Table)")
  {
    EZ_TEST_BOOL(sMain.OpenTable("MyTable") == EZ_SUCCESS);

    EZ_TEST_INT(sMain.GetBoolVariable("boolvar1", true), false);
    sMain.SetVariable("boolvar1", true);
    EZ_TEST_INT(sMain.GetBoolVariable("boolvar1", false), true);
    sMain.SetVariable("boolvar1", false);
    EZ_TEST_INT(sMain.GetBoolVariable("boolvar1", true), false);

    sMain.CloseTable();
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetVariable (string, Global)")
  {
    EZ_TEST_STRING(sMain.GetStringVariable("stringvar1", "bla"), "zweiundvierzig");

    sMain.SetVariable("stringvar1", "test1");
    EZ_TEST_STRING(sMain.GetStringVariable("stringvar1", "bla"), "test1");
    sMain.SetVariable("stringvar1", "zweiundvierzig");
    EZ_TEST_STRING(sMain.GetStringVariable("stringvar1", "bla"), "zweiundvierzig");

    sMain.SetVariable("stringvar1", "test1", 3);
    EZ_TEST_STRING(sMain.GetStringVariable("stringvar1", "bla"), "tes");
    sMain.SetVariable("stringvar1", "zweiundvierzigabc", 14);
    EZ_TEST_STRING(sMain.GetStringVariable("stringvar1", "bla"), "zweiundvierzig");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetVariable (string, Table)")
  {
    EZ_TEST_BOOL(sMain.OpenTable("MyTable") == EZ_SUCCESS);

    EZ_TEST_STRING(sMain.GetStringVariable("stringvar1", "bla"), "+zweiundvierzig");

    sMain.SetVariable("stringvar1", "+test1");
    EZ_TEST_STRING(sMain.GetStringVariable("stringvar1", "bla"), "+test1");
    sMain.SetVariable("stringvar1", "+zweiundvierzig");
    EZ_TEST_STRING(sMain.GetStringVariable("stringvar1", "bla"), "+zweiundvierzig");

    sMain.SetVariable("stringvar1", "+test1", 4);
    EZ_TEST_STRING(sMain.GetStringVariable("stringvar1", "bla"), "+tes");
    sMain.SetVariable("stringvar1", "+zweiundvierzigabc", 15);
    EZ_TEST_STRING(sMain.GetStringVariable("stringvar1", "bla"), "+zweiundvierzig");

    sMain.CloseTable();
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetVariable (nil, Global)")
  {
    EZ_TEST_STRING(sMain.GetStringVariable("stringvar1", "bla"), "zweiundvierzig");

    sMain.SetVariableNil("stringvar1");

    EZ_TEST_BOOL(sMain.IsVariableAvailable("stringvar1") == false); // It is Nil -> 'not available'

    sMain.SetVariable("stringvar1", "zweiundvierzig");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetVariable (nil, Table)")
  {
    EZ_TEST_BOOL(sMain.OpenTable("MyTable") == EZ_SUCCESS);

    EZ_TEST_STRING(sMain.GetStringVariable("stringvar1", "bla"), "+zweiundvierzig");

    sMain.SetVariableNil("stringvar1");

    EZ_TEST_BOOL(sMain.IsVariableAvailable("stringvar1") == false); // It is Nil -> 'not available'

    sMain.SetVariable("stringvar1", "+zweiundvierzig");

    sMain.CloseTable();
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "OpenTable")
  {
    EZ_TEST_BOOL(sMain.IsVariableAvailable("globaltable") == true);
    EZ_TEST_BOOL(sMain.IsVariableAvailable("table1") == false);
    EZ_TEST_BOOL(sMain.IsVariableAvailable("table2") == false);

    EZ_TEST_BOOL(sMain.IsFunctionAvailable("f_globaltable") == true);
    EZ_TEST_BOOL(sMain.IsFunctionAvailable("f_table1") == false);
    EZ_TEST_BOOL(sMain.IsFunctionAvailable("f_table2") == false);

    EZ_TEST_BOOL(sMain.OpenTable("NotMyTable") == EZ_FAILURE);

    EZ_TEST_BOOL(sMain.OpenTable("MyTable") == EZ_SUCCESS);
    {
      EZ_TEST_BOOL(sMain.IsVariableAvailable("globaltable") == false);
      EZ_TEST_BOOL(sMain.IsVariableAvailable("table1") == true);
      EZ_TEST_BOOL(sMain.IsVariableAvailable("table2") == false);

      EZ_TEST_BOOL(sMain.IsFunctionAvailable("f_globaltable") == false);
      EZ_TEST_BOOL(sMain.IsFunctionAvailable("f_table1") == true);
      EZ_TEST_BOOL(sMain.IsFunctionAvailable("f_table2") == false);

      EZ_TEST_BOOL(sMain.OpenTable("NotMyTable") == EZ_FAILURE);

      EZ_TEST_BOOL(sMain.OpenTable("SubTable") == EZ_SUCCESS);
      {
        EZ_TEST_BOOL(sMain.OpenTable("NotMyTable") == EZ_FAILURE);

        EZ_TEST_BOOL(sMain.IsVariableAvailable("globaltable") == false);
        EZ_TEST_BOOL(sMain.IsVariableAvailable("table1") == false);
        EZ_TEST_BOOL(sMain.IsVariableAvailable("table2") == true);

        EZ_TEST_BOOL(sMain.IsFunctionAvailable("f_globaltable") == false);
        EZ_TEST_BOOL(sMain.IsFunctionAvailable("f_table1") == false);
        EZ_TEST_BOOL(sMain.IsFunctionAvailable("f_table2") == true);

        sMain.CloseTable();
      }

      EZ_TEST_BOOL(sMain.IsVariableAvailable("globaltable") == false);
      EZ_TEST_BOOL(sMain.IsVariableAvailable("table1") == true);
      EZ_TEST_BOOL(sMain.IsVariableAvailable("table2") == false);

      EZ_TEST_BOOL(sMain.IsFunctionAvailable("f_globaltable") == false);
      EZ_TEST_BOOL(sMain.IsFunctionAvailable("f_table1") == true);
      EZ_TEST_BOOL(sMain.IsFunctionAvailable("f_table2") == false);

      EZ_TEST_BOOL(sMain.OpenTable("NotMyTable") == EZ_FAILURE);

      EZ_TEST_BOOL(sMain.OpenTable("SubTable") == EZ_SUCCESS);
      {
        EZ_TEST_BOOL(sMain.OpenTable("NotMyTable") == EZ_FAILURE);

        EZ_TEST_BOOL(sMain.IsVariableAvailable("globaltable") == false);
        EZ_TEST_BOOL(sMain.IsVariableAvailable("table1") == false);
        EZ_TEST_BOOL(sMain.IsVariableAvailable("table2") == true);

        EZ_TEST_BOOL(sMain.IsFunctionAvailable("f_globaltable") == false);
        EZ_TEST_BOOL(sMain.IsFunctionAvailable("f_table1") == false);
        EZ_TEST_BOOL(sMain.IsFunctionAvailable("f_table2") == true);

        sMain.CloseTable();
      }

      sMain.CloseTable();
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "RegisterCFunction")
  {
    EZ_TEST_BOOL(sMain.IsFunctionAvailable("Func1") == false);

    sMain.RegisterCFunction("Func1", MyFunc1);

    EZ_TEST_BOOL(sMain.IsFunctionAvailable("Func1") == true);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Call Lua Function")
  {
    EZ_TEST_BOOL(sMain.PrepareFunctionCall("NotExisting") == false);

    EZ_TEST_BOOL(sMain.PrepareFunctionCall("f_globaltable") == true);
    EZ_TEST_BOOL(sMain.CallPreparedFunction(0, &Log) == EZ_SUCCESS);

    ScriptLogIgnore::g_iErrors = 0;
    EZ_TEST_BOOL(sMain.PrepareFunctionCall("f_NotWorking") == true);
    EZ_TEST_BOOL(sMain.CallPreparedFunction(0, &LogIgnore) == EZ_FAILURE);
    EZ_TEST_INT(ScriptLogIgnore::g_iErrors, 1);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Call C Function")
  {
    EZ_TEST_BOOL(sMain.PrepareFunctionCall("NotExisting") == false);

    if (sMain.IsFunctionAvailable("Func1") == false)
      sMain.RegisterCFunction("Func1", MyFunc1);

    EZ_TEST_BOOL(sMain.PrepareFunctionCall("Func1") == true);

    EZ_TEST_BOOL(sMain.CallPreparedFunction(0, &Log) == EZ_SUCCESS);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Call C Function with Parameters")
  {
    if (sMain.IsFunctionAvailable("Func2") == false)
      sMain.RegisterCFunction("Func2", MyFunc2);

    EZ_TEST_BOOL(sMain.PrepareFunctionCall("Func2") == true);

    sMain.PushParameter(true);
    sMain.PushParameter(2.3f);
    sMain.PushParameter(42);
    sMain.PushParameterNil();
    sMain.PushParameter("test");
    sMain.PushParameter("tuttut", 3);

    EZ_TEST_BOOL(sMain.CallPreparedFunction(0, &Log) == EZ_SUCCESS);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Call C Function with Return Values")
  {
    if (sMain.IsFunctionAvailable("Func3") == false)
      sMain.RegisterCFunction("Func3", MyFunc3);

    EZ_TEST_BOOL(sMain.PrepareFunctionCall("Func3") == true);
    EZ_TEST_BOOL(sMain.CallPreparedFunction(6, &Log) == EZ_SUCCESS);

    EZ_TEST_BOOL(sMain.IsReturnValueBool(0));
    EZ_TEST_BOOL(sMain.IsReturnValueFloat(1));
    EZ_TEST_BOOL(sMain.IsReturnValueInt(2));
    EZ_TEST_BOOL(sMain.IsReturnValueNil(3));
    EZ_TEST_BOOL(sMain.IsReturnValueString(4));
    EZ_TEST_BOOL(sMain.IsReturnValueString(5));

    EZ_TEST_BOOL(sMain.GetBoolReturnValue(0) == false);
    EZ_TEST_FLOAT(sMain.GetFloatReturnValue(1), 2.3f, 0.0001f);
    EZ_TEST_INT(sMain.GetIntReturnValue(2), 42);
    EZ_TEST_STRING(sMain.GetStringReturnValue(4), "test");
    EZ_TEST_STRING(sMain.GetStringReturnValue(5), "tut");

    sMain.DiscardReturnValues();

    EZ_TEST_BOOL(sMain.PrepareFunctionCall("Func3") == true);
    EZ_TEST_BOOL(sMain.CallPreparedFunction(6, &Log) == EZ_SUCCESS);

    sMain.DiscardReturnValues();
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Call C Function with Table Parameter")
  {
    if (sMain.IsFunctionAvailable("Func4") == false)
      sMain.RegisterCFunction("Func4", MyFunc4);

    EZ_TEST_BOOL(sMain.PrepareFunctionCall("Func4") == true);

    sMain.PushTable("MyTable", true);

    EZ_TEST_BOOL(sMain.CallPreparedFunction(0, &Log) == EZ_SUCCESS);
  }
}

