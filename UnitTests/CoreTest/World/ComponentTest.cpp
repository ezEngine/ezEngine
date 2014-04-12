#include <PCH.h>
#include <Core/World/World.h>

class TestComponent;
typedef ezComponentManagerSimple<TestComponent> TestComponentManager;

class TestComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(TestComponent, TestComponentManager);

public:
  TestComponent() {}
  ~TestComponent() {}

  void Update()
  {
  }

  ezInt32 m_iSomeData;
};

EZ_BEGIN_COMPONENT_TYPE(TestComponent, ezComponent, TestComponentManager);
EZ_END_COMPONENT_TYPE();


EZ_CREATE_SIMPLE_TEST(World, Components)
{
  ezWorld world("TestComp");

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Components")
  {
    world.CreateComponentManager<TestComponentManager>();

    ezComponentHandle handle;
    TestComponent* pComponent = NULL;
    EZ_TEST_BOOL(!world.TryGetComponent(handle, pComponent));
  }
}