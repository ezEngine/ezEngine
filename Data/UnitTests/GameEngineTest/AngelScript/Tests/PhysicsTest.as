#include "TestFramework.as"

enum Phase
{
    Raycast,
    Spatial,
    Done
}

class ScriptObject : ezAngelScriptTestClass
{
    private Phase m_Phase = Phase::Raycast;
    private int m_iFoundInside = 0;
    private int m_iFoundOther = 0;
    private array<ezGameObjectHandle> m_Found;

    ScriptObject()
    {
        super("PhysicsTest");
    }

    bool FoundObject(ezGameObject@ obj)
    {
        if (obj.HasName("Inside"))
            ++m_iFoundInside;
        else
            ++m_iFoundOther;

        m_Found.PushBack(obj.GetHandle());

        return true;
    }

    bool ExecuteTests()
    {
        if (m_Phase == Phase::Raycast)
        {
            ezVec3 vHitPosition, vHitNormal;
            ezGameObjectHandle hHitObject;

            EZ_TEST_BOOL(!ezPhysics::Raycast(vHitPosition, vHitNormal, hHitObject, ezVec3(1, 2, 3), ezVec3(0, 0, -1) * 10.0f, 0, ezPhysicsShapeType::Dynamic));

            EZ_TEST_BOOL(ezPhysics::Raycast(vHitPosition, vHitNormal, hHitObject, ezVec3(1, 2, 3), ezVec3(0, 0, -1) * 10.0f, 0, ezPhysicsShapeType::Static));

            EZ_TEST_VEC3(vHitPosition, ezVec3(1, 2, 0));
            EZ_TEST_VEC3(vHitNormal, ezVec3(0, 0, 1));

            m_Phase = Phase::Spatial;
        }
        else if (m_Phase == Phase::Spatial)
        {
            ezSpatial::FindObjectsInSphere("Marker", ezVec3(5.5f, 5.5f, 5.0f), 1.0f, ReportObjectCB(FoundObject));

            EZ_TEST_INT(m_iFoundInside, 4);
            EZ_TEST_INT(m_iFoundOther, 0);
            EZ_TEST_INT(m_Found.GetCount(), 4);

            m_Phase = Phase::Done;
        }

        return m_Phase != Phase::Done;
    }
}
