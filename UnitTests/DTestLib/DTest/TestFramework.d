module DTest.TestFramework;
import core.runtime;

extern(C++) 
{
	class ezSimpleTestGroup
	{
	public:
		alias SimpleTestFunc = void function();
		void AddSimpleTest(const char* szName, SimpleTestFunc TestFunc);
	}

	ezSimpleTestGroup ezCreateSimpleTestGroup(const char* szName);
	void ezDestroySimpleTestGroup(ezSimpleTestGroup pTestGroup);

	class ezRegisterSimpleTestHelper
	{
		// Opaque class, not needed on D side
	}

	ezRegisterSimpleTestHelper ezCreateRegisterSimpleTestHelper(ezSimpleTestGroup pTestGroup, const char* szTestName, ezSimpleTestGroup.SimpleTestFunc Func);
	void ezDestroyRegisterSimpleTestHelper(ezRegisterSimpleTestHelper pHelper);

	void ezInitDTests()
	{
		Runtime.initialize();
	}

	void ezDeinitDTests()
	{
		Runtime.terminate();
	}
}

template Resolve(alias T)
{
	alias Resolve = T;
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
			alias member = Resolve!(__traits(getMember, mod, memberName));
			static if(__traits(compiles, typeof(member)))
			{
				static if(hasAttribute!(member, Test))
				{
					_RegisterTestHelpers ~= ezCreateRegisterSimpleTestHelper(_TestGroup, getAttribute!(member, Test).name ~ "\0", &member);
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