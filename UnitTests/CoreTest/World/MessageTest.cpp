#include <PCH.h>
#include <Foundation/Time/Clock.h>
#include <Core/World/World.h>

namespace
{
  class TestMessage : public ezMessage
  {
    EZ_DECLARE_MESSAGE_TYPE(TestMessage);

    int m_iValue;
  };

  class TestMessage2 : public ezMessage
  {
    EZ_DECLARE_MESSAGE_TYPE(TestMessage2);

    int m_iValue;
  };

  EZ_IMPLEMENT_MESSAGE_TYPE(TestMessage);
  EZ_IMPLEMENT_MESSAGE_TYPE(TestMessage2);

  class TestComponentMsg;
  typedef ezComponentManager<TestComponentMsg> TestComponentMsgManager;

  class TestComponentMsg : public ezComponent
  {
    EZ_DECLARE_COMPONENT_TYPE(TestComponentMsg, TestComponentMsgManager);

  public:
    TestComponentMsg() : m_iSomeData(1), m_iSomeData2(2) {}
    ~TestComponentMsg() {}

    void OnTestMessage(TestMessage& msg)
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

  EZ_BEGIN_COMPONENT_TYPE(TestComponentMsg, ezComponent, 1, TestComponentMsgManager);
    EZ_BEGIN_MESSAGEHANDLERS
      EZ_MESSAGE_HANDLER(TestMessage, OnTestMessage),
      EZ_MESSAGE_HANDLER(TestMessage2, OnTestMessage2)
    EZ_END_MESSAGEHANDLERS
  EZ_END_COMPONENT_TYPE();

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
  ezClock::SetNumGlobalClocks();
  ezWorld world("Test");
  TestComponentMsgManager* pManager = world.CreateComponentManager<TestComponentMsgManager>();

  ezGameObjectDesc desc;
  desc.m_sName.Assign("Root");
  ezGameObject* pRoot = nullptr;
  world.CreateObject(desc, pRoot);
  pRoot->AddComponent(pManager->CreateComponent());

  ezGameObject* pParents[2];
  desc.m_Parent = pRoot->GetHandle();
  desc.m_sName.Assign("Parent1");
  world.CreateObject(desc, pParents[0]);
  pParents[0]->AddComponent(pManager->CreateComponent());

  desc.m_sName.Assign("Parent2");
  world.CreateObject(desc, pParents[1]);
  pParents[1]->AddComponent(pManager->CreateComponent());

  for (ezUInt32 i = 0; i < 2; ++i)
  {
    desc.m_Parent = pParents[i]->GetHandle();
    for (ezUInt32 j = 0; j < 4; ++j)
    {
      ezStringBuilder sb;
      sb.AppendFormat("Parent%d_Child%d", i+1, j+1);
      desc.m_sName.Assign(sb.GetData());

      ezGameObject* pObject = nullptr;
      world.CreateObject(desc, pObject);
      pObject->AddComponent(pManager->CreateComponent());
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Direct Routing")
  {
    ResetComponents(*pRoot);

    TestMessage msg;
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

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Routing to parent")
  {
    ResetComponents(*pRoot);

    TestMessage msg;
    msg.m_iValue = 4;
    pParents[0]->SendMessage(msg, ezObjectMsgRouting::ToParent);

    TestMessage2 msg2;
    msg2.m_iValue = 4;
    pParents[0]->SendMessage(msg2, ezObjectMsgRouting::ToParent);

    TestComponentMsg* pComponent = nullptr;
    pParents[0]->TryGetComponentOfBaseType(pComponent);
    EZ_TEST_INT(pComponent->m_iSomeData, 5);
    EZ_TEST_INT(pComponent->m_iSomeData2, 10);

    pRoot->TryGetComponentOfBaseType(pComponent);
    EZ_TEST_INT(pComponent->m_iSomeData, 5);
    EZ_TEST_INT(pComponent->m_iSomeData2, 10);

    // siblings and children should not be affected
    pParents[1]->TryGetComponentOfBaseType(pComponent);
    EZ_TEST_INT(pComponent->m_iSomeData, 1);
    EZ_TEST_INT(pComponent->m_iSomeData2, 2);

    for (auto it = pParents[0]->GetChildren(); it.IsValid(); ++it)
    {
      it->TryGetComponentOfBaseType(pComponent);
      EZ_TEST_INT(pComponent->m_iSomeData, 1);
      EZ_TEST_INT(pComponent->m_iSomeData2, 2);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Routing to children")
  {
    ResetComponents(*pRoot);

    TestMessage msg;
    msg.m_iValue = 4;
    pParents[0]->SendMessage(msg, ezObjectMsgRouting::ToChildren);

    TestMessage2 msg2;
    msg2.m_iValue = 4;
    pParents[0]->SendMessage(msg2, ezObjectMsgRouting::ToChildren);

    TestComponentMsg* pComponent = nullptr;
    pParents[0]->TryGetComponentOfBaseType(pComponent);
    EZ_TEST_INT(pComponent->m_iSomeData, 5);
    EZ_TEST_INT(pComponent->m_iSomeData2, 10);

    for (auto it = pParents[0]->GetChildren(); it.IsValid(); ++it)
    {
      it->TryGetComponentOfBaseType(pComponent);
      EZ_TEST_INT(pComponent->m_iSomeData, 5);
      EZ_TEST_INT(pComponent->m_iSomeData2, 10);
    }

    // siblings and parent should not be affected
    pParents[1]->TryGetComponentOfBaseType(pComponent);
    EZ_TEST_INT(pComponent->m_iSomeData, 1);
    EZ_TEST_INT(pComponent->m_iSomeData2, 2);

    pRoot->TryGetComponentOfBaseType(pComponent);
    EZ_TEST_INT(pComponent->m_iSomeData, 1);
    EZ_TEST_INT(pComponent->m_iSomeData2, 2);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Routing to sub-tree")
  {
    ResetComponents(*pRoot);

    TestMessage msg;
    msg.m_iValue = 4;
    pParents[0]->SendMessage(msg, ezObjectMsgRouting::ToSubTree);

    TestMessage2 msg2;
    msg2.m_iValue = 4;
    pParents[0]->SendMessage(msg2, ezObjectMsgRouting::ToSubTree);

    TestComponentMsg* pComponent = nullptr;
    pParents[0]->TryGetComponentOfBaseType(pComponent);
    EZ_TEST_INT(pComponent->m_iSomeData, 5);
    EZ_TEST_INT(pComponent->m_iSomeData2, 10);

    pParents[1]->TryGetComponentOfBaseType(pComponent);
    EZ_TEST_INT(pComponent->m_iSomeData, 5);
    EZ_TEST_INT(pComponent->m_iSomeData2, 10);

    pRoot->TryGetComponentOfBaseType(pComponent);
    EZ_TEST_INT(pComponent->m_iSomeData, 5);
    EZ_TEST_INT(pComponent->m_iSomeData2, 10);

    for (auto it = pParents[0]->GetChildren(); it.IsValid(); ++it)
    {
      it->TryGetComponentOfBaseType(pComponent);
      EZ_TEST_INT(pComponent->m_iSomeData, 5);
      EZ_TEST_INT(pComponent->m_iSomeData2, 10);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Queuing")
  {
    ResetComponents(*pRoot);

    for (ezUInt32 i = 0; i < 10; ++i)
    {
      TestMessage msg;
      msg.m_iValue = i;
      pRoot->PostMessage(msg, ezObjectMsgQueueType::NextFrame);

      TestMessage2 msg2;
      msg2.m_iValue = i;
      pRoot->PostMessage(msg2, ezObjectMsgQueueType::NextFrame);
    }

    ezClock::UpdateAllGlobalClocks();
    world.Update();

    TestComponentMsg* pComponent = nullptr;
    pRoot->TryGetComponentOfBaseType(pComponent);
    EZ_TEST_INT(pComponent->m_iSomeData, 46);
    EZ_TEST_INT(pComponent->m_iSomeData2, 92);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Queuing with delay")
  {
    ResetComponents(*pRoot);

    for (ezUInt32 i = 0; i < 10; ++i)
    {
      TestMessage msg;
      msg.m_iValue = i;
      pRoot->PostMessage(msg, ezObjectMsgQueueType::NextFrame, ezTime::Seconds(i+1));

      TestMessage2 msg2;
      msg2.m_iValue = i;
      pRoot->PostMessage(msg2, ezObjectMsgQueueType::NextFrame, ezTime::Seconds(i+1));
    }

    ezClock::Get(ezGlobalClock_GameLogic)->SetFixedTimeStep(ezTime::Seconds(1.0f));

    int iDesiredValue = 1;
    int iDesiredValue2 = 2;

    for (ezUInt32 i = 0; i < 10; ++i)
    {
      iDesiredValue += i;
      iDesiredValue2 += i * 2;

      ezClock::UpdateAllGlobalClocks();
      world.Update();

      TestComponentMsg* pComponent = nullptr;
      pRoot->TryGetComponentOfBaseType(pComponent);
      EZ_TEST_INT(pComponent->m_iSomeData, iDesiredValue);
      EZ_TEST_INT(pComponent->m_iSomeData2, iDesiredValue2);
    }
  }
}
