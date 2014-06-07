#include <PCH.h>
#include <Foundation/Communication/MessageQueue.h>

namespace
{
  struct TestMessage : public ezMessage
  {
    EZ_DECLARE_MESSAGE_TYPE(TestMessage);

    int x;
    int y;
  };

  struct MetaData
  {
    int receiver;
  };

  typedef ezMessageQueue<MetaData> TestMessageQueue;

  EZ_IMPLEMENT_MESSAGE_TYPE(TestMessage);
}

EZ_CREATE_SIMPLE_TEST(Communication, MessageQueue)
{
  {
    TestMessage msg;
    EZ_TEST_INT(msg.GetSize(), sizeof(TestMessage));
  }

  TestMessageQueue q;
  
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Enqueue")
  {
    for (ezUInt32 i = 0; i < 100; ++i)
    {
      TestMessage* pMsg = EZ_DEFAULT_NEW(TestMessage);
      pMsg->x = rand();
      pMsg->y = rand();

      MetaData md;
      md.receiver = rand() % 10;

      q.Enqueue(pMsg, md);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Sorting")
  {
    struct MessageComparer
    {
      bool Less(const TestMessageQueue::Entry& a, const TestMessageQueue::Entry& b) const
      {
        if (a.m_MetaData.receiver != b.m_MetaData.receiver)
          return a.m_MetaData.receiver < b.m_MetaData.receiver;

        return a.m_pMessage->GetHash() < b.m_pMessage->GetHash();
      }
    };

    q.Sort(MessageComparer());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "operator[]")
  {
    EZ_LOCK(q);

    ezMessage* pLastMsg = q[0].m_pMessage;
    MetaData lastMd = q[0].m_MetaData;

    for (ezUInt32 i = 1; i < q.GetCount(); ++i)
    {
      ezMessage* pMsg = q[i].m_pMessage;
      MetaData md = q[i].m_MetaData;

      if (md.receiver == lastMd.receiver)
      {
        EZ_TEST_BOOL(pMsg->GetHash() >= pLastMsg->GetHash());
      }
      else
      {
        EZ_TEST_BOOL(md.receiver >= lastMd.receiver);
      }
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Dequeue")
  {
    ezMessage* pMsg = nullptr;
    MetaData md;

    while (q.TryDequeue(pMsg, md))
    {
      EZ_DEFAULT_DELETE(pMsg);
    }
  }
}
