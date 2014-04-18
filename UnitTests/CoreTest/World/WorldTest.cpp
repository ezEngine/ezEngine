#include <PCH.h>
#include <Foundation/Time/Clock.h>
#include <Core/World/World.h>

EZ_CREATE_SIMPLE_TEST_GROUP(World);

EZ_CREATE_SIMPLE_TEST(World, World)
{
  ezClock::SetNumGlobalClocks();
  ezWorld world("Test");

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GameObject parenting")
  {
    const float eps = ezMath::BasicType<float>::DefaultEpsilon();
    ezQuat q; q.SetFromAxisAndAngle(ezVec3(0.0f, 0.0f, 1.0f), ezAngle::Degree(90.0f));

    ezGameObjectDesc desc;
    desc.m_LocalPosition = ezVec3(100.0f, 0.0f, 0.0f);
    desc.m_LocalRotation = q;
    desc.m_LocalScaling = ezVec3(1.5f, 1.5f, 1.5f);
    desc.m_sName.Assign("Parent");

    ezGameObject* pParentObject;
    ezGameObjectHandle parentObject = world.CreateObject(desc, pParentObject);

    EZ_TEST_BOOL(pParentObject->GetLocalPosition() == desc.m_LocalPosition);
    EZ_TEST_BOOL(pParentObject->GetLocalRotation() == desc.m_LocalRotation);
    EZ_TEST_BOOL(pParentObject->GetLocalScaling() == desc.m_LocalScaling);

    EZ_TEST_BOOL(pParentObject->GetWorldPosition() == desc.m_LocalPosition);
    EZ_TEST_BOOL(pParentObject->GetWorldRotation().IsEqualRotation(desc.m_LocalRotation, eps));
    EZ_TEST_BOOL(pParentObject->GetWorldScaling() == desc.m_LocalScaling);

    EZ_TEST_STRING(pParentObject->GetName(), desc.m_sName.GetString().GetData());

    desc.m_LocalRotation.SetIdentity();
    desc.m_LocalScaling.Set(1.0f);
    desc.m_Parent = parentObject;

    ezGameObjectHandle childObjects[10];
    for (ezUInt32 i = 0; i < 10; ++i)
    {
      ezStringBuilder sb;
      sb.AppendFormat("Child_%d", i);
      desc.m_sName.Assign(sb.GetData());

      desc.m_LocalPosition = ezVec3(i * 10.0f, 0.0f, 0.0f);      
      
      childObjects[i] = world.CreateObject(desc);
    }

    ezUInt32 uiCounter = 0;
    for (auto it = pParentObject->GetChildren(); it.IsValid(); ++it)
    {
      ezStringBuilder sb;
      sb.AppendFormat("Child_%d", uiCounter);

      EZ_TEST_STRING(it->GetName(), sb.GetData());

      EZ_TEST_BOOL(it->GetWorldPosition().IsEqual(ezVec3(100.0f, uiCounter * 15.0f, 0.0f), eps)); // 15 because parent is scaled by 1.5
      EZ_TEST_BOOL(it->GetWorldRotation().IsEqualRotation(q, eps));
      EZ_TEST_BOOL(it->GetWorldScaling() == ezVec3(1.5f, 1.5f, 1.5f));

      ++uiCounter;
    }

    EZ_TEST_INT(uiCounter, 10);
    EZ_TEST_INT(pParentObject->GetChildCount(), 10);

    world.DeleteObject(childObjects[0]);
    world.DeleteObject(childObjects[3]);
    world.DeleteObject(childObjects[9]);

    EZ_TEST_BOOL(!world.IsValidObject(childObjects[0]));
    EZ_TEST_BOOL(!world.IsValidObject(childObjects[3]));
    EZ_TEST_BOOL(!world.IsValidObject(childObjects[9]));

    ezUInt32 indices[7] = { 1, 2, 4, 5, 6, 7, 8 };

    uiCounter = 0;
    for (auto it = pParentObject->GetChildren(); it.IsValid(); ++it)
    {
      ezStringBuilder sb;
      sb.AppendFormat("Child_%d", indices[uiCounter]);

      EZ_TEST_STRING(it->GetName(), sb.GetData());

      ++uiCounter;
    }

    EZ_TEST_INT(uiCounter, 7);
    EZ_TEST_INT(pParentObject->GetChildCount(), 7);

    world.DeleteObject(parentObject);
    EZ_TEST_BOOL(!world.IsValidObject(parentObject));
    
    for (ezUInt32 i = 0; i < 10; ++i)
    {
      EZ_TEST_BOOL(!world.IsValidObject(childObjects[i]));
    }

    // do one update step so dead objects get deleted
    world.Update();

    EZ_TEST_INT(world.GetObjectCount(), 0);
  }
}
