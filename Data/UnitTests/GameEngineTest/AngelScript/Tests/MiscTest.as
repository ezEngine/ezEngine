#include "TestFramework.as"

enum Phase
{
    CVar,
    Array,
    Prefabs,
    Done
}

class ScriptObject : ezAngelScriptTestClass
{
    private Phase m_Phase = Phase::CVar;

    ScriptObject()
    {
        super("MiscTest");
    }

    bool ExecuteTests()
    {
        if (m_Phase == Phase::CVar)
        {
            bool bOrgValue = ezCVar::GetBoolValue("App.ShowFPS");

            bool bShowFPS1 = ezCVar::GetBoolValue("App.ShowFPS");
            bool bShowFPS2 = ezCVar::GetValue_asBool("App.ShowFPS");
            EZ_TEST_BOOL(bShowFPS1 == bShowFPS2);

            ezCVar::SetBoolValue("App.ShowFPS", !bOrgValue);
            
            bShowFPS1 = ezCVar::GetBoolValue("App.ShowFPS");
            bShowFPS2 = ezCVar::GetValue_asBool("App.ShowFPS");
            EZ_TEST_BOOL(bShowFPS1 == bShowFPS2);

            ezCVar::SetBoolValue("App.ShowFPS", false);

            m_Phase = Phase::Array;
        }
        else if (m_Phase == Phase::Array)
        {
            array<ezInt32> a;
            EZ_TEST_BOOL(a.IsEmpty());
            a.SetCount(32);
            EZ_TEST_INT(a.GetCount(), 32);
            EZ_TEST_BOOL(!a.IsEmpty());
            
            for (ezUInt32 i = 0; i < 32; ++i)
            {
                EZ_TEST_INT(a[i], 0);
                a[i] = i + 1;
                EZ_TEST_INT(a[i], i + 1);
            }

            EZ_TEST_INT(a.PeekBack(), 32);

            EZ_TEST_BOOL(a.Contains(11));
            EZ_TEST_BOOL(!a.Contains(0));
            EZ_TEST_BOOL(!a.Contains(33));

            array<ezInt32> b = a;
            EZ_TEST_BOOL(a == b);

            a.ExpandAndGetRef() = 33;
            EZ_TEST_INT(a.GetCount(), 33);
            EZ_TEST_INT(a[32], 33);

            EZ_TEST_INT(a.IndexOf(13), 12);

            EZ_TEST_BOOL(a != b);
            b.ExpandAndGetRef() = 33;
            EZ_TEST_BOOL(a == b);

            a.PopBack();
            EZ_TEST_BOOL(a != b);
            b.PopBack();
            EZ_TEST_BOOL(a == b);

            a.Reverse();

            for (ezUInt32 i = 0; i < 32; ++i)
            {
                EZ_TEST_INT(a[i], 32 - i);
            }

            EZ_TEST_BOOL(a != b);
            a.Sort();
            EZ_TEST_BOOL(a == b);

            a.Clear();
            EZ_TEST_BOOL(a.IsEmpty());
            
            b.SetCount(0);
            EZ_TEST_BOOL(b.IsEmpty());
            EZ_TEST_BOOL(a == b);

            m_Phase = Phase::Prefabs;
        }
        else if (m_Phase == Phase::Prefabs)
        {
            ezTransform localTransform = ezTransform::Make(ezVec3(1, 2, 3));
            ezPrefabs::SpawnPrefab("{ a3ce5d3d-be5e-4bda-8820-b1ce3b3d33fd }", ezTransform::MakeGlobalTransform(GetOwner().GetGlobalTransform(), localTransform));

            localTransform.m_vPosition = ezVec3(2, 3, 4);
            ezPrefabs::SpawnPrefabAsChild("{ 42e938fb-5523-4606-8e64-6fee83dd0c7b }", GetOwner(), localTransform);
            
            m_Phase = Phase::Done;
        }

        return m_Phase != Phase::Done;
    }
}
