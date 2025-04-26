#include <CoreTest/CoreTestPCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Threading/Thread.h>
#include <Foundation/Types/ScopeExit.h>

EZ_CREATE_SIMPLE_TEST_GROUP(ResourceManager);

namespace
{
  using TestResourceHandle = ezTypedResourceHandle<class TestResource>;

  struct TestResourceDescriptor
  {
    ezUInt32 m_uiData = 0;
  };

  class TestResource : public ezResource
  {
    EZ_ADD_DYNAMIC_REFLECTION(TestResource, ezResource);
    EZ_RESOURCE_DECLARE_COMMON_CODE(TestResource);
    EZ_RESOURCE_DECLARE_CREATEABLE(TestResource, TestResourceDescriptor);

  public:
    TestResource()
      : ezResource(ezResource::DoUpdate::OnAnyThread, 1)
    {
    }

  protected:
    virtual ezResourceLoadDesc UnloadData(Unload WhatToUnload) override
    {
      ezResourceLoadDesc ld;
      ld.m_State = ezResourceState::Unloaded;
      ld.m_uiQualityLevelsDiscardable = 0;
      ld.m_uiQualityLevelsLoadable = 0;

      return ld;
    }

    virtual ezResourceLoadDesc UpdateContent(ezStreamReader* Stream) override
    {
      ezResourceLoadDesc ld;
      ld.m_State = ezResourceState::Loaded;
      ld.m_uiQualityLevelsDiscardable = 0;
      ld.m_uiQualityLevelsLoadable = 0;

      ezStreamReader& s = *Stream;

      ezUInt32 uiNumElements = 0;
      s >> uiNumElements;

      if (GetResourceID().StartsWith("NonBlockingLevel1-"))
      {
        m_hNested = ezResourceManager::LoadResource<TestResource>("Level0-0");
      }

      if (GetResourceID().StartsWith("BlockingLevel1-"))
      {
        m_hNested = ezResourceManager::LoadResource<TestResource>("Level0-0");

        ezResourceLock<TestResource> pTestResource(m_hNested, ezResourceAcquireMode::BlockTillLoaded_NeverFail);

        EZ_ASSERT_ALWAYS(pTestResource.GetAcquireResult() == ezResourceAcquireResult::Final, "");
      }

      m_Data.SetCountUninitialized(uiNumElements);

      for (ezUInt32 i = 0; i < uiNumElements; ++i)
      {
        s >> m_Data[i];
      }

      return ld;
    }

    virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override
    {
      out_NewMemoryUsage.m_uiMemoryCPU = sizeof(TestResource);
      out_NewMemoryUsage.m_uiMemoryGPU = 0;
    }

  public:
    void Test() { EZ_TEST_BOOL(!m_Data.IsEmpty()); }

  private:
    TestResourceHandle m_hNested;
    ezDynamicArray<ezUInt32> m_Data;
  };

  EZ_RESOURCE_IMPLEMENT_CREATEABLE(TestResource, TestResourceDescriptor)
  {
    ezResourceLoadDesc res;
    res.m_State = ezResourceState::Loaded;
    res.m_uiQualityLevelsDiscardable = 0;
    res.m_uiQualityLevelsLoadable = 0;

    EZ_TEST_INT(m_Data.GetCount(), 0);
    m_Data.PushBack(descriptor.m_uiData);
    ezThreadUtils::Sleep(ezTime::MakeFromMilliseconds(50));
    EZ_TEST_INT(m_Data.GetCount(), 1);

    return res;
  }

  class TestResourceTypeLoader : public ezResourceTypeLoader
  {
  public:
    struct LoadedData
    {
      ezDefaultMemoryStreamStorage m_StreamData;
      ezMemoryStreamReader m_Reader;
    };

    virtual ezResourceLoadData OpenDataStream(const ezResource* pResource) override
    {
      LoadedData* pData = EZ_DEFAULT_NEW(LoadedData);

      const ezUInt32 uiNumElements = 1024 * 10;
      pData->m_StreamData.Reserve(uiNumElements * sizeof(ezUInt32) + 1);

      ezMemoryStreamWriter writer(&pData->m_StreamData);
      pData->m_Reader.SetStorage(&pData->m_StreamData);

      writer << uiNumElements;

      for (ezUInt32 i = 0; i < uiNumElements; ++i)
      {
        writer << i;
      }

      ezResourceLoadData ld;
      ld.m_pCustomLoaderData = pData;
      ld.m_pDataStream = &pData->m_Reader;
      ld.m_sResourceDescription = pResource->GetResourceID();

      return ld;
    }

    virtual void CloseDataStream(const ezResource* pResource, const ezResourceLoadData& loaderData) override
    {
      LoadedData* pData = static_cast<LoadedData*>(loaderData.m_pCustomLoaderData);
      EZ_DEFAULT_DELETE(pData);
    }
  };

  EZ_RESOURCE_IMPLEMENT_COMMON_CODE(TestResource);
  EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(TestResource, 1, ezRTTIDefaultAllocator<TestResource>)
  EZ_END_DYNAMIC_REFLECTED_TYPE;


  class ResourceTestThread : public ezThread
  {
  public:
    ezDelegate<int()> m_Lambda;

    ResourceTestThread()
      : ezThread("Resource Test Thread")
    {
    }

    virtual ezUInt32 Run()
    {
      if (m_Lambda.IsValid())
      {
        return m_Lambda();
      }
      return 0;
    }
  };
} // namespace

EZ_CREATE_SIMPLE_TEST(ResourceManager, Basics)
{
  TestResourceTypeLoader TypeLoader;
  ezResourceManager::AllowResourceTypeAcquireDuringUpdateContent<TestResource, TestResource>();
  ezResourceManager::SetResourceTypeLoader<TestResource>(&TypeLoader);
  EZ_SCOPE_EXIT(ezResourceManager::SetResourceTypeLoader<TestResource>(nullptr));

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Main")
  {
    EZ_TEST_INT(ezResourceManager::GetAllResourcesOfType<TestResource>()->GetCount(), 0);

    const ezUInt32 uiNumResources = 200;

    ezDynamicArray<TestResourceHandle> hResources;
    hResources.Reserve(uiNumResources);

    ezStringBuilder sResourceID;
    for (ezUInt32 i = 0; i < uiNumResources; ++i)
    {
      sResourceID.SetFormat("Level0-{}", i);
      hResources.PushBack(ezResourceManager::LoadResource<TestResource>(sResourceID));
    }

    EZ_TEST_INT(ezResourceManager::GetAllResourcesOfType<TestResource>()->GetCount(), uiNumResources);

    for (ezUInt32 i = 0; i < uiNumResources; ++i)
    {
      ezResourceManager::PreloadResource(hResources[i]);
    }

    EZ_TEST_INT(ezResourceManager::GetAllResourcesOfType<TestResource>()->GetCount(), uiNumResources);

    for (ezUInt32 i = 0; i < uiNumResources; ++i)
    {
      ezResourceLock<TestResource> pTestResource(hResources[i], ezResourceAcquireMode::BlockTillLoaded_NeverFail);

      EZ_TEST_BOOL(pTestResource.GetAcquireResult() == ezResourceAcquireResult::Final);

      pTestResource->Test();
    }

    EZ_TEST_INT(ezResourceManager::GetAllResourcesOfType<TestResource>()->GetCount(), uiNumResources);

    hResources.Clear();

    ezUInt32 uiUnloaded = 0;

    for (ezUInt32 tries = 0; tries < 3; ++tries)
    {
      // if a resource is in a loading queue, unloading it can actually 'fail' for a short time
      uiUnloaded += ezResourceManager::FreeAllUnusedResources();

      if (uiUnloaded == uiNumResources)
        break;

      ezThreadUtils::Sleep(ezTime::MakeFromMilliseconds(100));
    }

    EZ_TEST_INT(ezResourceManager::GetAllResourcesOfType<TestResource>()->GetCount(), 0);
  }
}


EZ_CREATE_SIMPLE_TEST(ResourceManager, ThreadedCreateResource)
{
  TestResourceTypeLoader TypeLoader;
  ezResourceManager::AllowResourceTypeAcquireDuringUpdateContent<TestResource, TestResource>();
  ezResourceManager::SetResourceTypeLoader<TestResource>(&TypeLoader);
  EZ_SCOPE_EXIT(ezResourceManager::SetResourceTypeLoader<TestResource>(nullptr));

  auto testLambda = []() -> int
  {
    TestResourceDescriptor desc;
    TestResourceHandle hTest = ezResourceManager::GetOrCreateResource<TestResource>("test1", std::move(desc));
    {
      ezResourceLock<TestResource> pTest(hTest, ezResourceAcquireMode::PointerOnly);
      EZ_TEST_INT((int)pTest.GetAcquireResult(), (int)ezResourceAcquireResult::Final);
    }
    EZ_TEST_INT(ezResourceManager::GetAllResourcesOfType<TestResource>()->GetCount(), 1);
    return 0;
  };

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ThreadedCreateResource")
  {
    ezHybridArray<ezUniquePtr<ResourceTestThread>, 8> threads;
    for (ezUInt32 i = 0; i < 8; ++i)
    {
      ezUniquePtr<ResourceTestThread> thread = EZ_DEFAULT_NEW(ResourceTestThread);
      thread->m_Lambda = testLambda;
      threads.PushBack(std::move(thread));
    }

    EZ_TEST_INT(ezResourceManager::GetAllResourcesOfType<TestResource>()->GetCount(), 0);

    for (ezUInt32 i = 0; i < 8; ++i)
    {
      threads[i]->Start();
    }

    for (ezUInt32 i = 0; i < 8; ++i)
    {
      threads[i]->Join();
    }
    EZ_TEST_INT(ezResourceManager::GetAllResourcesOfType<TestResource>()->GetCount(), 1);

    threads.Clear();
  }
}


EZ_CREATE_SIMPLE_TEST(ResourceManager, NestedLoading)
{
  TestResourceTypeLoader TypeLoader;
  ezResourceManager::AllowResourceTypeAcquireDuringUpdateContent<TestResource, TestResource>();
  ezResourceManager::SetResourceTypeLoader<TestResource>(&TypeLoader);
  EZ_SCOPE_EXIT(ezResourceManager::SetResourceTypeLoader<TestResource>(nullptr));

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "NonBlocking")
  {
    EZ_TEST_INT(ezResourceManager::GetAllResourcesOfType<TestResource>()->GetCount(), 0);

    const ezUInt32 uiNumResources = 200;

    ezDynamicArray<TestResourceHandle> hResources;
    hResources.Reserve(uiNumResources);

    ezStringBuilder sResourceID;
    for (ezUInt32 i = 0; i < uiNumResources; ++i)
    {
      sResourceID.SetFormat("NonBlockingLevel1-{}", i);
      hResources.PushBack(ezResourceManager::LoadResource<TestResource>(sResourceID));
    }

    EZ_TEST_INT(ezResourceManager::GetAllResourcesOfType<TestResource>()->GetCount(), uiNumResources);

    for (ezUInt32 i = 0; i < uiNumResources; ++i)
    {
      ezResourceManager::PreloadResource(hResources[i]);
    }

    for (ezUInt32 i = 0; i < uiNumResources; ++i)
    {
      ezResourceLock<TestResource> pTestResource(hResources[i], ezResourceAcquireMode::BlockTillLoaded_NeverFail);

      EZ_TEST_BOOL(pTestResource.GetAcquireResult() == ezResourceAcquireResult::Final);

      pTestResource->Test();
    }

    EZ_TEST_INT(ezResourceManager::GetAllResourcesOfType<TestResource>()->GetCount(), uiNumResources + 1);

    hResources.Clear();

    while (ezResourceManager::IsAnyLoadingInProgress())
    {
      ezThreadUtils::Sleep(ezTime::MakeFromMilliseconds(100));
    }

    ezResourceManager::FreeAllUnusedResources();
    EZ_TEST_INT(ezResourceManager::GetAllResourcesOfType<TestResource>()->GetCount(), 0);
  }

  // Test disabled as it deadlocks
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Blocking")
  {
    EZ_TEST_INT(ezResourceManager::GetAllResourcesOfType<TestResource>()->GetCount(), 0);

    const ezUInt32 uiNumResources = 500;

    ezDynamicArray<TestResourceHandle> hResources;
    hResources.Reserve(uiNumResources);

    ezStringBuilder sResourceID;
    for (ezUInt32 i = 0; i < uiNumResources; ++i)
    {
      sResourceID.SetFormat("BlockingLevel1-{}", i);
      hResources.PushBack(ezResourceManager::LoadResource<TestResource>(sResourceID));
    }

    EZ_TEST_INT(ezResourceManager::GetAllResourcesOfType<TestResource>()->GetCount(), uiNumResources);

    for (ezUInt32 i = 0; i < uiNumResources; ++i)
    {
      ezResourceManager::PreloadResource(hResources[i]);
    }

    for (ezUInt32 i = 0; i < uiNumResources; ++i)
    {
      ezResourceLock<TestResource> pTestResource(hResources[i], ezResourceAcquireMode::BlockTillLoaded_NeverFail);

      EZ_TEST_BOOL(pTestResource.GetAcquireResult() == ezResourceAcquireResult::Final);

      pTestResource->Test();
    }

    EZ_TEST_INT(ezResourceManager::GetAllResourcesOfType<TestResource>()->GetCount(), uiNumResources + 1);

    hResources.Clear();

    while (ezResourceManager::IsAnyLoadingInProgress())
    {
      ezThreadUtils::Sleep(ezTime::MakeFromMilliseconds(100));
    }

    ezResourceManager::FreeAllUnusedResources();
    ezThreadUtils::Sleep(ezTime::MakeFromMilliseconds(100));
    ezResourceManager::FreeAllUnusedResources();

    EZ_TEST_INT(ezResourceManager::GetAllResourcesOfType<TestResource>()->GetCount(), 0);
  }
}
