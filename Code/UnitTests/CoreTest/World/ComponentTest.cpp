#include <CoreTestPCH.h>

#include <Core/World/World.h>
#include <Foundation/Time/Clock.h>

namespace
{
  class TestComponent;
  class TestComponentManager : public ezComponentManager<TestComponent, ezBlockStorageType::FreeList>
  {
  public:
    TestComponentManager(ezWorld* pWorld)
      : ezComponentManager<TestComponent, ezBlockStorageType::FreeList>(pWorld)
    {
    }

    virtual void Initialize() override
    {
      auto desc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(TestComponentManager::Update, this);
      auto desc2 = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(TestComponentManager::Update2, this);
      auto desc3 = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(TestComponentManager::Update3, this);
      desc3.m_fPriority = 1000.0f;

      auto desc4 = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(TestComponentManager::AUpdate3, this);
      desc4.m_fPriority = 1000.0f;

      desc.m_DependsOn.PushBack(ezMakeHashedString("TestComponentManager::Update2")); // update2 will be called before update
      desc.m_DependsOn.PushBack(ezMakeHashedString("TestComponentManager::Update3")); // update3 will be called before update

      auto descAsync = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(TestComponentManager::UpdateAsync, this);
      descAsync.m_Phase = ezComponentManagerBase::UpdateFunctionDesc::Phase::Async;
      descAsync.m_uiGranularity = 20;

      // Update functions are now registered in reverse order, so we can test whether dependencies work.
      this->RegisterUpdateFunction(descAsync);
      this->RegisterUpdateFunction(desc4);
      this->RegisterUpdateFunction(desc3);
      this->RegisterUpdateFunction(desc2);
      this->RegisterUpdateFunction(desc);
    }

    void Update(const ezWorldModule::UpdateContext& context);
    void Update2(const ezWorldModule::UpdateContext& context);
    void Update3(const ezWorldModule::UpdateContext& context);
    void AUpdate3(const ezWorldModule::UpdateContext& context);
    void UpdateAsync(const ezWorldModule::UpdateContext& context);
  };

  class TestComponent : public ezComponent
  {
    EZ_DECLARE_COMPONENT_TYPE(TestComponent, ezComponent, TestComponentManager);

  public:
    TestComponent()
      : m_iSomeData(1)
    {
    }
    ~TestComponent() {}

    virtual void Initialize() override { ++s_iInitCounter; }

    virtual void Deinitialize() override { --s_iInitCounter; }

    virtual void OnActivated() override
    {
      ++s_iActivateCounter;

      SpawnOther();
    }

    virtual void OnDeactivated() override { --s_iActivateCounter; }

    virtual void OnSimulationStarted() override { ++s_iSimulationStartedCounter; }

    void Update() { m_iSomeData *= 5; }

    void Update2() { m_iSomeData += 3; }

    void SpawnOther();

    ezInt32 m_iSomeData;

    static ezInt32 s_iInitCounter;
    static ezInt32 s_iActivateCounter;
    static ezInt32 s_iSimulationStartedCounter;

    static bool s_bSpawnOther;
  };

  ezInt32 TestComponent::s_iInitCounter = 0;
  ezInt32 TestComponent::s_iActivateCounter = 0;
  ezInt32 TestComponent::s_iSimulationStartedCounter = 0;
  bool TestComponent::s_bSpawnOther = false;

  EZ_BEGIN_COMPONENT_TYPE(TestComponent, 1, ezComponentMode::Static)
  EZ_END_COMPONENT_TYPE

  void TestComponentManager::Update(const ezWorldModule::UpdateContext& context)
  {
    for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
    {
      if (it->IsActive())
        it->Update();
    }
  }

  void TestComponentManager::Update2(const ezWorldModule::UpdateContext& context)
  {
    for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
    {
      if (it->IsActive())
        it->Update2();
    }
  }

  void TestComponentManager::Update3(const ezWorldModule::UpdateContext& context) {}

  void TestComponentManager::AUpdate3(const ezWorldModule::UpdateContext& context) {}

  void TestComponentManager::UpdateAsync(const ezWorldModule::UpdateContext& context)
  {
    for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
    {
      if (it->IsActive())
        it->Update();
    }
  }

  typedef ezComponentManager<class TestComponent2, ezBlockStorageType::FreeList> TestComponent2Manager;

  class TestComponent2 : public ezComponent
  {
    EZ_DECLARE_COMPONENT_TYPE(TestComponent2, ezComponent, TestComponent2Manager);

    virtual void OnActivated() override { TestComponent::s_iActivateCounter++; }
  };

  EZ_BEGIN_COMPONENT_TYPE(TestComponent2, 1, ezComponentMode::Static)
  EZ_END_COMPONENT_TYPE

  void TestComponent::SpawnOther()
  {
    if (s_bSpawnOther)
    {
      ezGameObjectDesc desc;
      desc.m_hParent = GetOwner()->GetHandle();

      ezGameObject* pChild = nullptr;
      GetWorld()->CreateObject(desc, pChild);

      TestComponent2* pChildComponent = nullptr;
      TestComponent2::CreateComponent(pChild, pChildComponent);
    }
  }
} // namespace


EZ_CREATE_SIMPLE_TEST(World, Components)
{
  ezWorldDesc worldDesc("Test");
  ezWorld world(worldDesc);
  EZ_LOCK(world.GetWriteMarker());

  TestComponentManager* pManager = world.GetOrCreateComponentManager<TestComponentManager>();

  ezGameObjectDesc desc;
  ezGameObject* pObject;
  ezGameObjectHandle hObject = world.CreateObject(desc, pObject);
  EZ_TEST_BOOL(!hObject.IsInvalidated());

  ezGameObject* pObject2;
  world.CreateObject(desc, pObject2);

  TestComponent* pComponent = nullptr;

  TestComponent::s_iInitCounter = 0;
  TestComponent::s_iActivateCounter = 0;
  TestComponent::s_iSimulationStartedCounter = 0;
  TestComponent::s_bSpawnOther = false;

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Component Init")
  {
    // test recursive write lock
    EZ_LOCK(world.GetWriteMarker());

    ezComponentHandle handle;
    EZ_TEST_BOOL(!world.TryGetComponent(handle, pComponent));

    // Update with no components created
    world.Update();

    handle = TestComponent::CreateComponent(pObject, pComponent);

    TestComponent* pTest = nullptr;
    EZ_TEST_BOOL(world.TryGetComponent(handle, pTest));
    EZ_TEST_BOOL(pTest == pComponent);
    EZ_TEST_BOOL(pComponent->GetHandle() == handle);

    TestComponent2* pTest2 = nullptr;
    EZ_TEST_BOOL(!world.TryGetComponent(handle, pTest2));

    EZ_TEST_INT(pComponent->m_iSomeData, 1);
    EZ_TEST_INT(TestComponent::s_iInitCounter, 0);

    for (ezUInt32 i = 1; i < 100; ++i)
    {
      pManager->CreateComponent(pObject2, pComponent);
      pComponent->m_iSomeData = i + 1;
    }

    EZ_TEST_INT(pManager->GetComponentCount(), 100);
    EZ_TEST_INT(TestComponent::s_iInitCounter, 0);

    // Update with no components created
    world.Update();

    EZ_TEST_INT(pManager->GetComponentCount(), 100);
    EZ_TEST_INT(TestComponent::s_iInitCounter, 100);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Component Update")
  {
    // test recursive read lock
    EZ_LOCK(world.GetReadMarker());

    world.Update();

    ezUInt32 uiCounter = 0;
    for (auto it = pManager->GetComponents(); it.IsValid(); ++it)
    {
      EZ_TEST_INT(it->m_iSomeData, (((uiCounter + 4) * 25) + 3) * 25);
      ++uiCounter;
    }

    EZ_TEST_INT(uiCounter, 100);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Delete Component")
  {
    pManager->DeleteComponent(pComponent->GetHandle());
    EZ_TEST_INT(pManager->GetComponentCount(), 99);
    EZ_TEST_INT(TestComponent::s_iInitCounter, 99);

    // component should also be removed from the game object
    EZ_TEST_INT(pObject2->GetComponents().GetCount(), 98);

    world.DeleteObjectNow(pObject2->GetHandle());
    world.Update();

    EZ_TEST_INT(TestComponent::s_iInitCounter, 1);

    world.DeleteComponentManager<TestComponentManager>();
    pManager = nullptr;
    EZ_TEST_INT(TestComponent::s_iInitCounter, 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Delete Objects with Component")
  {
    ezGameObjectDesc desc;

    ezGameObject* pObjectA = nullptr;
    ezGameObject* pObjectB = nullptr;
    ezGameObject* pObjectC = nullptr;

    desc.m_sName.Assign("A");
    ezGameObjectHandle hObjectA = world.CreateObject(desc, pObjectA);
    desc.m_sName.Assign("B");
    ezGameObjectHandle hObjectB = world.CreateObject(desc, pObjectB);
    desc.m_sName.Assign("C");
    ezGameObjectHandle hObjectC = world.CreateObject(desc, pObjectC);

    EZ_TEST_BOOL(!hObjectA.IsInvalidated());
    EZ_TEST_BOOL(!hObjectB.IsInvalidated());
    EZ_TEST_BOOL(!hObjectC.IsInvalidated());

    TestComponent* pComponentA = nullptr;
    TestComponent* pComponentB = nullptr;
    TestComponent* pComponentC = nullptr;

    ezComponentHandle hComponentA = TestComponent::CreateComponent(pObjectA, pComponentA);
    ezComponentHandle hComponentB = TestComponent::CreateComponent(pObjectB, pComponentB);
    ezComponentHandle hComponentC = TestComponent::CreateComponent(pObjectC, pComponentC);

    EZ_TEST_BOOL(!hComponentA.IsInvalidated());
    EZ_TEST_BOOL(!hComponentB.IsInvalidated());
    EZ_TEST_BOOL(!hComponentC.IsInvalidated());

    world.DeleteObjectNow(pObjectB->GetHandle());

    EZ_TEST_BOOL(pObjectA->IsActive());
    EZ_TEST_BOOL(pComponentA->IsActive());
    EZ_TEST_BOOL(pComponentA->GetOwner() == pObjectA);

    EZ_TEST_BOOL(!pObjectB->IsActive());
    EZ_TEST_BOOL(!pComponentB->IsActive());
    EZ_TEST_BOOL(pComponentB->GetOwner() == nullptr);

    EZ_TEST_BOOL(pObjectC->IsActive());
    EZ_TEST_BOOL(pComponentC->IsActive());
    EZ_TEST_BOOL(pComponentC->GetOwner() == pObjectC);

    world.Update();

    EZ_TEST_BOOL(world.TryGetObject(hObjectA, pObjectA));
    EZ_TEST_BOOL(world.TryGetObject(hObjectC, pObjectC));

    // Since we're not recompacting storage for components, pointer should still be valid.
    // EZ_TEST_BOOL(world.TryGetComponent(hComponentA, pComponentA));
    // EZ_TEST_BOOL(world.TryGetComponent(hComponentC, pComponentC));

    EZ_TEST_BOOL(pObjectA->IsActive());
    EZ_TEST_STRING(pObjectA->GetName(), "A");
    EZ_TEST_BOOL(pComponentA->IsActive());
    EZ_TEST_BOOL(pComponentA->GetOwner() == pObjectA);

    EZ_TEST_BOOL(pObjectC->IsActive());
    EZ_TEST_STRING(pObjectC->GetName(), "C");
    EZ_TEST_BOOL(pComponentC->IsActive());
    EZ_TEST_BOOL(pComponentC->GetOwner() == pObjectC);

    // creating a new component should reuse memory from component B
    TestComponent* pComponentB2 = nullptr;
    ezComponentHandle hComponentB2 = TestComponent::CreateComponent(pObjectB, pComponentB2);
    EZ_TEST_BOOL(!hComponentB2.IsInvalidated());
    EZ_TEST_BOOL(pComponentB2 == pComponentB);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Get Components")
  {
    const ezWorld& constWorld = world;

    const TestComponentManager* pConstManager = constWorld.GetComponentManager<TestComponentManager>();

    for (auto it = pConstManager->GetComponents(); it.IsValid(); it.Next())
    {
      ezComponentHandle hComponent = it->GetHandle();

      const TestComponent* pConstComponent = nullptr;
      EZ_TEST_BOOL(constWorld.TryGetComponent(hComponent, pConstComponent));
      EZ_TEST_BOOL(pConstComponent == (const TestComponent*)it);

      EZ_TEST_BOOL(pConstManager->TryGetComponent(hComponent, pConstComponent));
      EZ_TEST_BOOL(pConstComponent == (const TestComponent*)it);
    }

    world.DeleteComponentManager<TestComponentManager>();
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Component Callbacks")
  {
    ezGameObjectDesc desc;
    ezGameObject* pObject = nullptr;
    world.CreateObject(desc, pObject);

    // Simulation stopped, component active
    {
      world.SetWorldSimulationEnabled(false);
      TestComponent::s_iInitCounter = 0;
      TestComponent::s_iActivateCounter = 0;
      TestComponent::s_iSimulationStartedCounter = 0;

      TestComponent* pComponent = nullptr;
      TestComponent::CreateComponent(pObject, pComponent);

      EZ_TEST_INT(TestComponent::s_iInitCounter, 0);
      EZ_TEST_INT(TestComponent::s_iActivateCounter, 0);
      EZ_TEST_INT(TestComponent::s_iSimulationStartedCounter, 0);

      world.Update();

      EZ_TEST_INT(TestComponent::s_iInitCounter, 1);
      EZ_TEST_INT(TestComponent::s_iActivateCounter, 1);
      EZ_TEST_INT(TestComponent::s_iSimulationStartedCounter, 0);

      world.SetWorldSimulationEnabled(true);
      world.Update();

      EZ_TEST_INT(TestComponent::s_iInitCounter, 1);
      EZ_TEST_INT(TestComponent::s_iActivateCounter, 1);
      EZ_TEST_INT(TestComponent::s_iSimulationStartedCounter, 1);

      pComponent->SetActive(false);

      EZ_TEST_INT(TestComponent::s_iInitCounter, 1);
      EZ_TEST_INT(TestComponent::s_iActivateCounter, 0);
      EZ_TEST_INT(TestComponent::s_iSimulationStartedCounter, 1);

      pComponent->SetActive(true);

      EZ_TEST_INT(TestComponent::s_iInitCounter, 1);
      EZ_TEST_INT(TestComponent::s_iActivateCounter, 1);
      EZ_TEST_INT(TestComponent::s_iSimulationStartedCounter, 2);

      TestComponent::DeleteComponent(pComponent);
    }

    // Simulation stopped, component inactive
    {
      world.SetWorldSimulationEnabled(false);
      TestComponent::s_iInitCounter = 0;
      TestComponent::s_iActivateCounter = 0;
      TestComponent::s_iSimulationStartedCounter = 0;

      TestComponent* pComponent = nullptr;
      TestComponent::CreateComponent(pObject, pComponent);
      pComponent->SetActive(false);

      EZ_TEST_INT(TestComponent::s_iInitCounter, 0);
      EZ_TEST_INT(TestComponent::s_iActivateCounter, 0);
      EZ_TEST_INT(TestComponent::s_iSimulationStartedCounter, 0);

      world.Update();

      EZ_TEST_INT(TestComponent::s_iInitCounter, 1);
      EZ_TEST_INT(TestComponent::s_iActivateCounter, 0);
      EZ_TEST_INT(TestComponent::s_iSimulationStartedCounter, 0);

      pComponent->SetActive(true);

      EZ_TEST_INT(TestComponent::s_iInitCounter, 1);
      EZ_TEST_INT(TestComponent::s_iActivateCounter, 1);
      EZ_TEST_INT(TestComponent::s_iSimulationStartedCounter, 0);

      pComponent->SetActive(false);

      EZ_TEST_INT(TestComponent::s_iInitCounter, 1);
      EZ_TEST_INT(TestComponent::s_iActivateCounter, 0);
      EZ_TEST_INT(TestComponent::s_iSimulationStartedCounter, 0);

      world.SetWorldSimulationEnabled(true);
      world.Update();

      EZ_TEST_INT(TestComponent::s_iInitCounter, 1);
      EZ_TEST_INT(TestComponent::s_iActivateCounter, 0);
      EZ_TEST_INT(TestComponent::s_iSimulationStartedCounter, 0);

      pComponent->SetActive(true);

      EZ_TEST_INT(TestComponent::s_iInitCounter, 1);
      EZ_TEST_INT(TestComponent::s_iActivateCounter, 1);
      EZ_TEST_INT(TestComponent::s_iSimulationStartedCounter, 1);

      TestComponent::DeleteComponent(pComponent);
    }

    // Simulation started, component active
    {
      world.SetWorldSimulationEnabled(true);
      TestComponent::s_iInitCounter = 0;
      TestComponent::s_iActivateCounter = 0;
      TestComponent::s_iSimulationStartedCounter = 0;

      TestComponent* pComponent = nullptr;
      TestComponent::CreateComponent(pObject, pComponent);

      EZ_TEST_INT(TestComponent::s_iInitCounter, 0);
      EZ_TEST_INT(TestComponent::s_iActivateCounter, 0);
      EZ_TEST_INT(TestComponent::s_iSimulationStartedCounter, 0);

      world.Update();

      EZ_TEST_INT(TestComponent::s_iInitCounter, 1);
      EZ_TEST_INT(TestComponent::s_iActivateCounter, 1);
      EZ_TEST_INT(TestComponent::s_iSimulationStartedCounter, 1);

      TestComponent::DeleteComponent(pComponent);
    }

    // Simulation started, component inactive
    {
      world.SetWorldSimulationEnabled(true);
      TestComponent::s_iInitCounter = 0;
      TestComponent::s_iActivateCounter = 0;
      TestComponent::s_iSimulationStartedCounter = 0;

      TestComponent* pComponent = nullptr;
      TestComponent::CreateComponent(pObject, pComponent);
      pComponent->SetActive(false);

      EZ_TEST_INT(TestComponent::s_iInitCounter, 0);
      EZ_TEST_INT(TestComponent::s_iActivateCounter, 0);
      EZ_TEST_INT(TestComponent::s_iSimulationStartedCounter, 0);

      world.Update();

      EZ_TEST_INT(TestComponent::s_iInitCounter, 1);
      EZ_TEST_INT(TestComponent::s_iActivateCounter, 0);
      EZ_TEST_INT(TestComponent::s_iSimulationStartedCounter, 0);

      pComponent->SetActive(true);

      EZ_TEST_INT(TestComponent::s_iInitCounter, 1);
      EZ_TEST_INT(TestComponent::s_iActivateCounter, 1);
      EZ_TEST_INT(TestComponent::s_iSimulationStartedCounter, 1);

      TestComponent::DeleteComponent(pComponent);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Component dependent initialization")
  {
    ezGameObjectDesc desc;
    ezGameObject* pObject = nullptr;
    world.CreateObject(desc, pObject);

    world.SetWorldSimulationEnabled(true);

    TestComponent::s_iInitCounter = 0;
    TestComponent::s_iActivateCounter = 0;
    TestComponent::s_iSimulationStartedCounter = 0;
    TestComponent::s_bSpawnOther = true;

    TestComponent* pComponent = nullptr;
    TestComponent::CreateComponent(pObject, pComponent);

    world.Update();

    EZ_TEST_INT(TestComponent::s_iInitCounter, 1);
    EZ_TEST_INT(TestComponent::s_iActivateCounter, 2);
    EZ_TEST_INT(TestComponent::s_iSimulationStartedCounter, 1);
  }
}
