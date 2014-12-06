#include <PCH.h>
#include <Foundation/Time/Clock.h>
#include <Core/World/World.h>

EZ_CREATE_SIMPLE_TEST_GROUP(World);

namespace
{
  union TestWorldObjects
  {
    struct
    {
      ezGameObject* pParent1;
      ezGameObject* pParent2;
      ezGameObject* pChild11;
      ezGameObject* pChild21;
    };
    ezGameObject* pObjects[4];
  };

  TestWorldObjects CreateTestWorld(ezWorld& world)
  {
    TestWorldObjects testWorldObjects;
    ezMemoryUtils::ZeroFill(&testWorldObjects);

    ezQuat q; q.SetFromAxisAndAngle(ezVec3(0.0f, 0.0f, 1.0f), ezAngle::Degree(90.0f));

    ezGameObjectDesc desc;
    desc.m_LocalPosition = ezVec3(100.0f, 0.0f, 0.0f);
    desc.m_LocalRotation = q;
    desc.m_LocalScaling = ezVec3(1.5f, 1.5f, 1.5f);
    desc.m_sName.Assign("Parent1");

    world.CreateObject(desc, testWorldObjects.pParent1);

    desc.m_sName.Assign("Parent2");
    world.CreateObject(desc, testWorldObjects.pParent2);

    desc.m_Parent = testWorldObjects.pParent1->GetHandle();
    desc.m_sName.Assign("Child11");
    world.CreateObject(desc, testWorldObjects.pChild11);

    desc.m_Parent = testWorldObjects.pParent2->GetHandle();
    desc.m_sName.Assign("Child21");
    world.CreateObject(desc, testWorldObjects.pChild21);

    return testWorldObjects;
  }

  void TestTransforms(const TestWorldObjects& o)
  {
    ezQuat q; q.SetFromAxisAndAngle(ezVec3(0.0f, 0.0f, 1.0f), ezAngle::Degree(90.0f));

    for (ezUInt32 i = 0; i < sizeof(TestWorldObjects) / sizeof(ezGameObject*); ++i)
    {
      EZ_TEST_VEC3(o.pObjects[i]->GetLocalPosition(), ezVec3(100.0f, 0.0f, 0.0f), 0);
      EZ_TEST_BOOL(o.pObjects[i]->GetLocalRotation() == q);
      EZ_TEST_VEC3(o.pObjects[i]->GetLocalScaling(), ezVec3(1.5f, 1.5f, 1.5f), 0);
    }
  }
}

class ezGameObjectTest
{
public:
  static void TestInternals(ezGameObject* pObject, ezGameObject* pParent, ezUInt32 uiHierarchyLevel, ezUInt32 uiTransformationDataIndex)
  {
    EZ_TEST_INT(pObject->m_uiHierarchyLevel, uiHierarchyLevel);
    EZ_TEST_INT(pObject->m_uiTransformationDataIndex, uiTransformationDataIndex);
    EZ_TEST_BOOL(pObject->m_pTransformationData->m_pObject == pObject);
    EZ_TEST_BOOL(pObject->m_pTransformationData->m_pParentData == (pParent != nullptr ? pParent->m_pTransformationData : nullptr));
    EZ_TEST_BOOL(pObject->GetParent() == pParent);
  }
};

EZ_CREATE_SIMPLE_TEST(World, World)
{
  ezClock::SetNumGlobalClocks();
  
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GameObject parenting")
  {
    ezWorld world("Test");

    const float eps = ezMath::BasicType<float>::DefaultEpsilon();
    ezQuat q; q.SetFromAxisAndAngle(ezVec3(0.0f, 0.0f, 1.0f), ezAngle::Degree(90.0f));

    ezGameObjectDesc desc;
    desc.m_LocalPosition = ezVec3(100.0f, 0.0f, 0.0f);
    desc.m_LocalRotation = q;
    desc.m_LocalScaling = ezVec3(1.5f, 1.5f, 1.5f);
    desc.m_sName.Assign("Parent");

    ezGameObject* pParentObject;
    ezGameObjectHandle parentObject = world.CreateObject(desc, pParentObject);

    EZ_TEST_VEC3(pParentObject->GetLocalPosition(), desc.m_LocalPosition, 0);
    EZ_TEST_BOOL(pParentObject->GetLocalRotation() == desc.m_LocalRotation);
    EZ_TEST_VEC3(pParentObject->GetLocalScaling(), desc.m_LocalScaling, 0);

    EZ_TEST_VEC3(pParentObject->GetWorldPosition(), desc.m_LocalPosition, 0);
    EZ_TEST_BOOL(pParentObject->GetWorldRotation().IsEqualRotation(desc.m_LocalRotation, eps * 10.0f));
    EZ_TEST_VEC3(pParentObject->GetWorldScaling(), desc.m_LocalScaling, 0);

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

      EZ_TEST_VEC3(it->GetWorldPosition(), ezVec3(100.0f, uiCounter * 15.0f, 0.0f), eps * 2.0f); // 15 because parent is scaled by 1.5
      EZ_TEST_BOOL(it->GetWorldRotation().IsEqualRotation(q, eps * 10.0f));
      EZ_TEST_VEC3(it->GetWorldScaling(), ezVec3(1.5f, 1.5f, 1.5f), 0.0f);

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

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Re-parenting 1")
  {
    ezWorld world("Test");
    TestWorldObjects o = CreateTestWorld(world);

    o.pParent1->AddChild(o.pParent2->GetHandle());
    o.pParent2->SetParent(o.pParent1->GetHandle());

    world.Update();

    TestTransforms(o);

    ezGameObjectTest::TestInternals(o.pParent1, nullptr, 0, 0);
    ezGameObjectTest::TestInternals(o.pParent2, o.pParent1, 1, 1);
    ezGameObjectTest::TestInternals(o.pChild11, o.pParent1, 1, 0);
    ezGameObjectTest::TestInternals(o.pChild21, o.pParent2, 2, 0);

    EZ_TEST_INT(o.pParent1->GetChildCount(), 2);
    auto it = o.pParent1->GetChildren();
    EZ_TEST_BOOL(o.pChild11 == it);
    ++it;
    EZ_TEST_BOOL(o.pParent2 == it);
    ++it;
    EZ_TEST_BOOL(!it.IsValid());

    it = o.pParent2->GetChildren();
    EZ_TEST_BOOL(o.pChild21 == it);
    ++it;
    EZ_TEST_BOOL(!it.IsValid());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Re-parenting 2")
  {
    ezWorld world("Test");
    TestWorldObjects o = CreateTestWorld(world);

    o.pChild21->SetParent(ezGameObjectHandle());

    world.Update();

    TestTransforms(o);

    ezGameObjectTest::TestInternals(o.pParent1, nullptr, 0, 0);
    ezGameObjectTest::TestInternals(o.pParent2, nullptr, 0, 1);
    ezGameObjectTest::TestInternals(o.pChild11, o.pParent1, 1, 0);
    ezGameObjectTest::TestInternals(o.pChild21, nullptr, 0, 2);

    auto it = o.pParent1->GetChildren();
    EZ_TEST_BOOL(o.pChild11 == it);
    ++it;
    EZ_TEST_BOOL(!it.IsValid());

    EZ_TEST_INT(o.pParent2->GetChildCount(), 0);
    it = o.pParent2->GetChildren();
    EZ_TEST_BOOL(!it.IsValid());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Traversal")
  {
    ezWorld world("Test");
    TestWorldObjects o = CreateTestWorld(world);

    {
      struct BreadthFirstTest
      {
        BreadthFirstTest() { m_uiCounter = 0; }

        bool Visit(ezGameObject* pObject)
        {
          if (m_uiCounter < EZ_ARRAY_SIZE(m_o.pObjects))
          {
            EZ_TEST_BOOL(pObject == m_o.pObjects[m_uiCounter]);
          }

          ++m_uiCounter;
          return true;
        }

        ezUInt32 m_uiCounter;
        TestWorldObjects m_o;
      };

      BreadthFirstTest bft;
      bft.m_o = o;

      world.Traverse(ezWorld::VisitorFunc(&BreadthFirstTest::Visit, &bft), ezWorld::BreadthFirst);
      EZ_TEST_INT(bft.m_uiCounter, EZ_ARRAY_SIZE(o.pObjects));
    }

    {
      world.CreateObject(ezGameObjectDesc());

      struct DepthFirstTest
      {
        DepthFirstTest() { m_uiCounter = 0; }

        bool Visit(ezGameObject* pObject)
        {
          if      (m_uiCounter == 0) { EZ_TEST_BOOL(pObject == m_o.pParent1); }
          else if (m_uiCounter == 1) { EZ_TEST_BOOL(pObject == m_o.pChild11); }
          else if (m_uiCounter == 2) { EZ_TEST_BOOL(pObject == m_o.pParent2); }
          else if (m_uiCounter == 3) { EZ_TEST_BOOL(pObject == m_o.pChild21); }

          ++m_uiCounter;
          if (m_uiCounter >= EZ_ARRAY_SIZE(m_o.pObjects))
            return false;

          return true;
        }

        ezUInt32 m_uiCounter;
        TestWorldObjects m_o;
      };

      DepthFirstTest dft;
      dft.m_o = o;

      world.Traverse(ezWorld::VisitorFunc(&DepthFirstTest::Visit, &dft), ezWorld::DepthFirst);
      EZ_TEST_INT(dft.m_uiCounter, EZ_ARRAY_SIZE(o.pObjects));
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Multiple Worlds")
  {
    ezWorld world1("Test1");
    ezWorld world2("Test2");

    ezGameObjectDesc desc;
    desc.m_sName.Assign("Obj1");
    
    ezGameObjectHandle hObj1 = world1.CreateObject(desc);
    EZ_TEST_BOOL(world1.IsValidObject(hObj1));

    desc.m_sName.Assign("Obj2");

    ezGameObjectHandle hObj2 = world2.CreateObject(desc);
    EZ_TEST_BOOL(world2.IsValidObject(hObj2));

    ezGameObject* pObj1 = nullptr;
    EZ_TEST_BOOL(world1.TryGetObject(hObj1, pObj1));
    EZ_TEST_BOOL(pObj1 != nullptr);

    ezGameObject* pObj2 = nullptr;
    EZ_TEST_BOOL(world2.TryGetObject(hObj2, pObj2));
    EZ_TEST_BOOL(pObj2 != nullptr);

    world2.DeleteObject(hObj2);

    EZ_TEST_BOOL(!world2.IsValidObject(hObj2));
  }
}
