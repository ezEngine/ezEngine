module DTest.TestFramework;
import ez.Foundation.Basics;
import core.runtime;

extern(C++) 
{
	class ezSimpleTestGroup
	{
	public:
		alias SimpleTestFunc = void function();
		final void AddSimpleTest(const(char)* szName, SimpleTestFunc TestFunc);
	}

	ezSimpleTestGroup ezCreateSimpleTestGroup(const(char)* szName);
	void ezDestroySimpleTestGroup(ezSimpleTestGroup pTestGroup);

	class ezRegisterSimpleTestHelper
	{
		// Opaque class, not needed on D side
	}

	ezRegisterSimpleTestHelper ezCreateRegisterSimpleTestHelper(ezSimpleTestGroup pTestGroup, const(char)* szTestName, ezSimpleTestGroup.SimpleTestFunc Func);
	void ezDestroyRegisterSimpleTestHelper(ezRegisterSimpleTestHelper pHelper);
}

template Resolve(alias T)
{
	alias Resolve = T;
}

template WrapTestFunction(alias func)
{
	extern(C++) void WrapTestFunction()
	{
		try
		{
		  func();
		}
		catch(Exception ex)
		{
      ezTestFramework.Error("D Exception".ptr, ex.file.ptr, cast(int)ex.line, func.stringof.ptr, "%.*s".ptr, ex.msg.length, ex.msg.ptr);
		}
		catch(Error ex)
		{
      ezTestFramework.Error("D Error".ptr, ex.file.ptr, cast(int)ex.line, func.stringof.ptr, "%.*s".ptr, ex.msg.length, ex.msg.ptr);
		}
	}
}

mixin template TestGroup(string GroupName)
{
	public import ez.Script.Uda;

	__gshared ezSimpleTestGroup _TestGroup;
	__gshared ezRegisterSimpleTestHelper[] _RegisterTestHelpers;

	shared static this()
	{
		_TestGroup = ezCreateSimpleTestGroup((GroupName ~ "\0").ptr);
		
		alias mod = Resolve!(__traits(parent, _TestGroup));
		foreach(memberName; __traits(allMembers, mod))
		{
      static if(memberName[0] != '_')
      {
			  alias member = Resolve!(__traits(getMember, mod, memberName));
			  static if(__traits(compiles, typeof(member)))
			  {
				  static if(hasAttribute!(member, Test))
				  {
					  _RegisterTestHelpers ~= ezCreateRegisterSimpleTestHelper(_TestGroup, getAttribute!(member, Test).name ~ "\0", &WrapTestFunction!member);
				  }
			  }
      }
		}
	}
}

// UDA
struct Test 
{ 
	string name;
}

enum ezTestBlock : bool
{
  Disabled = false,
  Enabled = true
}

extern(C++) struct ezTestOutput
{
  /// \brief Defines the type of output message for ezTestOutputMessage.
  enum Enum
  {
    InvalidType = -1,
    StartOutput = 0,
    BeginBlock,
    EndBlock,
    ImportantInfo,
    Details,
    Success,
    Message,
    Error,
    Duration,
    FinalResult,
    AllOutputTypes
  }
}

extern(C++) class ezTestFramework
{
  static void Output(ezTestOutput.Enum Type, const(char)* szMsg, ...);
  static void Error(const(char)* szError, const(char)* szFile, ezInt32 iLine, const(char)* szFunction, const(char)* szMsg, ...);
}

extern(C++) void ezSetTestBlockName(const(char)* szTestBlockName);
extern(C++) void ezIncreaseAssertCount();

bool EZ_TEST_BLOCK(ezTestBlock enabled, string name)
{
  __gshared char[1024] buffer;
  if(enabled)
  {
    buffer[0..name.length] = name[];
    buffer[name.length] = '\0';
    ezSetTestBlockName(buffer.ptr);
    return true;
  }
  ezSetTestBlockName("\0".ptr);
  ezTestFramework.Output(ezTestOutput.Enum.Message, "Skipped Test Block '%.*s'", name.length, name.ptr);
  return false;
}

void EZ_TEST_BOOL(bool condition, string file = __FILE__, size_t line = __LINE__, string func = __PRETTY_FUNCTION__)
{
  ezIncreaseAssertCount();
  if(!condition)
  {
    ezTestFramework.Error("Bool condition failed".ptr, file.ptr, cast(int)line, func.ptr, "".ptr);
  }
}

struct TestBoolMsgContext
{
  string file;
  size_t line;
  string func;

  this(string file, size_t line, string func)
  {
    this.file = file;
    this.line = line;
    this.func = func;
  }

  void opCall(ARGS...)(bool condition, const(char)[] msg, ARGS args)
  {
    ezIncreaseAssertCount();
    if(!condition)
    {
      ezTestFramework.Error("Bool condition failed".ptr, file.ptr, cast(int)line, func.ptr, (msg ~ "\0").ptr, args);
    }
  }
}

TestBoolMsgContext EZ_TEST_BOOL_MSG(string file = __FILE__, size_t line = __LINE__, string func = __PRETTY_FUNCTION__)
{
  return TestBoolMsgContext(file, line, func);
}

void EZ_TEST_INT(T)(T i1, T i2, string file = __FILE__, size_t line = __LINE__, string func = __PRETTY_FUNCTION__)
{
  ezIncreaseAssertCount();
  if(i1 != i2)
  {
    import std.format : format;
    ezTestFramework.Error(format("Failure: %s does not equal %s\0", i1, i2).ptr, file.ptr, cast(int)line, func.ptr, "".ptr);
  }
}