#include <PCH.h>
#include <Foundation/Time/Clock.h>
#include <Core/World/World.h>

namespace
{
  class TestComponent;
  class TestComponentManager : public ezComponentManager<TestComponent>
  {
  public:
    TestComponentManager(ezWorld* pWorld) : ezComponentManager<TestComponent>(pWorld)
    {
    }

    virtual ezResult Initialize() override
    {
      auto desc = EZ_CREATE_COMPONENT_UPDATE_FUNCTION_DESC(TestComponentManager::Update, this);
      auto desc2 = EZ_CREATE_COMPONENT_UPDATE_FUNCTION_DESC(TestComponentManager::Update2, this);
      desc.m_DependsOn.PushBack(desc2.m_Function); // update2 will be called before update

      auto descAsync = EZ_CREATE_COMPONENT_UPDATE_FUNCTION_DESC(TestComponentManager::UpdateAsync, this);
      descAsync.m_Phase = ezComponentManagerBase::UpdateFunctionDesc::Async;
      descAsync.m_uiGranularity = 20;

      this->RegisterUpdateFunction(desc);
      this->RegisterUpdateFunction(desc2);
      this->RegisterUpdateFunction(descAsync);

      return EZ_SUCCESS;
    }

    void Update(ezUInt32 uiStartIndex, ezUInt32 uiCount);
    void Update2(ezUInt32 uiStartIndex, ezUInt32 uiCount);
    void UpdateAsync(ezUInt32 uiStartIndex, ezUInt32 uiCount);
  };

  class TestComponent : public ezComponent
  {
    EZ_DECLARE_COMPONENT_TYPE(TestComponent, TestComponentManager);

  public:
    TestComponent() : m_iSomeData(1) {}
    ~TestComponent() {}

    virtual ezResult Initialize() override
    {
      ++s_iInitCounter;
      return EZ_SUCCESS;
    }

    virtual ezResult Deinitialize() override
    {
      --s_iInitCounter;
      return EZ_SUCCESS;
    }

    virtual ezResult OnAttachedToObject() override
    {
      ++s_iAttachCounter;
      return EZ_SUCCESS;
    }

    virtual ezResult OnDetachedFromObject() override
    {
      --s_iAttachCounter;
      return EZ_SUCCESS;
    }

    void Update()
    {
      m_iSomeData *= 5;
    }

    void Update2()
    {
      m_iSomeData += 3;
    }

    ezInt32 m_iSomeData;

    static ezInt32 s_iInitCounter;
    static ezInt32 s_iAttachCounter;
  };

  ezInt32 TestComponent::s_iInitCounter = 0;
  ezInt32 TestComponent::s_iAttachCounter = 0;

  EZ_BEGIN_COMPONENT_TYPE(TestComponent, ezComponent, 1, TestComponentManager);
  EZ_END_COMPONENT_TYPE();

  void TestComponentManager::Update(ezUInt32 uiStartIndex, ezUInt32 uiCount)
  {
    for (auto it = this->m_ComponentStorage.GetIterator(uiStartIndex, uiCount); it.IsValid(); ++it)
    {
      if (it->IsActive())
        it->Update();
    }
  }

  void TestComponentManager::Update2(ezUInt32 uiStartIndex, ezUInt32 uiCount)
  {
    for (auto it = this->m_ComponentStorage.GetIterator(uiStartIndex, uiCount); it.IsValid(); ++it)
    {
      if (it->IsActive())
        it->Update2();
    }
  }

  void TestComponentManager::UpdateAsync(ezUInt32 uiStartIndex, ezUInt32 uiCount)
  {
    for (auto it = this->m_ComponentStorage.GetIterator(uiStartIndex, uiCount); it.IsValid(); ++it)
    {
      if (it->IsActive())
        it->Update();
    }
  }
}


EZ_CREATE_SIMPLE_TEST(World, Components)
{
  ezClock::SetNumGlobalClocks();

  ezWorld world("TestComp");
  world.CreateComponentManager<TestComponentManager>();
  TestComponentManager* pManager = world.GetComponentManager<TestComponentManager>();

  TestComponent* pComponent = nullptr;
  ezGameObject* pObject = nullptr;

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Component Init")
  {
    ezComponentHandle handle;
    EZ_TEST_BOOL(!world.TryGetComponent(handle, pComponent));

    // Update with no components created
    world.Update();

    handle = TestComponent::CreateComponent(&world, pComponent);
    TestComponent* pTest = nullptr;
    EZ_TEST_BOOL(world.TryGetComponent(handle, pTest));
    EZ_TEST_BOOL(pTest == pComponent);
    EZ_TEST_BOOL(pComponent->GetHandle() == handle);

    EZ_TEST_INT(pComponent->m_iSomeData, 1);
    EZ_TEST_INT(TestComponent::s_iInitCounter, 1);

    for (ezUInt32 i = 1; i < 100; ++i)
    {
      pManager->CreateComponent(pComponent);
      pComponent->m_iSomeData = i + 1;
    }

    EZ_TEST_INT(pManager->GetComponentCount(), 100);
    EZ_TEST_INT(TestComponent::s_iInitCounter, 100);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Component Update")
  {
    world.Update();

    ezUInt32 uiCounter = 0;
    for (auto it = pManager->GetComponents(); it.IsValid(); ++it)
    {
      EZ_TEST_INT(it->m_iSomeData, (uiCounter + 4) * 25);
      ++uiCounter;
    }

    EZ_TEST_INT(uiCounter, 100);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Attach to objects")
  {
    world.CreateObject(ezGameObjectDesc(), pObject);

    EZ_TEST_INT(TestComponent::s_iAttachCounter, 0);

    for (auto it = pManager->GetComponents(); it.IsValid(); ++it)
    {
      pObject->AddComponent(it);
      EZ_TEST_BOOL(it->GetOwner() != nullptr);
    }

    EZ_TEST_INT(TestComponent::s_iAttachCounter, 100);
    EZ_TEST_INT(pObject->GetComponents().GetCount(), 100);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Detach from objects")
  {
    ezComponent* pFirstComponent = pObject->GetComponents()[0];
    pObject->RemoveComponent(pFirstComponent);

    EZ_TEST_INT(TestComponent::s_iInitCounter, 100);
    EZ_TEST_INT(TestComponent::s_iAttachCounter, 99);
    EZ_TEST_INT(pObject->GetComponents().GetCount(), 99);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Delete Component")
  {
    pManager->DeleteComponent(pComponent->GetHandle());
    EZ_TEST_INT(pManager->GetComponentCount(), 99);
    EZ_TEST_INT(TestComponent::s_iInitCounter, 99);
    EZ_TEST_INT(TestComponent::s_iAttachCounter, 98);

    // component should also be removed from the game object
    EZ_TEST_INT(pObject->GetComponents().GetCount(), 98);

    world.DeleteObject(pObject->GetHandle());
    world.Update();

    EZ_TEST_INT(TestComponent::s_iInitCounter, 1);
    EZ_TEST_INT(TestComponent::s_iAttachCounter, 0);

    world.DeleteComponentManager<TestComponentManager>();
    EZ_TEST_INT(TestComponent::s_iInitCounter, 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Delete Objects with Component")
  {
    world.CreateComponentManager<TestComponentManager>();

    ezGameObject* pObjectA = nullptr;
    ezGameObject* pObjectB = nullptr;
    ezGameObject* pObjectC = nullptr;
    ezGameObjectHandle hObjectA = world.CreateObject(ezGameObjectDesc(), pObjectA);
    ezGameObjectHandle hObjectB = world.CreateObject(ezGameObjectDesc(), pObjectB);
    ezGameObjectHandle hObjectC = world.CreateObject(ezGameObjectDesc(), pObjectC);

    TestComponent* pComponentA = nullptr;
    TestComponent* pComponentB = nullptr;
    TestComponent* pComponentC = nullptr;

    ezComponentHandle hComponentA = TestComponent::CreateComponent(&world, pComponentA);
    ezComponentHandle hComponentB = TestComponent::CreateComponent(&world, pComponentB);
    ezComponentHandle hComponentC = TestComponent::CreateComponent(&world, pComponentC);

    pObjectA->AddComponent(pComponentA);
    pObjectB->AddComponent(pComponentB);
    pObjectC->AddComponent(pComponentC);

    world.DeleteObject(pObjectB->GetHandle());

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
    //EZ_TEST_BOOL(world.TryGetComponent(hComponentA, pComponentA));
    //EZ_TEST_BOOL(world.TryGetComponent(hComponentC, pComponentC));

    EZ_TEST_BOOL(pObjectA->IsActive());
    EZ_TEST_BOOL(pComponentA->IsActive());
    EZ_TEST_BOOL(pComponentA->GetOwner() == pObjectA);

    EZ_TEST_BOOL(pObjectC->IsActive());
    EZ_TEST_BOOL(pComponentC->IsActive());
    EZ_TEST_BOOL(pComponentC->GetOwner() == pObjectC);

    // creating a new component should reuse memory from component B
    TestComponent* pComponentB2 = nullptr;
    ezComponentHandle hComponentB2 = TestComponent::CreateComponent(&world, pComponentB2);
    EZ_TEST_BOOL(pComponentB2 == pComponentB);
  }
}