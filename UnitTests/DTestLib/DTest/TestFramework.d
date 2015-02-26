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

mixin template TestGroup(string GroupName)
{

}

// UDA
struct Test 
{ 
	string name;
}