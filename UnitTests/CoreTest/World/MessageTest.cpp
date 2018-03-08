#include <PCH.h>
#include <Foundation/Time/Clock.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Core/World/World.h>

namespace
{
  struct ezMsgTest : public ezMessage
  {
    EZ_DECLARE_MESSAGE_TYPE(ezMsgTest, ezMessage);

  };

  EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgTest);
  EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgTest, 1, ezRTTIDefaultAllocator<ezMsgTest>)
  EZ_END_DYNAMIC_REFLECTED_TYPE

  struct TestMessage1 : public ezMsgTest
  {
    EZ_DECLARE_MESSAGE_TYPE(TestMessage1, ezMsgTest);

    int m_iValue;
  };

  struct TestMessage2 : public ezMsgTest
  {
    EZ_DECLARE_MESSAGE_TYPE(TestMessage2, ezMsgTest);

    virtual ezInt32 GetSortingKey() const override
    {
      return 2;
    }

    int m_iValue;
  };

  EZ_IMPLEMENT_MESSAGE_TYPE(TestMessage1);
  EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(TestMessage1, 1, ezRTTIDefaultAllocator<TestMessage1>)
  EZ_END_DYNAMIC_REFLECTED_TYPE

  EZ_IMPLEMENT_MESSAGE_TYPE(TestMessage2);
  EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(TestMessage2, 1, ezRTTIDefaultAllocator<TestMessage2>)
  EZ_END_DYNAMIC_REFLECTED_TYPE

  class TestComponentMsg;
  typedef ezComponentManager<TestComponentMsg, ezBlockStorageType::FreeList> TestComponentMsgManager;

  class TestComponentMsg : public ezComponent
  {
    EZ_DECLARE_COMPONENT_TYPE(TestComponentMsg, ezComponent, TestComponentMsgManager);

  public:
    TestComponentMsg() : m_iSomeData(1), m_iSomeData2(2) {}
    ~TestComponentMsg() {}

    virtual void SerializeComponent(ezWorldWriter& stream) const override {}
    virtual void DeserializeComponent(ezWorldReader& stream) override {}

    void OnTestMessage(TestMessage1& msg)
    {
      m_iSomeData += msg.m_iValue;
    }

    void OnTestMessage2(TestMessage2& msg)
    {
      m_iSomeData2 += 2 * msg.m_iValue;
    }

    ezInt32 m_iSomeData;
    ezInt32 m_iSomeData2;
  };

  EZ_BEGIN_COMPONENT_TYPE(TestComponentMsg, 1, ezComponentMode::Static)
  {
    EZ_BEGIN_MESSAGEHANDLERS
    {
      EZ_MESSAGE_HANDLER(TestMessage1, OnTestMessage),
      EZ_MESSAGE_HANDLER(TestMessage2, OnTestMessage2),
    }
    EZ_END_MESSAGEHANDLERS
  }
  EZ_END_COMPONENT_TYPE

  void ResetComponents(ezGameObject& object)
  {
    TestComponentMsg* pComponent = nullptr;
    if (object.TryGetComponentOfBaseType(pComponent))
    {
      pComponent->m_iSomeData = 1;
      pComponent->m_iSomeData2 = 2;
    }

    for (auto it = object.GetChildren(); it.IsValid(); ++it)
    {
      ResetComponents(*it);
    }
  }
}

EZ_CREATE_SIMPLE_TEST(World, Messaging)
{
  ezWorldDesc worldDesc("Test");
  ezWorld world(worldDesc);
  EZ_LOCK(world.GetWriteMarker());

  TestComponentMsgManager* pManager = world.GetOrCreateComponentManager<TestComponentMsgManager>();

  ezGameObjectDesc desc;
  desc.m_sName.Assign("Root");
  ezGameObject* pRoot = nullptr;
  world.CreateObject(desc, pRoot);
  TestComponentMsg* pComponent = nullptr;
  pManager->CreateComponent(pRoot, pComponent);

  ezGameObject* pParents[2];
  desc.m_hParent = pRoot->GetHandle();
  desc.m_sName.Assign("Parent1");
  world.CreateObject(desc, pParents[0]);
  pManager->CreateComponent(pParents[0], pComponent);

  desc.m_sName.Assign("Parent2");
  world.CreateObject(desc, pParents[1]);
  pManager->CreateComponent(pParents[1], pComponent);

  for (ezUInt32 i = 0; i < 2; ++i)
  {
    desc.m_hParent = pParents[i]->GetHandle();
    for (ezUInt32 j = 0; j < 4; ++j)
    {
      ezStringBuilder sb;
      sb.AppendFormat("Parent{0}_Child{1}", i+1, j+1);
      desc.m_sName.Assign(sb.GetData());

      ezGameObject* pObject = nullptr;
      world.CreateObject(desc, pObject);
      pManager->CreateComponent(pObject, pComponent);
    }
  }

  // one update step so components are initialized
  world.Update();

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Direct Routing")
  {
    ResetComponents(*pRoot);

    TestMessage1 msg;
    msg.m_iValue = 4;
    pParents[0]->SendMessage(msg);

    TestMessage2 msg2;
    msg2.m_iValue = 4;
    pParents[0]->SendMessage(msg2);

    TestComponentMsg* pComponent = nullptr;
    pParents[0]->TryGetComponentOfBaseType(pComponent);
    EZ_TEST_INT(pComponent->m_iSomeData, 5);
    EZ_TEST_INT(pComponent->m_iSomeData2, 10);

    // siblings, parent and children should not be affected
    pParents[1]->TryGetComponentOfBaseType(pComponent);
    EZ_TEST_INT(pComponent->m_iSomeData, 1);
    EZ_TEST_INT(pComponent->m_iSomeData2, 2);

    pRoot->TryGetComponentOfBaseType(pComponent);
    EZ_TEST_INT(pComponent->m_iSomeData, 1);
    EZ_TEST_INT(pComponent->m_iSomeData2, 2);

    for (auto it = pParents[0]->GetChildren(); it.IsValid(); ++it)
    {
      it->TryGetComponentOfBaseType(pComponent);
      EZ_TEST_INT(pComponent->m_iSomeData, 1);
      EZ_TEST_INT(pComponent->m_iSomeData2, 2);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Queuing")
  {
    ResetComponents(*pRoot);

    for (ezUInt32 i = 0; i < 10; ++i)
    {
      TestMessage1 msg;
      msg.m_iValue = i;
      pRoot->PostMessage(msg, ezObjectMsgQueueType::NextFrame);

      TestMessage2 msg2;
      msg2.m_iValue = i;
      pRoot->PostMessage(msg2, ezObjectMsgQueueType::NextFrame);
    }

    world.Update();

    TestComponentMsg* pComponent = nullptr;
    pRoot->TryGetComponentOfBaseType(pComponent);
    EZ_TEST_INT(pComponent->m_iSomeData, 46);
    EZ_TEST_INT(pComponent->m_iSomeData2, 92);

    ezFrameAllocator::Reset();
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Queuing with delay")
  {
    ResetComponents(*pRoot);

    for (ezUInt32 i = 0; i < 10; ++i)
    {
      TestMessage1 msg;
      msg.m_iValue = i;
      pRoot->PostMessage(msg, ezObjectMsgQueueType::NextFrame, ezTime::Seconds(i+1));

      TestMessage2 msg2;
      msg2.m_iValue = i;
      pRoot->PostMessage(msg2, ezObjectMsgQueueType::NextFrame, ezTime::Seconds(i+1));
    }

    world.GetClock().SetFixedTimeStep(ezTime::Seconds(1.001f));

    int iDesiredValue = 1;
    int iDesiredValue2 = 2;

    for (ezUInt32 i = 0; i < 10; ++i)
    {
      iDesiredValue += i;
      iDesiredValue2 += i * 2;

      world.Update();

      TestComponentMsg* pComponent = nullptr;
      pRoot->TryGetComponentOfBaseType(pComponent);
      EZ_TEST_INT(pComponent->m_iSomeData, iDesiredValue);
      EZ_TEST_INT(pComponent->m_iSomeData2, iDesiredValue2);
    }

    ezFrameAllocator::Reset();
  }
}
