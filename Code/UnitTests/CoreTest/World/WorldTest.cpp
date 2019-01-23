#include <PCH.h>

#include <Core/World/World.h>
#include <Foundation/Time/Clock.h>

EZ_CREATE_SIMPLE_TEST_GROUP(World);

namespace
{
  union TestWorldObjects {
    struct
    {
      ezGameObject* pParent1;
      ezGameObject* pParent2;
      ezGameObject* pChild11;
      ezGameObject* pChild21;
    };
    ezGameObject* pObjects[4];
  };

  TestWorldObjects CreateTestWorld(ezWorld& world, bool bDynamic)
  {
    TestWorldObjects testWorldObjects;
    ezMemoryUtils::ZeroFill(&testWorldObjects, 1);

    ezQuat q;
    q.SetFromAxisAndAngle(ezVec3(0.0f, 0.0f, 1.0f), ezAngle::Degree(90.0f));

    ezGameObjectDesc desc;
    desc.m_bDynamic = bDynamic;
    desc.m_LocalPosition = ezVec3(100.0f, 0.0f, 0.0f);
    desc.m_LocalRotation = q;
    desc.m_LocalScaling = ezVec3(1.5f, 1.5f, 1.5f);
    desc.m_sName.Assign("Parent1");

    world.CreateObject(desc, testWorldObjects.pParent1);

    desc.m_sName.Assign("Parent2");
    world.CreateObject(desc, testWorldObjects.pParent2);

    desc.m_hParent = testWorldObjects.pParent1->GetHandle();
    desc.m_sName.Assign("Child11");
    world.CreateObject(desc, testWorldObjects.pChild11);

    desc.m_hParent = testWorldObjects.pParent2->GetHandle();
    desc.m_sName.Assign("Child21");
    world.CreateObject(desc, testWorldObjects.pChild21);

    return testWorldObjects;
  }

  void TestTransforms(const TestWorldObjects& o, ezVec3 offset = ezVec3(100.0f, 0.0f, 0.0f))
  {
    const float eps = ezMath::BasicType<float>::DefaultEpsilon();
    ezQuat q;
    q.SetFromAxisAndAngle(ezVec3(0.0f, 0.0f, 1.0f), ezAngle::Degree(90.0f));

    for (ezUInt32 i = 0; i < 2; ++i)
    {
      EZ_TEST_VEC3(o.pObjects[i]->GetGlobalPosition(), offset, 0);
      EZ_TEST_BOOL(o.pObjects[i]->GetGlobalRotation().IsEqualRotation(q, eps * 10.0f));
      EZ_TEST_VEC3(o.pObjects[i]->GetGlobalScaling(), ezVec3(1.5f, 1.5f, 1.5f), 0);
    }

    for (ezUInt32 i = 2; i < 4; ++i)
    {
      EZ_TEST_VEC3(o.pObjects[i]->GetGlobalPosition(), offset + ezVec3(0.0f, 150.0f, 0.0f), eps * 2.0f);
      EZ_TEST_BOOL(o.pObjects[i]->GetGlobalRotation().IsEqualRotation(q * q, eps * 10.0f));
      EZ_TEST_VEC3(o.pObjects[i]->GetGlobalScaling(), ezVec3(2.25f, 2.25f, 2.25f), 0);
    }
  }

  void SanityCheckWorld(ezWorld& world)
  {
    struct Traverser
    {
      Traverser(ezWorld& world)
          : m_World(world)
      {
      }

      ezWorld& m_World;
      ezSet<ezGameObject*> m_Found;

      ezVisitorExecution::Enum Visit(ezGameObject* pObject)
      {
        ezGameObject* pObject2 = nullptr;
        EZ_TEST_BOOL_MSG(m_World.TryGetObject(pObject->GetHandle(), pObject2), "Visited object that is not part of the world!");
        EZ_TEST_BOOL_MSG(pObject2 == pObject, "Handle did not resolve to the same object!");
        EZ_TEST_BOOL_MSG(!m_Found.Contains(pObject), "Object visited twice!");
        m_Found.Insert(pObject);

        const ezUInt32 uiChildren = pObject->GetChildCount();
        ezUInt32 uiChildren2 = 0;
        for (auto it = pObject->GetChildren(); it.IsValid(); ++it)
        {
          uiChildren2++;
          auto handle = it->GetHandle();
          ezGameObject* pChild = nullptr;
          EZ_TEST_BOOL_MSG(m_World.TryGetObject(handle, pChild), "Could not resolve child!");
          ezGameObject* pParent = pChild->GetParent();
          EZ_TEST_BOOL_MSG(pParent == pObject, "pObject's child's parent does not point to pObject!");
        }
        EZ_TEST_INT(uiChildren, uiChildren2);
        return ezVisitorExecution::Continue;
      }
    };

    Traverser traverser(world);
    world.Traverse(ezWorld::VisitorFunc(&Traverser::Visit, &traverser), ezWorld::TraversalMethod::BreadthFirst);
  }

  class CustomCoordinateSystemProvider : public ezCoordinateSystemProvider
  {
  public:
    CustomCoordinateSystemProvider(const ezWorld* pWorld)
        : ezCoordinateSystemProvider(pWorld)
    {
    }

    virtual void GetCoordinateSystem(const ezVec3& vGlobalPosition, ezCoordinateSystem& out_CoordinateSystem) const override
    {
      ezMat3 mTmp;
      mTmp.SetLookInDirectionMatrix(-vGlobalPosition, ezVec3(0, 0, 1));

      out_CoordinateSystem.m_vRightDir = mTmp.GetRow(0);
      out_CoordinateSystem.m_vUpDir = mTmp.GetRow(1);
      out_CoordinateSystem.m_vForwardDir = mTmp.GetRow(2);
    }
  };
}

class ezGameObjectTest
{
public:
  static void TestInternals(ezGameObject* pObject, ezGameObject* pParent, ezUInt32 uiHierarchyLevel)
  {
    EZ_TEST_INT(pObject->m_uiHierarchyLevel, uiHierarchyLevel);
    EZ_TEST_BOOL(pObject->m_pTransformationData->m_pObject == pObject);

    if (pParent)
    {
      EZ_TEST_BOOL(pObject->m_pTransformationData->m_pParentData->m_pObject == pParent);
    }

    EZ_TEST_BOOL(pObject->m_pTransformationData->m_pParentData == (pParent != nullptr ? pParent->m_pTransformationData : nullptr));
    EZ_TEST_BOOL(pObject->GetParent() == pParent);
  }
};

EZ_CREATE_SIMPLE_TEST(World, World)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Transforms dynamic")
  {
    ezWorldDesc worldDesc("Test");
    ezWorld world(worldDesc);
    EZ_LOCK(world.GetWriteMarker());

    TestWorldObjects o = CreateTestWorld(world, true);

    ezVec3 offset = ezVec3(200.0f, 0.0f, 0.0f);
    o.pParent1->SetLocalPosition(offset);
    o.pParent2->SetLocalPosition(offset);

    world.Update();

    TestTransforms(o, offset);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Transforms static")
  {
    ezWorldDesc worldDesc("Test");
    worldDesc.m_bReportErrorWhenStaticObjectMoves = false;

    ezWorld world(worldDesc);
    EZ_LOCK(world.GetWriteMarker());

    TestWorldObjects o = CreateTestWorld(world, false);

    ezVec3 offset = ezVec3(200.0f, 0.0f, 0.0f);
    o.pParent1->SetLocalPosition(offset);
    o.pParent2->SetLocalPosition(offset);

    // No need to call world update since global transform is updated immediately for static objects.
    // world.Update();

    TestTransforms(o, offset);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GameObject parenting")
  {
    ezWorldDesc worldDesc("Test");
    ezWorld world(worldDesc);
    EZ_LOCK(world.GetWriteMarker());

    const float eps = ezMath::BasicType<float>::DefaultEpsilon();
    ezQuat q;
    q.SetFromAxisAndAngle(ezVec3(0.0f, 0.0f, 1.0f), ezAngle::Degree(90.0f));

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

    EZ_TEST_VEC3(pParentObject->GetGlobalPosition(), desc.m_LocalPosition, 0);
    EZ_TEST_BOOL(pParentObject->GetGlobalRotation().IsEqualRotation(desc.m_LocalRotation, eps * 10.0f));
    EZ_TEST_VEC3(pParentObject->GetGlobalScaling(), desc.m_LocalScaling, 0);

    EZ_TEST_STRING(pParentObject->GetName(), desc.m_sName.GetString().GetData());

    desc.m_LocalRotation.SetIdentity();
    desc.m_LocalScaling.Set(1.0f);
    desc.m_hParent = parentObject;

    ezGameObjectHandle childObjects[10];
    for (ezUInt32 i = 0; i < 10; ++i)
    {
      ezStringBuilder sb;
      sb.AppendFormat("Child_{0}", i);
      desc.m_sName.Assign(sb.GetData());

      desc.m_LocalPosition = ezVec3(i * 10.0f, 0.0f, 0.0f);

      childObjects[i] = world.CreateObject(desc);
    }

    ezUInt32 uiCounter = 0;
    for (auto it = pParentObject->GetChildren(); it.IsValid(); ++it)
    {
      ezStringBuilder sb;
      sb.AppendFormat("Child_{0}", uiCounter);

      EZ_TEST_STRING(it->GetName(), sb.GetData());

      EZ_TEST_VEC3(it->GetGlobalPosition(), ezVec3(100.0f, uiCounter * 15.0f, 0.0f), eps * 2.0f); // 15 because parent is scaled by 1.5
      EZ_TEST_BOOL(it->GetGlobalRotation().IsEqualRotation(q, eps * 10.0f));
      EZ_TEST_VEC3(it->GetGlobalScaling(), ezVec3(1.5f, 1.5f, 1.5f), 0.0f);

      ++uiCounter;
    }

    EZ_TEST_INT(uiCounter, 10);
    EZ_TEST_INT(pParentObject->GetChildCount(), 10);

    world.DeleteObjectNow(childObjects[0]);
    world.DeleteObjectNow(childObjects[3]);
    world.DeleteObjectNow(childObjects[9]);

    EZ_TEST_BOOL(!world.IsValidObject(childObjects[0]));
    EZ_TEST_BOOL(!world.IsValidObject(childObjects[3]));
    EZ_TEST_BOOL(!world.IsValidObject(childObjects[9]));

    ezUInt32 indices[7] = {1, 2, 4, 5, 6, 7, 8};

    uiCounter = 0;
    for (auto it = pParentObject->GetChildren(); it.IsValid(); ++it)
    {
      ezStringBuilder sb;
      sb.AppendFormat("Child_{0}", indices[uiCounter]);

      EZ_TEST_STRING(it->GetName(), sb.GetData());

      ++uiCounter;
    }

    EZ_TEST_INT(uiCounter, 7);
    EZ_TEST_INT(pParentObject->GetChildCount(), 7);

    // do one update step so dead objects get deleted
    world.Update();
    SanityCheckWorld(world);

    EZ_TEST_BOOL(!world.IsValidObject(childObjects[0]));
    EZ_TEST_BOOL(!world.IsValidObject(childObjects[3]));
    EZ_TEST_BOOL(!world.IsValidObject(childObjects[9]));

    uiCounter = 0;
    for (auto it = pParentObject->GetChildren(); it.IsValid(); ++it)
    {
      ezStringBuilder sb;
      sb.AppendFormat("Child_{0}", indices[uiCounter]);

      EZ_TEST_STRING(it->GetName(), sb.GetData());

      ++uiCounter;
    }

    EZ_TEST_INT(uiCounter, 7);
    EZ_TEST_INT(pParentObject->GetChildCount(), 7);

    world.DeleteObjectDelayed(parentObject);
    EZ_TEST_BOOL(world.IsValidObject(parentObject));

    for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(indices); ++i)
    {
      EZ_TEST_BOOL(world.IsValidObject(childObjects[indices[i]]));
    }

    // do one update step so dead objects get deleted
    world.Update();
    SanityCheckWorld(world);

    EZ_TEST_BOOL(!world.IsValidObject(parentObject));

    for (ezUInt32 i = 0; i < 10; ++i)
    {
      EZ_TEST_BOOL(!world.IsValidObject(childObjects[i]));
    }

    EZ_TEST_INT(world.GetObjectCount(), 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Re-parenting 1")
  {
    ezWorldDesc worldDesc("Test");
    ezWorld world(worldDesc);
    EZ_LOCK(world.GetWriteMarker());

    TestWorldObjects o = CreateTestWorld(world, true);

    o.pParent1->AddChild(o.pParent2->GetHandle());
    o.pParent2->SetParent(o.pParent1->GetHandle());
    SanityCheckWorld(world);
    // No need to update the world since re-parenting is now done immediately.
    // world.Update();

    TestTransforms(o);

    ezGameObjectTest::TestInternals(o.pParent1, nullptr, 0);
    ezGameObjectTest::TestInternals(o.pParent2, o.pParent1, 1);
    ezGameObjectTest::TestInternals(o.pChild11, o.pParent1, 1);
    ezGameObjectTest::TestInternals(o.pChild21, o.pParent2, 2);

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
    ezWorldDesc worldDesc("Test");
    ezWorld world(worldDesc);
    EZ_LOCK(world.GetWriteMarker());

    TestWorldObjects o = CreateTestWorld(world, true);

    o.pChild21->SetParent(ezGameObjectHandle());
    SanityCheckWorld(world);
    // No need to update the world since re-parenting is now done immediately.
    // world.Update();

    TestTransforms(o);

    ezGameObjectTest::TestInternals(o.pParent1, nullptr, 0);
    ezGameObjectTest::TestInternals(o.pParent2, nullptr, 0);
    ezGameObjectTest::TestInternals(o.pChild11, o.pParent1, 1);
    ezGameObjectTest::TestInternals(o.pChild21, nullptr, 0);

    auto it = o.pParent1->GetChildren();
    EZ_TEST_BOOL(o.pChild11 == it);
    ++it;
    EZ_TEST_BOOL(!it.IsValid());

    EZ_TEST_INT(o.pParent2->GetChildCount(), 0);
    it = o.pParent2->GetChildren();
    EZ_TEST_BOOL(!it.IsValid());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Re-parenting 3")
  {
    ezWorldDesc worldDesc("Test");
    ezWorld world(worldDesc);
    EZ_LOCK(world.GetWriteMarker());

    TestWorldObjects o = CreateTestWorld(world, true);
    SanityCheckWorld(world);
    // Here we test whether the sibling information is correctly cleared.

    o.pChild21->SetParent(o.pParent1->GetHandle());
    SanityCheckWorld(world);
    // pChild21 has a previous (pChild11) sibling.
    o.pParent2->SetParent(o.pParent1->GetHandle());
    SanityCheckWorld(world);
    // pChild21 has a previous (pChild11) and next (pParent2) sibling.
    o.pChild21->SetParent(ezGameObjectHandle());
    SanityCheckWorld(world);
    // pChild21 has no siblings.
    o.pChild21->SetParent(o.pParent1->GetHandle());
    SanityCheckWorld(world);
    // pChild21 has a previous (pChild11) sibling again.
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Traversal")
  {
    ezWorldDesc worldDesc("Test");
    ezWorld world(worldDesc);
    EZ_LOCK(world.GetWriteMarker());

    TestWorldObjects o = CreateTestWorld(world, false);

    {
      struct BreadthFirstTest
      {
        BreadthFirstTest() { m_uiCounter = 0; }

        ezVisitorExecution::Enum Visit(ezGameObject* pObject)
        {
          if (m_uiCounter < EZ_ARRAY_SIZE(m_o.pObjects))
          {
            EZ_TEST_BOOL(pObject == m_o.pObjects[m_uiCounter]);
          }

          ++m_uiCounter;
          return ezVisitorExecution::Continue;
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

        ezVisitorExecution::Enum Visit(ezGameObject* pObject)
        {
          if (m_uiCounter == 0)
          {
            EZ_TEST_BOOL(pObject == m_o.pParent1);
          }
          else if (m_uiCounter == 1)
          {
            EZ_TEST_BOOL(pObject == m_o.pChild11);
          }
          else if (m_uiCounter == 2)
          {
            EZ_TEST_BOOL(pObject == m_o.pParent2);
          }
          else if (m_uiCounter == 3)
          {
            EZ_TEST_BOOL(pObject == m_o.pChild21);
          }

          ++m_uiCounter;
          if (m_uiCounter >= EZ_ARRAY_SIZE(m_o.pObjects))
            return ezVisitorExecution::Stop;

          return ezVisitorExecution::Continue;
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
    ezWorldDesc worldDesc1("Test1");
    ezWorld world1(worldDesc1);
    EZ_LOCK(world1.GetWriteMarker());

    ezWorldDesc worldDesc2("Test2");
    ezWorld world2(worldDesc2);
    EZ_LOCK(world2.GetWriteMarker());

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

    pObj1->SetGlobalKey("Obj1");
    pObj1 = nullptr;
    EZ_TEST_BOOL(world1.TryGetObjectWithGlobalKey(ezTempHashedString("Obj1"), pObj1));
    EZ_TEST_BOOL(!world1.TryGetObjectWithGlobalKey(ezTempHashedString("Obj2"), pObj1));
    EZ_TEST_BOOL(pObj1 != nullptr);

    ezGameObject* pObj2 = nullptr;
    EZ_TEST_BOOL(world2.TryGetObject(hObj2, pObj2));
    EZ_TEST_BOOL(pObj2 != nullptr);

    pObj2->SetGlobalKey("Obj2");
    pObj2 = nullptr;
    EZ_TEST_BOOL(world2.TryGetObjectWithGlobalKey(ezTempHashedString("Obj2"), pObj2));
    EZ_TEST_BOOL(!world2.TryGetObjectWithGlobalKey(ezTempHashedString("Obj1"), pObj2));
    EZ_TEST_BOOL(pObj2 != nullptr);

    pObj2->SetGlobalKey("Deschd");
    EZ_TEST_BOOL(world2.TryGetObjectWithGlobalKey(ezTempHashedString("Deschd"), pObj2));
    EZ_TEST_BOOL(!world2.TryGetObjectWithGlobalKey(ezTempHashedString("Obj2"), pObj2));

    world2.DeleteObjectNow(hObj2);

    EZ_TEST_BOOL(!world2.IsValidObject(hObj2));
    EZ_TEST_BOOL(!world2.TryGetObjectWithGlobalKey(ezTempHashedString("Deschd"), pObj2));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Custom coordinate system")
  {
    ezWorldDesc worldDesc("Test");
    ezWorld world(worldDesc);

    ezUniquePtr<CustomCoordinateSystemProvider> pProvider = EZ_DEFAULT_NEW(CustomCoordinateSystemProvider, &world);
    CustomCoordinateSystemProvider* pProviderBackup = pProvider.Borrow();

    world.SetCoordinateSystemProvider(std::move(pProvider));
    EZ_TEST_BOOL(&world.GetCoordinateSystemProvider() == pProviderBackup);

    ezVec3 pos = ezVec3(2, 3, 0);

    ezCoordinateSystem coordSys;
    world.GetCoordinateSystem(pos, coordSys);

    EZ_TEST_VEC3(coordSys.m_vForwardDir, (-pos).GetNormalized(), ezMath::BasicType<float>::SmallEpsilon());
    EZ_TEST_VEC3(coordSys.m_vUpDir, ezVec3(0, 0, 1), ezMath::BasicType<float>::SmallEpsilon());
  }
}
