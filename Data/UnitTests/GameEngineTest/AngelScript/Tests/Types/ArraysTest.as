#include "../TestFramework.as"

void ExecuteTests()
{
    // typedef to basic type
    {
        ezUInt32 test = 5;
        array<ezUInt32> elements;

        EZ_TEST_BOOL(elements.IsEmpty());
        EZ_TEST_BOOL(elements.GetCount() == 0);
        EZ_TEST_BOOL(!elements.Contains(test));
        EZ_TEST_INT(elements.IndexOf(test), -1);
        elements.PushBack(test);
        EZ_TEST_BOOL(!elements.IsEmpty());
        EZ_TEST_BOOL(elements.GetCount() == 1);
        EZ_TEST_BOOL(elements.Contains(test));
        EZ_TEST_INT(elements.IndexOf(test), 0);
        EZ_TEST_BOOL(elements[0] == test);
    }

    // pod value type
    {
        ezGameObjectHandle test;
        array<ezGameObjectHandle> elements;

        EZ_TEST_BOOL(elements.IsEmpty());
        EZ_TEST_BOOL(elements.GetCount() == 0);
        EZ_TEST_BOOL(!elements.Contains(test));
        EZ_TEST_INT(elements.IndexOf(test), -1);
        elements.PushBack(test);
        EZ_TEST_BOOL(!elements.IsEmpty());
        EZ_TEST_BOOL(elements.GetCount() == 1);
        EZ_TEST_BOOL(elements.Contains(test));
        EZ_TEST_INT(elements.IndexOf(test), 0);
        EZ_TEST_BOOL(elements[0] == test);
    }

    // non pod value type
    {
        ezString test = "Test";
        array<ezString> elements;

        EZ_TEST_BOOL(elements.IsEmpty());
        EZ_TEST_BOOL(elements.GetCount() == 0);
        EZ_TEST_BOOL(!elements.Contains(test));
        EZ_TEST_INT(elements.IndexOf(test), -1);
        elements.PushBack(test);
        EZ_TEST_BOOL(!elements.IsEmpty());
        EZ_TEST_BOOL(elements.GetCount() == 1);
        EZ_TEST_BOOL(elements.Contains(test));
        EZ_TEST_INT(elements.IndexOf(test), 0);
        EZ_TEST_BOOL(elements[0] == test);
    }

    // ezStringView
    {
        ezStringView test = "Test";
        array<ezStringView> elements;

        EZ_TEST_BOOL(elements.IsEmpty());
        EZ_TEST_BOOL(elements.GetCount() == 0);
        EZ_TEST_BOOL(!elements.Contains(test));
        EZ_TEST_INT(elements.IndexOf(test), -1);
        elements.PushBack(test);
        EZ_TEST_BOOL(!elements.IsEmpty());
        EZ_TEST_BOOL(elements.GetCount() == 1);
        EZ_TEST_BOOL(elements.Contains(test));
        EZ_TEST_INT(elements.IndexOf(test), 0);
        EZ_TEST_BOOL(elements[0] == test);
    }
}