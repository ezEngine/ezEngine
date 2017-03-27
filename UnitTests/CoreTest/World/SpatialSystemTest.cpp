#include <PCH.h>
#include <Foundation/Containers/HashSet.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/Profiling/Profiling.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/World/World.h>

namespace
{
  typedef ezComponentManager<class TestBoundsComponent, ezBlockStorageType::Compact> TestBoundsComponentManager;

  class TestBoundsComponent : public ezComponent
  {
    EZ_DECLARE_COMPONENT_TYPE(TestBoundsComponent, ezComponent, TestBoundsComponentManager);

  public:
    virtual void Initialize() override
    {
      GetOwner()->UpdateLocalBounds();
    }

    void OnUpdateLocalBounds(ezUpdateLocalBoundsMessage& msg)
    {
      auto& rng = GetWorld()->GetRandomNumberGenerator();

      float x = (float)rng.DoubleMinMax(1.0, 100.0);
      float y = (float)rng.DoubleMinMax(1.0, 100.0);
      float z = (float)rng.DoubleMinMax(1.0, 100.0);

      ezBoundingBox bounds;
      bounds.SetCenterAndHalfExtents(ezVec3::ZeroVector(), ezVec3(x, y, z));

      msg.AddBounds(bounds);
    }
  };

  EZ_BEGIN_COMPONENT_TYPE(TestBoundsComponent, 1)
  {
    EZ_BEGIN_MESSAGEHANDLERS
    {
      EZ_MESSAGE_HANDLER(ezUpdateLocalBoundsMessage, OnUpdateLocalBounds)
    }
    EZ_END_MESSAGEHANDLERS
  }
  EZ_END_COMPONENT_TYPE
}

EZ_CREATE_SIMPLE_TEST(World, SpatialSystem)
{
  ezWorldDesc worldDesc("Test");
  worldDesc.m_uiRandomNumberGeneratorSeed = 5;

  ezWorld world(worldDesc);
  EZ_LOCK(world.GetWriteMarker());

  auto& rng = world.GetRandomNumberGenerator();
  double range = 10000.0;

  for (ezUInt32 i = 0; i < 1000; ++i)
  {
    float x = (float)rng.DoubleMinMax(-range, range);
    float y = (float)rng.DoubleMinMax(-range, range);
    float z = (float)rng.DoubleMinMax(-range, range);

    ezGameObjectDesc desc;
    desc.m_LocalPosition = ezVec3(x, y, z);

    ezGameObject* pObject = nullptr;
    world.CreateObject(desc, pObject);

    TestBoundsComponent* pComponent = nullptr;
    TestBoundsComponent::CreateComponent(&world, pComponent);

    pObject->AttachComponent(pComponent);
  }

  world.Update();

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "FindObjectsInSphere")
  {
    ezBoundingSphere testSphere(ezVec3(100.0f, 60.0f, 400.0f), 3000.0f);

    ezDynamicArray<ezGameObject*> objectsInSphere;
    ezHashSet<ezGameObject*> uniqueObjects;
    world.GetSpatialSystem().FindObjectsInSphere(testSphere, objectsInSphere);

    for (auto pObject : objectsInSphere)
    {
      ezBoundingSphere objSphere = pObject->GetGlobalBounds().GetSphere();

      EZ_TEST_BOOL(testSphere.Overlaps(objSphere));
      EZ_TEST_BOOL(!uniqueObjects.Insert(pObject));
    }

    // Check for missing objects
    for (auto it = world.GetObjects(); it.IsValid(); ++it)
    {
      ezBoundingSphere objSphere = it->GetGlobalBounds().GetSphere();
      if (testSphere.Overlaps(objSphere))
      {
        EZ_TEST_BOOL(uniqueObjects.Contains(it));
      }
    }

    objectsInSphere.Clear();
    uniqueObjects.Clear();

    world.GetSpatialSystem().FindObjectsInSphere(testSphere, [&](ezGameObject* pObject)
    {
      objectsInSphere.PushBack(pObject);
      EZ_TEST_BOOL(!uniqueObjects.Insert(pObject));

      return ezVisitorExecution::Continue;
    });

    for (auto pObject : objectsInSphere)
    {
      ezBoundingSphere objSphere = pObject->GetGlobalBounds().GetSphere();

      EZ_TEST_BOOL(testSphere.Overlaps(objSphere));
    }

    // Check for missing objects
    for (auto it = world.GetObjects(); it.IsValid(); ++it)
    {
      ezBoundingSphere objSphere = it->GetGlobalBounds().GetSphere();
      if (testSphere.Overlaps(objSphere))
      {
        EZ_TEST_BOOL(uniqueObjects.Contains(it));
      }
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "FindObjectsInBox")
  {
    ezBoundingBox testBox;
    testBox.SetCenterAndHalfExtents(ezVec3(100.0f, 60.0f, 400.0f), ezVec3(3000.0f));

    ezDynamicArray<ezGameObject*> objectsInBox;
    ezHashSet<ezGameObject*> uniqueObjects;
    world.GetSpatialSystem().FindObjectsInBox(testBox, objectsInBox);

    for (auto pObject : objectsInBox)
    {
      ezBoundingBox objBox = pObject->GetGlobalBounds().GetBox();

      EZ_TEST_BOOL(testBox.Overlaps(objBox));
      EZ_TEST_BOOL(!uniqueObjects.Insert(pObject));
    }

    // Check for missing objects
    for (auto it = world.GetObjects(); it.IsValid(); ++it)
    {
      ezBoundingBox objBox = it->GetGlobalBounds().GetBox();
      if (testBox.Overlaps(objBox))
      {
        EZ_TEST_BOOL(uniqueObjects.Contains(it));
      }
    }

    objectsInBox.Clear();
    uniqueObjects.Clear();

    world.GetSpatialSystem().FindObjectsInBox(testBox, [&](ezGameObject* pObject)
    {
      objectsInBox.PushBack(pObject);
      EZ_TEST_BOOL(!uniqueObjects.Insert(pObject));

      return ezVisitorExecution::Continue;
    });

    for (auto pObject : objectsInBox)
    {
      ezBoundingSphere objSphere = pObject->GetGlobalBounds().GetSphere();

      EZ_TEST_BOOL(testBox.Overlaps(objSphere));
    }

    // Check for missing objects
    for (auto it = world.GetObjects(); it.IsValid(); ++it)
    {
      ezBoundingBox objBox = it->GetGlobalBounds().GetBox();
      if (testBox.Overlaps(objBox))
      {
        EZ_TEST_BOOL(uniqueObjects.Contains(it));
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
      ezProfilingSystem::Capture(fileWriter);
      ezLog::Info("Profiling capture saved to '{0}'.", fileWriter.GetFilePathAbsolute().GetData());
    }
  }
}
