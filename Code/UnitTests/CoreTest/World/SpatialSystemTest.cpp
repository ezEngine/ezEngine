#include <CoreTest/CoreTestPCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/World/World.h>
#include <Foundation/Containers/HashSet.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Profiling/ProfilingUtils.h>
#include <Foundation/Utilities/GraphicsUtils.h>

namespace
{
  static ezSpatialData::Category s_SpecialTestCategory = ezSpatialData::RegisterCategory("SpecialTestCategory", ezSpatialData::Flags::None);

  using TestBoundsComponentManager = ezComponentManager<class TestBoundsComponent, ezBlockStorageType::Compact>;

  class TestBoundsComponent : public ezComponent
  {
    EZ_DECLARE_COMPONENT_TYPE(TestBoundsComponent, ezComponent, TestBoundsComponentManager);

  public:
    virtual void Initialize() override { GetOwner()->UpdateLocalBounds(); }

    void OnUpdateLocalBounds(ezMsgUpdateLocalBounds& ref_msg)
    {
      auto& rng = GetWorld()->GetRandomNumberGenerator();

      float x = (float)rng.DoubleMinMax(1.0, 100.0);
      float y = (float)rng.DoubleMinMax(1.0, 100.0);
      float z = (float)rng.DoubleMinMax(1.0, 100.0);

      ezBoundingBox bounds = ezBoundingBox::MakeFromCenterAndHalfExtents(ezVec3::MakeZero(), ezVec3(x, y, z));

      ezSpatialData::Category category = m_SpecialCategory;
      if (category == ezInvalidSpatialDataCategory)
      {
        category = GetOwner()->IsDynamic() ? ezDefaultSpatialDataCategories::RenderDynamic : ezDefaultSpatialDataCategories::RenderStatic;
      }

      ref_msg.AddBounds(ezBoundingBoxSphere::MakeFromBox(bounds), category);
    }

    ezSpatialData::Category m_SpecialCategory = ezInvalidSpatialDataCategory;
  };

  // clang-format off
  EZ_BEGIN_COMPONENT_TYPE(TestBoundsComponent, 1, ezComponentMode::Static)
  {
    EZ_BEGIN_MESSAGEHANDLERS
    {
      EZ_MESSAGE_HANDLER(ezMsgUpdateLocalBounds, OnUpdateLocalBounds)
    }
    EZ_END_MESSAGEHANDLERS;
  }
  EZ_END_COMPONENT_TYPE;
  // clang-format on
} // namespace

EZ_CREATE_SIMPLE_TEST(World, SpatialSystem)
{
  ezWorldDesc worldDesc("Test");
  worldDesc.m_uiRandomNumberGeneratorSeed = 5;

  ezWorld world(worldDesc);
  EZ_LOCK(world.GetWriteMarker());

  auto& rng = world.GetRandomNumberGenerator();

  ezDynamicArray<ezGameObject*> objects;
  objects.Reserve(1000);

  for (ezUInt32 i = 0; i < 1000; ++i)
  {
    constexpr const double range = 10000.0;

    float x = (float)rng.DoubleMinMax(-range, range);
    float y = (float)rng.DoubleMinMax(-range, range);
    float z = (float)rng.DoubleMinMax(-range, range);

    ezGameObjectDesc desc;
    desc.m_bDynamic = (i >= 500);
    desc.m_LocalPosition = ezVec3(x, y, z);

    ezGameObject* pObject = nullptr;
    world.CreateObject(desc, pObject);

    objects.PushBack(pObject);

    TestBoundsComponent* pComponent = nullptr;
    TestBoundsComponent::CreateComponent(pObject, pComponent);
  }

  world.Update();

  ezSpatialSystem::QueryParams queryParams;
  queryParams.m_uiCategoryBitmask = ezDefaultSpatialDataCategories::RenderStatic.GetBitmask();

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "FindObjectsInSphere")
  {
    ezBoundingSphere testSphere = ezBoundingSphere::MakeFromCenterAndRadius(ezVec3(100.0f, 60.0f, 400.0f), 3000.0f);

    ezDynamicArray<ezGameObject*> objectsInSphere;
    ezHashSet<ezGameObject*> uniqueObjects;
    world.GetSpatialSystem()->FindObjectsInSphere(testSphere, queryParams, objectsInSphere);

    for (auto pObject : objectsInSphere)
    {
      ezBoundingSphere objSphere = pObject->GetGlobalBounds().GetSphere();

      EZ_TEST_BOOL(testSphere.Overlaps(objSphere));
      EZ_TEST_BOOL(!uniqueObjects.Insert(pObject));
      EZ_TEST_BOOL(pObject->IsStatic());
    }

    // Check for missing objects
    for (auto it = world.GetObjects(); it.IsValid(); ++it)
    {
      ezBoundingSphere objSphere = it->GetGlobalBounds().GetSphere();
      if (testSphere.Overlaps(objSphere))
      {
        EZ_TEST_BOOL(it->IsDynamic() || uniqueObjects.Contains((ezGameObject*)it));
      }
    }

    objectsInSphere.Clear();
    uniqueObjects.Clear();

    world.GetSpatialSystem()->FindObjectsInSphere(testSphere, queryParams, [&](ezGameObject* pObject)
      {
      objectsInSphere.PushBack(pObject);
      EZ_TEST_BOOL(!uniqueObjects.Insert(pObject));

      return ezVisitorExecution::Continue; });

    for (auto pObject : objectsInSphere)
    {
      ezBoundingSphere objSphere = pObject->GetGlobalBounds().GetSphere();

      EZ_TEST_BOOL(testSphere.Overlaps(objSphere));
      EZ_TEST_BOOL(pObject->IsStatic());
    }

    // Check for missing objects
    for (auto it = world.GetObjects(); it.IsValid(); ++it)
    {
      ezBoundingSphere objSphere = it->GetGlobalBounds().GetSphere();
      if (testSphere.Overlaps(objSphere))
      {
        EZ_TEST_BOOL(it->IsDynamic() || uniqueObjects.Contains((ezGameObject*)it));
      }
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "FindObjectsInBox")
  {
    ezBoundingBox testBox = ezBoundingBox::MakeFromCenterAndHalfExtents(ezVec3(100.0f, 60.0f, 400.0f), ezVec3(3000.0f));

    ezDynamicArray<ezGameObject*> objectsInBox;
    ezHashSet<ezGameObject*> uniqueObjects;
    world.GetSpatialSystem()->FindObjectsInBox(testBox, queryParams, objectsInBox);

    for (auto pObject : objectsInBox)
    {
      ezBoundingBox objBox = pObject->GetGlobalBounds().GetBox();

      EZ_TEST_BOOL(testBox.Overlaps(objBox));
      EZ_TEST_BOOL(!uniqueObjects.Insert(pObject));
      EZ_TEST_BOOL(pObject->IsStatic());
    }

    // Check for missing objects
    for (auto it = world.GetObjects(); it.IsValid(); ++it)
    {
      ezBoundingBox objBox = it->GetGlobalBounds().GetBox();
      if (testBox.Overlaps(objBox))
      {
        EZ_TEST_BOOL(it->IsDynamic() || uniqueObjects.Contains((ezGameObject*)it));
      }
    }

    objectsInBox.Clear();
    uniqueObjects.Clear();

    world.GetSpatialSystem()->FindObjectsInBox(testBox, queryParams, [&](ezGameObject* pObject)
      {
      objectsInBox.PushBack(pObject);
      EZ_TEST_BOOL(!uniqueObjects.Insert(pObject));

      return ezVisitorExecution::Continue; });

    for (auto pObject : objectsInBox)
    {
      ezBoundingSphere objSphere = pObject->GetGlobalBounds().GetSphere();

      EZ_TEST_BOOL(testBox.Overlaps(objSphere));
      EZ_TEST_BOOL(pObject->IsStatic());
    }

    // Check for missing objects
    for (auto it = world.GetObjects(); it.IsValid(); ++it)
    {
      ezBoundingBox objBox = it->GetGlobalBounds().GetBox();
      if (testBox.Overlaps(objBox))
      {
        EZ_TEST_BOOL(it->IsDynamic() || uniqueObjects.Contains((ezGameObject*)it));
      }
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "FindVisibleObjects")
  {
    constexpr uint32_t numUpdates = 13;

    // update a few times to increase internal frame counter
    for (uint32_t i = 0; i < numUpdates; ++i)
    {
      world.Update();
    }

    queryParams.m_uiCategoryBitmask = ezDefaultSpatialDataCategories::RenderDynamic.GetBitmask();

    ezMat4 lookAt = ezGraphicsUtils::CreateLookAtViewMatrix(ezVec3::MakeZero(), ezVec3::MakeAxisX(), ezVec3::MakeAxisZ());
    ezMat4 projection = ezGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovX(ezAngle::MakeFromDegree(80.0f), 1.0f, 1.0f, 10000.0f);

    ezFrustum testFrustum = ezFrustum::MakeFromMVP(projection * lookAt);

    ezDynamicArray<const ezGameObject*> visibleObjects;
    ezHashSet<const ezGameObject*> uniqueObjects;
    world.GetSpatialSystem()->FindVisibleObjects(testFrustum, queryParams, visibleObjects, {}, ezVisibilityState::Direct);

    EZ_TEST_BOOL(!visibleObjects.IsEmpty());

    for (auto pObject : visibleObjects)
    {
      EZ_TEST_BOOL(testFrustum.Overlaps(pObject->GetGlobalBoundsSimd().GetSphere()));
      EZ_TEST_BOOL(!uniqueObjects.Insert(pObject));
      EZ_TEST_BOOL(pObject->IsDynamic());

      ezVisibilityState::Enum visType = pObject->GetVisibilityState();
      EZ_TEST_BOOL(visType == ezVisibilityState::Direct);
    }

    // Check for missing objects
    for (auto it = world.GetObjects(); it.IsValid(); ++it)
    {
      ezGameObject* pObject = it;

      if (testFrustum.GetObjectPosition(pObject->GetGlobalBounds().GetSphere()) == ezVolumePosition::Outside)
      {
        ezVisibilityState::Enum visType = pObject->GetVisibilityState();
        EZ_TEST_BOOL(visType == ezVisibilityState::Invisible);
      }
    }

    // Move some objects
    for (auto it = world.GetObjects(); it.IsValid(); ++it)
    {
      constexpr const double range = 500.0f;

      if (it->IsDynamic())
      {
        ezVec3 pos = it->GetLocalPosition();

        pos.x += (float)rng.DoubleMinMax(-range, range);
        pos.y += (float)rng.DoubleMinMax(-range, range);
        pos.z += (float)rng.DoubleMinMax(-range, range);

        it->SetLocalPosition(pos);
      }
    }

    world.Update();

    // Check that last frame visible doesn't reset entirely after moving
    for (const ezGameObject* pObject : visibleObjects)
    {
      ezVisibilityState::Enum visType = pObject->GetVisibilityState();
      EZ_TEST_BOOL(visType == ezVisibilityState::Direct);
    }
  }

  if (false)
  {
    ezStringBuilder outputPath = ezTestFramework::GetInstance()->GetAbsOutputPath();
    EZ_TEST_BOOL(ezFileSystem::AddDataDirectory(outputPath.GetData(), "test", "output", ezDataDirUsage::AllowWrites) == EZ_SUCCESS);

    ezProfilingUtils::SaveProfilingCapture(":output/profiling.json").IgnoreResult();
  }

  // Test multiple categories for spatial data
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "MultipleCategories")
  {
    for (ezUInt32 i = 0; i < objects.GetCount(); ++i)
    {
      ezGameObject* pObject = objects[i];

      TestBoundsComponent* pComponent = nullptr;
      TestBoundsComponent::CreateComponent(pObject, pComponent);
      pComponent->m_SpecialCategory = s_SpecialTestCategory;
    }

    world.Update();

    ezDynamicArray<ezGameObjectHandle> allObjects;
    allObjects.Reserve(world.GetObjectCount());

    for (auto it = world.GetObjects(); it.IsValid(); ++it)
    {
      allObjects.PushBack(it->GetHandle());
    }

    for (ezUInt32 i = allObjects.GetCount(); i-- > 0;)
    {
      world.DeleteObjectNow(allObjects[i]);
    }

    world.Update();
  }
}
