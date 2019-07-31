#include <CoreTestPCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/World/World.h>
#include <Foundation/Containers/HashSet.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Profiling/Profiling.h>

namespace
{
  static ezSpatialData::Category s_SpecialTestCategory = ezSpatialData::RegisterCategory("SpecialTestCategory");

  typedef ezComponentManager<class TestBoundsComponent, ezBlockStorageType::Compact> TestBoundsComponentManager;

  class TestBoundsComponent : public ezComponent
  {
    EZ_DECLARE_COMPONENT_TYPE(TestBoundsComponent, ezComponent, TestBoundsComponentManager);

  public:
    virtual void Initialize() override { GetOwner()->UpdateLocalBounds(); }

    void OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg)
    {
      auto& rng = GetWorld()->GetRandomNumberGenerator();

      float x = (float)rng.DoubleMinMax(1.0, 100.0);
      float y = (float)rng.DoubleMinMax(1.0, 100.0);
      float z = (float)rng.DoubleMinMax(1.0, 100.0);

      ezBoundingBox bounds;
      bounds.SetCenterAndHalfExtents(ezVec3::ZeroVector(), ezVec3(x, y, z));

      ezSpatialData::Category category = m_SpecialCategory;
      if (category == ezInvalidSpatialDataCategory)
      {
        category = GetOwner()->IsDynamic() ? ezDefaultSpatialDataCategories::RenderDynamic : ezDefaultSpatialDataCategories::RenderStatic;
      }

      msg.AddBounds(bounds, category);
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
  double range = 10000.0;

  ezDynamicArray<ezGameObject*> objects;
  objects.Reserve(1000);

  for (ezUInt32 i = 0; i < 1000; ++i)
  {
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

  ezUInt32 uiCategoryBitmask = ezDefaultSpatialDataCategories::RenderStatic.GetBitmask();

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "FindObjectsInSphere")
  {
    ezBoundingSphere testSphere(ezVec3(100.0f, 60.0f, 400.0f), 3000.0f);

    ezDynamicArray<ezGameObject*> objectsInSphere;
    ezHashSet<ezGameObject*> uniqueObjects;
    world.GetSpatialSystem().FindObjectsInSphere(testSphere, uiCategoryBitmask, objectsInSphere);

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
        EZ_TEST_BOOL(it->IsDynamic() || uniqueObjects.Contains(it));
      }
    }

    objectsInSphere.Clear();
    uniqueObjects.Clear();

    world.GetSpatialSystem().FindObjectsInSphere(testSphere, uiCategoryBitmask, [&](ezGameObject* pObject) {
      objectsInSphere.PushBack(pObject);
      EZ_TEST_BOOL(!uniqueObjects.Insert(pObject));

      return ezVisitorExecution::Continue;
    });

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
        EZ_TEST_BOOL(it->IsDynamic() || uniqueObjects.Contains(it));        
      }
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "FindObjectsInBox")
  {
    ezBoundingBox testBox;
    testBox.SetCenterAndHalfExtents(ezVec3(100.0f, 60.0f, 400.0f), ezVec3(3000.0f));

    ezDynamicArray<ezGameObject*> objectsInBox;
    ezHashSet<ezGameObject*> uniqueObjects;
    world.GetSpatialSystem().FindObjectsInBox(testBox, uiCategoryBitmask, objectsInBox);

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
        EZ_TEST_BOOL(it->IsDynamic() || uniqueObjects.Contains(it));
      }
    }

    objectsInBox.Clear();
    uniqueObjects.Clear();

    world.GetSpatialSystem().FindObjectsInBox(testBox, uiCategoryBitmask, [&](ezGameObject* pObject) {
      objectsInBox.PushBack(pObject);
      EZ_TEST_BOOL(!uniqueObjects.Insert(pObject));

      return ezVisitorExecution::Continue;
    });

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
        EZ_TEST_BOOL(it->IsDynamic() || uniqueObjects.Contains(it));
      }
    }
  }

  if (false)
  {
    ezStringBuilder outputPath = ezTestFramework::GetInstance()->GetAbsOutputPath();
    ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);

    EZ_TEST_BOOL(ezFileSystem::AddDataDirectory(outputPath.GetData(), "test", "output", ezFileSystem::AllowWrites) == EZ_SUCCESS);

    ezFileWriter fileWriter;
    if (fileWriter.Open(":output/profiling.json") == EZ_SUCCESS)
    {
      ezProfilingSystem::ProfilingData profilingData = ezProfilingSystem::Capture();
      profilingData.Write(fileWriter);
      ezLog::Info("Profiling capture saved to '{0}'.", fileWriter.GetFilePathAbsolute().GetData());
    }
  }

  // Test multiple categories for spatial data
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
