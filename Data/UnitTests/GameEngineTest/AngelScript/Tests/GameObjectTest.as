#include "TestFramework.as"

enum Phase
{
    Init,
    Render,

    Delete,
    CheckDelete,

    Done
}

class ScriptObject : ezAngelScriptTestClass
{
    ezTime RenderDelay = ezTime::Seconds(0.2);

    private Phase m_Phase = Phase::Init;
    private ezGameObjectHandle m_hChild1;
    private ezGameObjectHandle m_hChild2;
    private ezComponentHandle m_hFog;
    private ezComponentHandle m_hLight;
    private ezComponentHandle m_hBlackboard;

    private ezTime m_tStart;

    ScriptObject()
    {
        super("GameObjectTest");
    }

    bool ExecuteTests()
    {
        if (m_Phase == Phase::Init)
        {
            EZ_TEST_BOOL(m_hChild1.IsInvalidated());
            EZ_TEST_BOOL(m_hChild2.IsInvalidated());

            ezGameObject@ obj1 = GetOwner().FindChildByName("Child1");
            m_hChild1 = obj1.GetHandle();

            ezGameObject@ obj2 = GetOwner().FindChildByPath("Child2");
            m_hChild2 = obj2.GetHandle();

            EZ_TEST_BOOL(!m_hChild1.IsInvalidated());
            EZ_TEST_BOOL(!m_hChild2.IsInvalidated());
            EZ_TEST_BOOL(m_hChild1 == m_hChild1);
            EZ_TEST_BOOL(m_hChild2 == m_hChild2);
            EZ_TEST_BOOL(m_hChild1 != m_hChild2);

            EZ_TEST_BOOL(obj1.GetParent().HasName("GameObject Test"));
            EZ_TEST_BOOL(@obj1.GetWorld() == @GetWorld());

            EZ_TEST_BOOL(!obj1.HasTag("Dummy"));
            EZ_TEST_BOOL(obj2.HasTag("Dummy"));

            obj1.MakeDynamic();

            ezFogComponent@ fog;
            EZ_TEST_BOOL(obj1.TryGetComponentOfBaseType(@fog));

            EZ_TEST_BOOL(m_hFog.IsInvalidated());
            m_hFog = fog.GetHandle();
            EZ_TEST_BOOL(!m_hFog.IsInvalidated());

            fog.Color = ezColor::OrangeRed;
            fog.Density = 5;

            ezPointLightComponent@ light;
            EZ_TEST_BOOL(obj2.TryGetComponentOfBaseType(@light));
            m_hLight = light.GetHandle();
            EZ_TEST_BOOL(!light.Active);
            light.Active = true;

            m_tStart = ezTime::Now();
            m_Phase = Phase::Render;

            ezGameObjectDesc gd;
            gd.m_LocalPosition.Set(0, 0, 10);
            
            ezGameObject@ spawnObj;
            GetWorld().CreateObject(gd, @spawnObj);

            ezSpawnComponent@ spawnCmp;
            spawnObj.CreateComponent(@spawnCmp);

            spawnCmp.Prefab = "{ a3ce5d3d-be5e-4bda-8820-b1ce3b3d33fd }";
            spawnCmp.Deviation = ezAngle::MakeFromDegree(30);
            spawnCmp.TriggerManualSpawn(true, ezVec3::MakeZero());

            ezAlwaysVisibleComponent@ vis;
            spawnObj.CreateComponent(@vis); // needed for the text to show up

            ezDebugTextComponent@ text;
            spawnObj.CreateComponent(@text);
            text.MaxDistance = 1000;
            text.Color = ezColor::LightGreen;
            text.Text = "Hello World! ({})";
            text.Value0 = 42;

            ezGameObject@ boxObj;
            gd.m_LocalPosition.z = 7;
            GetWorld().CreateObject(gd, @boxObj);

            ezMeshComponent@ mesh;
            boxObj.CreateComponent(@mesh);
            mesh.Mesh = "{ e9e6b167-c18c-4260-9b0e-4cf4503c1463 }";
            mesh.Color = ezColor::BlueViolet * 10;
            mesh.CustomData = ezVec4(1, 2, 3, 4);

            ezGameObject@ skybox;
            EZ_TEST_BOOL(GetWorld().TryGetObjectWithGlobalKey("Skybox", skybox));

            ezSkyBoxComponent@ skyboxComp;
            EZ_TEST_BOOL(skybox.TryGetComponentOfBaseType(@skyboxComp));

            skyboxComp.CubeMap = "{ da307f05-9ef4-4a09-81d4-b86f5be99412 }";

            ezGameObject@ bbObj;
            gd.m_LocalPosition.z = 7;
            gd.m_LocalPosition.y = 5;
            GetWorld().CreateObject(gd, @bbObj);            

            ezLocalBlackboardComponent@ bbComp;
            bbObj.CreateComponent(@bbComp);
            m_hBlackboard = bbComp.GetHandle();

            bbComp.SetEntryValue("a", 1);
            bbComp.SetEntryValue("b", 2.0f);
            bbComp.SetEntryValue("c", ezColor::RebeccaPurple);
            bbComp.SetEntryValue("d", ezVec3(1, 2, 3));
            bbComp.SetEntryValue("e", ezHashedString("HS"));
            bbComp.SetEntryValue("f", ezColorGammaUB(50, 100, 150, 250));
            bbComp.SetEntryValue("g", m_hChild1);
            bbComp.SetEntryValue("h", m_hFog);
        }
        else if (m_Phase == Phase::Render)
        {
            ezFogComponent@ fog;
            EZ_TEST_BOOL(GetWorld().TryGetComponent(m_hFog, @fog));

            const float fLerp = ezMath::Min(1.0f, ((ezTime::Now() - m_tStart) / RenderDelay).AsFloatInSeconds());

            fog.Color = ezMath::Lerp(ezColor::OrangeRed, ezColor::CornflowerBlue, fLerp);

            ezWorld@ fogWorld = fog.GetWorld();
            ezGameObject@ fogObj = fog.GetOwner();
            fog.GetOwner().SetGlobalPosition(ezMath::Lerp(ezVec3(0, 0, 5), ezVec3(0, 0, 10), fLerp));

            if (ezTime::Now() - m_tStart >= RenderDelay)
            {
                ezPointLightComponent@ light;
                EZ_TEST_BOOL(GetWorld().TryGetComponent(m_hLight, @light));
                light.Active = false;

                fogObj.SetActiveFlag(false);
    
                m_Phase = Phase::Delete;
            }
        }
        else if (m_Phase == Phase::Delete)
        {
            GetWorld().DeleteObjectDelayed(m_hChild1);

            ezMsgDeleteGameObject msg;
            GetWorld().PostMessage(m_hChild2, msg, ezTime::MakeZero(), ezObjectMsgQueueType::NextFrame);

            {
                ezLocalBlackboardComponent@ bbComp;
                EZ_TEST_BOOL(GetWorld().TryGetComponent(m_hBlackboard, @bbComp));

                EZ_TEST_INT(bbComp.GetEntryValue_asInt32("a"), 1);
                EZ_TEST_FLOAT(bbComp.GetEntryValue_asFloat("b"), 2);
                EZ_TEST_COLOR(bbComp.GetEntryValue_asColor("c"), ezColor::RebeccaPurple);
                EZ_TEST_VEC3(bbComp.GetEntryValue_asVec3("d"), ezVec3(1, 2, 3));
                EZ_TEST_STRING(bbComp.GetEntryValue_asString("e"), "HS");
                EZ_TEST_COLOR(bbComp.GetEntryValue_asColor("f"), ezColorGammaUB(50, 100, 150, 250));
                EZ_TEST_BOOL(bbComp.GetEntryValue_asGameObjectHandle("g") == m_hChild1);
                EZ_TEST_BOOL(bbComp.GetEntryValue_asComponentHandle("h") == m_hFog);
            }
            
            m_Phase = Phase::CheckDelete;
        }
        else if (m_Phase == Phase::CheckDelete)
        {
            ezGameObject@ obj;

            EZ_TEST_BOOL(!GetWorld().TryGetObject(m_hChild1, obj));
            EZ_TEST_BOOL(!GetWorld().TryGetObject(m_hChild2, obj));
            
            m_Phase = Phase::Done;
        }

        return m_Phase != Phase::Done;
    }
}