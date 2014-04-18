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

    virtual ezResult Initialize() EZ_OVERRIDE
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

    void Update()
    {
      m_iSomeData *= 5;
    }

    void Update2()
    {
      m_iSomeData += 3;
    }

    ezInt32 m_iSomeData;
  };

  EZ_BEGIN_COMPONENT_TYPE(TestComponent, ezComponent, TestComponentManager);
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

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Component Update")
  {
    ezWorld world("TestComp");
    world.CreateComponentManager<TestComponentManager>();

    ezComponentHandle handle;
    TestComponent* pComponent = nullptr;
    EZ_TEST_BOOL(!world.TryGetComponent(handle, pComponent));

    TestComponentManager* pManager = world.GetComponentManager<TestComponentManager>();
    handle = pManager->CreateComponent(pComponent);
    TestComponent* pTest = nullptr;
    EZ_TEST_BOOL(world.TryGetComponent(handle, pTest));
    EZ_TEST_BOOL(pTest == pComponent);
    EZ_TEST_BOOL(pComponent->GetHandle() == handle);

    EZ_TEST_INT(pComponent->m_iSomeData, 1);

    for (ezUInt32 i = 1; i < 100; ++i)
    {
      pManager->CreateComponent(pComponent);
      pComponent->m_iSomeData = i + 1;
    }

    world.Update();

    ezUInt32 uiCounter = 0;
    for (auto it = pManager->GetComponents(); it.IsValid(); ++it)
    {
      EZ_TEST_INT(it->m_iSomeData, (uiCounter + 4) * 25);
      ++uiCounter;
    }

    EZ_TEST_INT(uiCounter, 100);
  }
}