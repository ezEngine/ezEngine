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

    // Bigger pod value type
    {
        ezVec4 test(1.0f, 2.0f, 3.0f, 4.0f);
        ezVec4 test2(5.0f, 6.0f, 7.0f, 8.0f);
        array<ezVec4> elements;

        EZ_TEST_BOOL(elements.IsEmpty());
        EZ_TEST_BOOL(elements.GetCount() == 0);
        EZ_TEST_BOOL(!elements.Contains(test));
        EZ_TEST_BOOL(!elements.Contains(test2));
        EZ_TEST_INT(elements.IndexOf(test), -1);
        EZ_TEST_INT(elements.IndexOf(test2), -1);

        elements.PushBack(test);
        EZ_TEST_BOOL(!elements.IsEmpty());
        EZ_TEST_BOOL(elements.GetCount() == 1);
        EZ_TEST_BOOL(elements.Contains(test));
        EZ_TEST_INT(elements.IndexOf(test), 0);
        EZ_TEST_BOOL(elements[0] == test);
        EZ_TEST_BOOL(elements[0] != test2);

        elements.PushBack(test2);
        EZ_TEST_BOOL(!elements.IsEmpty());
        EZ_TEST_BOOL(elements.GetCount() == 2);
        EZ_TEST_BOOL(elements.Contains(test));
        EZ_TEST_BOOL(elements.Contains(test2));
        EZ_TEST_INT(elements.IndexOf(test), 0);
        EZ_TEST_INT(elements.IndexOf(test2), 1);
        EZ_TEST_BOOL(elements[0] == test);
        EZ_TEST_BOOL(elements[1] == test2);
        EZ_TEST_BOOL(elements[0] != test2);
        EZ_TEST_BOOL(elements[1] != test);

        elements.PushBack(ezVec4(9.0f, 10.0f, 11.0f, 12.0f));
        EZ_TEST_BOOL(elements.Contains(ezVec4(9.0f, 10.0f, 11.0f, 12.0f)));
        EZ_TEST_BOOL(elements[2] == ezVec4(9.0f, 10.0f, 11.0f, 12.0f));
    }

    // non-pod value type
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