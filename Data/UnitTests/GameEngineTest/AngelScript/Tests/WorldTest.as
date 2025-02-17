#include "TestFramework.as"

enum Phase
{
    CreateObj,
    CheckExists,
    CheckDeleted,
    Done,
}

class ScriptObject : ezAngelScriptTestClass
{
    private Phase m_Phase = Phase::CreateObj;
    private ezGameObjectHandle m_hCreated;
    private ezTime m_LastUpdate;

    ScriptObject()
    {
        super("WorldTest");
    }
    
    bool ExecuteTests()
    {
        ezGameObject@ owner = GetOwner();
        ezWorld@ world = GetWorld();

        ezGameObject@ allTestsObj;
        EZ_TEST_BOOL(world.TryGetObjectWithGlobalKey("Tests", @allTestsObj));
        EZ_TEST_STRING(allTestsObj.GetName(), "All Tests");

        ezClock@ clock = world.GetClock();
        ezTime tNow = clock.GetAccumulatedTime();
        EZ_TEST_BOOL(tNow > m_LastUpdate);
        m_LastUpdate = tNow;

        if (m_Phase == Phase::CreateObj)
        {
            ezGameObjectDesc gd;
            gd.m_LocalPosition.Set(1, 2, 3);
            gd.m_sName = "TestObj";
            gd.m_hParent = owner.GetHandle();
            
            ezGameObject@ go;
            m_hCreated = GetWorld().CreateObject(gd, go);
            EZ_TEST_BOOL(world.IsValidObject(m_hCreated));

            m_Phase = Phase::CheckExists;
            return true;
        }

        if (m_Phase == Phase::CheckExists)
        {
            ezGameObject@ obj1 = owner.FindChildByName("TestObj");
            EZ_TEST_BOOL(@obj1 != null);
            EZ_TEST_BOOL(world.IsValidObject(m_hCreated));
            EZ_TEST_BOOL(obj1.GetHandle() == m_hCreated);

            ezGameObject@ obj2;
            EZ_TEST_BOOL(world.TryGetObject(m_hCreated, @obj2));

            EZ_TEST_BOOL(@obj1 == @obj2);

            world.DeleteObjectDelayed(obj1.GetHandle());

            EZ_TEST_STRING(obj2.GetName(), "TestObj");

            m_Phase = Phase::CheckDeleted;
            return true;            
        }

        if (m_Phase == Phase::CheckDeleted)
        {
            ezGameObject@ obj = owner.FindChildByName("TestObj");
            EZ_TEST_BOOL(@obj == null);

            EZ_TEST_BOOL(!world.IsValidObject(m_hCreated));

            ezGameObject@ obj2;
            EZ_TEST_BOOL(!world.TryGetObject(m_hCreated, @obj2));

            m_Phase = Phase::Done;
            return false;
        }

        return false;
    }
}

