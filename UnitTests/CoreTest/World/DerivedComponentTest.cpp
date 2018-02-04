#include <PCH.h>
#include <Foundation/Time/Clock.h>
#include <Core/World/World.h>

namespace
{
  typedef ezComponentManagerSimple<class TestComponentBase, ezComponentUpdateType::Always> TestComponentBaseManager;

  class TestComponentBase : public ezComponent
  {
    EZ_DECLARE_COMPONENT_TYPE(TestComponentBase, ezComponent, TestComponentBaseManager);

  public:
    void Update()
    {
      ++s_iUpdateCounter;
    }

    static int s_iUpdateCounter;
  };

  int TestComponentBase::s_iUpdateCounter = 0;

  EZ_BEGIN_COMPONENT_TYPE(TestComponentBase, 1, ezComponentMode::Static)
  EZ_END_COMPONENT_TYPE

  //////////////////////////////////////////////////////////////////////////

  typedef ezComponentManagerSimple<class TestComponentDerived1, ezComponentUpdateType::Always> TestComponentDerived1Manager;

  class TestComponentDerived1 : public TestComponentBase
  {
    EZ_DECLARE_COMPONENT_TYPE(TestComponentDerived1, TestComponentBase, TestComponentDerived1Manager);

  public:
    void Update()
    {
      ++s_iUpdateCounter;
    }

    static int s_iUpdateCounter;
  };

  int TestComponentDerived1::s_iUpdateCounter = 0;

  EZ_BEGIN_COMPONENT_TYPE(TestComponentDerived1, 1, ezComponentMode::Static)
  EZ_END_COMPONENT_TYPE
}


EZ_CREATE_SIMPLE_TEST(World, DerivedComponents)
{
  ezWorldDesc worldDesc("Test");
  ezWorld world(worldDesc);
  EZ_LOCK(world.GetWriteMarker());

  TestComponentBaseManager* pManagerBase = world.GetOrCreateComponentManager<TestComponentBaseManager>();
  TestComponentDerived1Manager* pManagerDerived1 = world.GetOrCreateComponentManager<TestComponentDerived1Manager>();

  ezGameObjectDesc desc;
  ezGameObject* pObject;
  ezGameObjectHandle hObject = world.CreateObject(desc, pObject);
  EZ_TEST_BOOL(!hObject.IsInvalidated());

  ezGameObject* pObject2;
  world.CreateObject(desc, pObject2);

  TestComponentBase::s_iUpdateCounter = 0;
  TestComponentDerived1::s_iUpdateCounter = 0;

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Derived Component Update")
  {
    TestComponentBase* pComponentBase = nullptr;
    ezComponentHandle hComponentBase = TestComponentBase::CreateComponent(pObject, pComponentBase);

    TestComponentBase* pTestBase = nullptr;
    EZ_TEST_BOOL(world.TryGetComponent(hComponentBase, pTestBase));
    EZ_TEST_BOOL(pTestBase == pComponentBase);
    EZ_TEST_BOOL(pComponentBase->GetHandle() == hComponentBase);
    EZ_TEST_BOOL(pComponentBase->GetManager() == pManagerBase);

    TestComponentDerived1* pComponentDerived1 = nullptr;
    ezComponentHandle hComponentDerived1 = TestComponentDerived1::CreateComponent(pObject2, pComponentDerived1);

    TestComponentDerived1* pTestDerived1 = nullptr;
    EZ_TEST_BOOL(world.TryGetComponent(hComponentDerived1, pTestDerived1));
    EZ_TEST_BOOL(pTestDerived1 == pComponentDerived1);
    EZ_TEST_BOOL(pComponentDerived1->GetHandle() == hComponentDerived1);
    EZ_TEST_BOOL(pComponentDerived1->GetManager() == pManagerDerived1);

    world.Update();

    EZ_TEST_INT(TestComponentBase::s_iUpdateCounter, 1);
    EZ_TEST_INT(TestComponentDerived1::s_iUpdateCounter, 1);

    // Get component manager via rtti
    EZ_TEST_BOOL(world.GetComponentManager(ezGetStaticRTTI<TestComponentBase>()) == pManagerBase);
    EZ_TEST_BOOL(world.GetComponentManager(ezGetStaticRTTI<TestComponentDerived1>()) == pManagerDerived1);
  }
}
