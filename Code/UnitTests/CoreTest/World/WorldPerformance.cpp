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

  void AddObjectsToWorld(ezWorld& world, bool bDynamic, ezUInt32 uiNumObjects, ezUInt32 uiTreeLevelNumNodeDiv, ezUInt32 uiTreeDepth, ezInt32 iAttachCompsDepth,
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

    for (ezUInt32 i = 0; i < uiNumObjects; ++i)
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

      AddObjectsToWorld(world, bDynamic, ezMath::Max(uiNumObjects / uiTreeLevelNumNodeDiv, 1U), uiTreeLevelNumNodeDiv, uiTreeDepth - 1, iAttachCompsDepth - 1, hObj);
    }
  }

  void MeasureCreationTime(bool bDynamic, ezUInt32 uiNumObjects, ezUInt32 uiTreeLevelNumNodeDiv, ezUInt32 uiTreeDepth, ezInt32 iAttachCompsDepth,
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

      AddObjectsToWorld(*pWorld, bDynamic, uiNumObjects, uiTreeLevelNumNodeDiv, uiTreeDepth, iAttachCompsDepth);

      const ezTime tDiff = sw.Checkpoint();

      ezTestFramework::Output(ezTestOutput::Duration, "Creating %u %s objects (depth: %u): %.2fms", pWorld->GetObjectCount(),
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
  EZ_TEST_BLOCK(EnableInRelease, "Create many objects")
  {
    // it makes no difference whether we create static or dynamic objects
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

EZ_CREATE_SIMPLE_TEST(World, Profile_Deletion)
{
  EZ_TEST_BLOCK(EnableInRelease, "Delete many objects")
  {
    ezWorldDesc worldDesc("Test");
    ezWorld world(worldDesc);
    MeasureCreationTime(true, 10, 1, 5, 2, &world);

    ezStopwatch sw;

    EZ_LOCK(world.GetWriteMarker());
    ezUInt32 uiNumObjects = world.GetObjectCount();

    world.Clear();
    world.Update();    

    const ezTime tDiff = sw.Checkpoint();
    ezTestFramework::Output(ezTestOutput::Duration, "Deleting %u objects: %.2fms", uiNumObjects, tDiff.GetMilliseconds());
  }
}

EZ_CREATE_SIMPLE_TEST(World, Profile_Update)
{
  EZ_TEST_BLOCK(EnableInRelease, "Update 1,000,000 static objects")
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

      ezTestFramework::Output(ezTestOutput::Duration, "Updating %u objects: %.2fms", world.GetObjectCount(), tDiff.GetMilliseconds());
    }
  }

  EZ_TEST_BLOCK(EnableInRelease, "Update 100,000 dynamic objects")
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

      ezTestFramework::Output(ezTestOutput::Duration, "Updating %u objects: %.2fms", world.GetObjectCount(), tDiff.GetMilliseconds());
    }
  }

  EZ_TEST_BLOCK(EnableInRelease, "Update 100,000 dynamic objects with components")
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

      ezTestFramework::Output(ezTestOutput::Duration, "Updating %u objects: %.2fms", world.GetObjectCount(), tDiff.GetMilliseconds());
    }
  }

  EZ_TEST_BLOCK(EnableInRelease, "Update 250,000 dynamic objects")
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

      ezTestFramework::Output(ezTestOutput::Duration, "Updating %u objects: %.2fms", world.GetObjectCount(), tDiff.GetMilliseconds());
    }
  }

  EZ_TEST_BLOCK(EnableInRelease, "MT Update 250,000 dynamic objects")
  {
    ezWorldDesc worldDesc("Test");
    worldDesc.m_bAutoCreateSpatialSystem = false; // allows multi-threaded update
    ezWorld world(worldDesc);
    MeasureCreationTime(true, 200, 5, 6, 0, &world);

    ezStopwatch sw;

    // first round always has some overhead
    for (ezUInt32 i = 0; i < 3; ++i)
    {
      EZ_LOCK(world.GetWriteMarker());
      world.Update();

      const ezTime tDiff = sw.Checkpoint();

      ezTestFramework::Output(ezTestOutput::Duration, "Updating %u objects (MT): %.2fms", world.GetObjectCount(), tDiff.GetMilliseconds());
    }
  }

  EZ_TEST_BLOCK(EnableInRelease, "MT Update 1,000,000 dynamic objects")
  {
    ezWorldDesc worldDesc("Test");
    worldDesc.m_bAutoCreateSpatialSystem = false; // allows multi-threaded update
    ezWorld world(worldDesc);
    MeasureCreationTime(true, 100, 1, 3, 1, &world);

    ezStopwatch sw;

    // first round always has some overhead
    for (ezUInt32 i = 0; i < 3; ++i)
    {
      EZ_LOCK(world.GetWriteMarker());
      world.Update();

      const ezTime tDiff = sw.Checkpoint();

      ezTestFramework::Output(ezTestOutput::Duration, "Updating %u objects (MT): %.2fms", world.GetObjectCount(), tDiff.GetMilliseconds());
    }
  }
}
