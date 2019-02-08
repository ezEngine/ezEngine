#include <CoreTestPCH.h>

#include <Core/World/World.h>
#include <Foundation/Time/Clock.h>
#include <Foundation/Time/Stopwatch.h>

namespace
{
  class ezTestComponentManager;

  class ezTestComponent : public ezComponent
  {
    EZ_DECLARE_COMPONENT_TYPE(ezTestComponent, ezComponent, ezTestComponentManager);
  };

  class ezTestComponentManager : public ezComponentManager<class ezTestComponent, ezBlockStorageType::FreeList>
  {
  public:
    ezTestComponentManager(ezWorld* pWorld)
        : ezComponentManager<ezTestComponent, ezBlockStorageType::FreeList>(pWorld)
    {
      m_qRotation.SetIdentity();
    }

    virtual void Initialize() override
    {
      auto desc = ezWorldModule::UpdateFunctionDesc(ezWorldModule::UpdateFunction(&ezTestComponentManager::Update, this), "Update");
      desc.m_bOnlyUpdateWhenSimulating = false;

      RegisterUpdateFunction(desc);
    }

    void Update(const ezWorldModule::UpdateContext& context)
    {
      ezQuat qRot;
      qRot.SetFromAxisAndAngle(ezVec3(0, 0, 1), ezAngle::Degree(2.0f));

      m_qRotation = qRot * m_qRotation;

      for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
      {
        ComponentType* pComponent = it;
        if (pComponent->IsActiveAndInitialized())
        {
          auto pOwner = pComponent->GetOwner();
          pOwner->SetLocalRotation(m_qRotation);
        }
      }
    }

    ezQuat m_qRotation;
  };

  // clang-format off
  EZ_BEGIN_COMPONENT_TYPE(ezTestComponent, 1, ezComponentMode::Dynamic);
  EZ_END_COMPONENT_TYPE;
  // clang-format on

  void AddNodesToWorld(ezWorld& world, bool bDynamic, ezUInt32 uiNumNodes, ezUInt32 uiTreeLevelNumNodeDiv, ezUInt32 uiTreeDepth, ezInt32 iAttachCompsDepth,
                       ezGameObjectHandle hParent = ezGameObjectHandle())
  {
    if (uiTreeDepth == 0)
      return;

    ezGameObjectDesc gd;
    gd.m_bDynamic = bDynamic;
    gd.m_hParent = hParent;

    float posX = 0.0f;
    float posY = uiTreeDepth * 5.0f;

    ezTestComponentManager* pMan = world.GetOrCreateComponentManager<ezTestComponentManager>();

    for (ezUInt32 i = 0; i < uiNumNodes; ++i)
    {
      gd.m_LocalPosition.Set(posX, posY, 0);
      posX += 5.0f;

      ezGameObject* pObj;
      auto hObj = world.CreateObject(gd, pObj);

      if (iAttachCompsDepth > 0)
      {
        ezTestComponent* comp;
        pMan->CreateComponent(pObj, comp);
      }

      AddNodesToWorld(world, bDynamic, ezMath::Max(uiNumNodes / uiTreeLevelNumNodeDiv, 1U), uiTreeLevelNumNodeDiv, uiTreeDepth - 1, iAttachCompsDepth - 1, hObj);
    }
  }

  void MeasureCreationTime(bool bDynamic, ezUInt32 uiNumNodes, ezUInt32 uiTreeLevelNumNodeDiv, ezUInt32 uiTreeDepth, ezInt32 iAttachCompsDepth,
                           ezWorld* pWorld = nullptr)
  {
    ezWorldDesc worldDesc("Test");
    ezWorld world(worldDesc);

    if (pWorld == nullptr)
    {
      pWorld = &world;
    }

    EZ_LOCK(pWorld->GetWriteMarker());

    {
      ezStopwatch sw;

      AddNodesToWorld(*pWorld, bDynamic, uiNumNodes, uiTreeLevelNumNodeDiv, uiTreeDepth, iAttachCompsDepth);

      const ezTime tDiff = sw.Checkpoint();

      ezTestFramework::Output(ezTestOutput::Duration, "Creating %u %s nodes (depth: %u): %.2fms", pWorld->GetObjectCount(),
                              bDynamic ? "dynamic" : "static", uiTreeDepth, tDiff.GetMilliseconds());
    }
  }

} // namespace


#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
static const ezTestBlock::Enum EnableInRelease = ezTestBlock::DisabledNoWarning;
#else
static const ezTestBlock::Enum EnableInRelease = ezTestBlock::Enabled;
#endif

EZ_CREATE_SIMPLE_TEST(World, Profile_Creation)
{
  EZ_TEST_BLOCK(EnableInRelease, "Create many nodes")
  {
    // it makes no difference whether we create static or dynamic nodes
    static bool bDynamic = true;
    bDynamic = !bDynamic;

    MeasureCreationTime(bDynamic, 10, 1, 4, 0);
    MeasureCreationTime(bDynamic, 10, 1, 5, 0);
    MeasureCreationTime(bDynamic, 100, 1, 2, 0);
    MeasureCreationTime(bDynamic, 10000, 1, 1, 0);
    MeasureCreationTime(bDynamic, 100000, 1, 1, 0);
    MeasureCreationTime(bDynamic, 1000000, 1, 1, 0);
    MeasureCreationTime(bDynamic, 100, 1, 3, 0);
    MeasureCreationTime(bDynamic, 3, 1, 12, 0);
    MeasureCreationTime(bDynamic, 1, 1, 80, 0);
  }
}

EZ_CREATE_SIMPLE_TEST(World, Profile_Update)
{
  EZ_TEST_BLOCK(EnableInRelease, "Update 1,000,000 static nodes")
  {
    ezWorldDesc worldDesc("Test");
    ezWorld world(worldDesc);
    MeasureCreationTime(false, 100, 1, 3, 0, &world);

    ezStopwatch sw;

    // first round always has some overhead
    for (ezUInt32 i = 0; i < 3; ++i)
    {
      EZ_LOCK(world.GetWriteMarker());
      world.Update();

      const ezTime tDiff = sw.Checkpoint();

      ezTestFramework::Output(ezTestOutput::Duration, "Updating %u nodes: %.2fms", world.GetObjectCount(), tDiff.GetMilliseconds());
    }
  }

  EZ_TEST_BLOCK(EnableInRelease, "Update 100,000 dynamic nodes")
  {
    ezWorldDesc worldDesc("Test");
    ezWorld world(worldDesc);
    MeasureCreationTime(true, 10, 1, 5, 0, &world);

    ezStopwatch sw;

    // first round always has some overhead
    for (ezUInt32 i = 0; i < 3; ++i)
    {
      EZ_LOCK(world.GetWriteMarker());
      world.Update();

      const ezTime tDiff = sw.Checkpoint();

      ezTestFramework::Output(ezTestOutput::Duration, "Updating %u nodes: %.2fms", world.GetObjectCount(), tDiff.GetMilliseconds());
    }
  }

  EZ_TEST_BLOCK(EnableInRelease, "Update 100,000 dynamic nodes with components")
  {
    ezWorldDesc worldDesc("Test");
    ezWorld world(worldDesc);
    MeasureCreationTime(true, 10, 1, 5, 2, &world);

    ezStopwatch sw;

    // first round always has some overhead
    for (ezUInt32 i = 0; i < 3; ++i)
    {
      EZ_LOCK(world.GetWriteMarker());
      world.Update();

      const ezTime tDiff = sw.Checkpoint();

      ezTestFramework::Output(ezTestOutput::Duration, "Updating %u nodes: %.2fms", world.GetObjectCount(), tDiff.GetMilliseconds());
    }
  }

  EZ_TEST_BLOCK(EnableInRelease, "Update 250,000 dynamic nodes")
  {
    ezWorldDesc worldDesc("Test");
    ezWorld world(worldDesc);
    MeasureCreationTime(true, 200, 5, 6, 0, &world);

    ezStopwatch sw;

    // first round always has some overhead
    for (ezUInt32 i = 0; i < 3; ++i)
    {
      EZ_LOCK(world.GetWriteMarker());
      world.Update();

      const ezTime tDiff = sw.Checkpoint();

      ezTestFramework::Output(ezTestOutput::Duration, "Updating %u nodes: %.2fms", world.GetObjectCount(), tDiff.GetMilliseconds());
    }
  }

  EZ_TEST_BLOCK(EnableInRelease, "Update 1,000,000 dynamic nodes")
  {
    ezWorldDesc worldDesc("Test");
    ezWorld world(worldDesc);
    MeasureCreationTime(true, 100, 1, 3, 1, &world);

    ezStopwatch sw;

    // first round always has some overhead
    for (ezUInt32 i = 0; i < 3; ++i)
    {
      EZ_LOCK(world.GetWriteMarker());
      world.Update();

      const ezTime tDiff = sw.Checkpoint();

      ezTestFramework::Output(ezTestOutput::Duration, "Updating %u nodes: %.2fms", world.GetObjectCount(), tDiff.GetMilliseconds());
    }
  }
}
