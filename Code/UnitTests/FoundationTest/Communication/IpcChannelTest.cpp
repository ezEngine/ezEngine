#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Communication/IpcChannel.h>
#include <Foundation/Time/Stopwatch.h>
#include <optional>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP) || EZ_ENABLED(EZ_PLATFORM_LINUX)

class ChannelTester
{
public:
  ChannelTester(ezIpcChannel* pChannel, bool bPing)
  {
    m_bPing = bPing;
    m_pChannel = pChannel;
    m_pChannel->SetReceiveCallback(ezMakeDelegate(&ChannelTester::ReceiveMessageData, this));
    m_pChannel->m_Events.AddEventHandler(ezMakeDelegate(&ChannelTester::OnIpcEventReceived, this));
  }
  ~ChannelTester()
  {
    m_pChannel->m_Events.RemoveEventHandler(ezMakeDelegate(&ChannelTester::OnIpcEventReceived, this));
    m_pChannel->SetReceiveCallback({});
  }

  void OnIpcEventReceived(const ezIpcChannelEvent& e)
  {
    EZ_LOCK(m_Mutex);
    m_ReceivedEvents.ExpandAndGetRef() = e;
  }

  std::optional<ezIpcChannelEvent> WaitForEvents(ezTime timeout)
  {
    ezStopwatch sw;

    while (sw.GetRunningTotal() < timeout)
    {
      ezThreadUtils::Sleep(ezTime::MakeFromMilliseconds(10));
      EZ_LOCK(m_Mutex);
      if (!m_ReceivedEvents.IsEmpty())
      {
        ezIpcChannelEvent e = m_ReceivedEvents.PeekFront();
        m_ReceivedEvents.PopFront();
        return e;
      }
    }

    return {};
  }

  void ReceiveMessageData(ezArrayPtr<const ezUInt8> data)
  {
    EZ_LOCK(m_Mutex);
    if (m_bPing)
    {
      m_pChannel->Send(data);
    }
    else
    {
      m_ReceivedMessages.ExpandAndGetRef() = data;
    }
  }

  std::optional<ezDynamicArray<ezUInt8>> WaitForMessage(ezTime timeout)
  {
    ezResult res = m_pChannel->WaitForMessages(timeout);
    if (res.Succeeded())
    {
      EZ_LOCK(m_Mutex);
      if (m_ReceivedMessages.GetCount() > 0)
      {
        auto res2 = m_ReceivedMessages.PeekFront();
        m_ReceivedMessages.PopFront();
        return res2;
      }
    }
    return {};
  }

private:
  bool m_bPing = false;
  ezMutex m_Mutex;
  ezIpcChannel* m_pChannel = nullptr;
  ezDeque<ezDynamicArray<ezUInt8>> m_ReceivedMessages;
  ezDeque<ezIpcChannelEvent> m_ReceivedEvents;
};

void TestIPCChannel(ezIpcChannel* pServer, ChannelTester* pServerTester, ezIpcChannel* pClient, ChannelTester* pClientTester)
{
  auto MessageMatches = [](const ezStringView& sReference, const ezDataBuffer& msg) -> bool
  {
    ezStringView sTemp(reinterpret_cast<const char*>(msg.GetData()), msg.GetCount());
    return sTemp == sReference;
  };

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Connect")
  {
    EZ_TEST_BOOL(pServer->GetConnectionState() == ezIpcChannel::ConnectionState::Disconnected);
    EZ_TEST_BOOL(pClient->GetConnectionState() == ezIpcChannel::ConnectionState::Disconnected);
    {
      auto res = pServerTester->WaitForEvents(ezTime::MakeFromMilliseconds(100));
      EZ_TEST_BOOL(!res.has_value());
      auto res2 = pClientTester->WaitForEvents(ezTime::MakeFromMilliseconds(100));
      EZ_TEST_BOOL(!res2.has_value());
    }
    {
      pServer->Connect();
      auto res = pServerTester->WaitForEvents(ezTime::MakeFromMilliseconds(100));
      EZ_TEST_BOOL(res.has_value() && res->m_Type == ezIpcChannelEvent::Connecting);
      EZ_TEST_BOOL(pServer->GetConnectionState() == ezIpcChannel::ConnectionState::Connecting);
    }
    {
      pClient->Connect();
      auto res = pClientTester->WaitForEvents(ezTime::MakeFromMilliseconds(100));
      EZ_TEST_BOOL(res.has_value() && res->m_Type == ezIpcChannelEvent::Connecting);
    }
    auto res = pServerTester->WaitForEvents(ezTime::MakeFromSeconds(100));
    EZ_TEST_BOOL(res.has_value() && res->m_Type == ezIpcChannelEvent::Connected);
    auto res2 = pClientTester->WaitForEvents(ezTime::MakeFromSeconds(100));
    EZ_TEST_BOOL(res2.has_value() && res2->m_Type == ezIpcChannelEvent::Connected);

    EZ_TEST_BOOL(pServer->GetConnectionState() == ezIpcChannel::ConnectionState::Connected);
    EZ_TEST_BOOL(pClient->GetConnectionState() == ezIpcChannel::ConnectionState::Connected);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Connect When Already Connected")
  {
    pServer->Connect();
    pClient->Connect();
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ClientSend")
  {
    ezStringView sMsg = "TestMessage"_ezsv;

    EZ_TEST_BOOL(pClient->Send(ezConstByteArrayPtr(reinterpret_cast<const ezUInt8*>(sMsg.GetStartPointer()), sMsg.GetElementCount())));

    auto res = pServerTester->WaitForEvents(ezTime::MakeFromSeconds(1));
    EZ_TEST_BOOL(res.has_value() && res->m_Type == ezIpcChannelEvent::NewMessages);
    auto res2 = pClientTester->WaitForEvents(ezTime::MakeFromSeconds(1));
    EZ_TEST_BOOL(res2.has_value() && res2->m_Type == ezIpcChannelEvent::NewMessages);

    auto res3 = pClientTester->WaitForMessage(ezTime::MakeFromSeconds(1));
    EZ_TEST_BOOL(res3.has_value() && MessageMatches(sMsg, res3.value()));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ServerSend")
  {
    ezStringView sMsg = "TestMessage2"_ezsv;

    EZ_TEST_BOOL(pServer->Send(ezConstByteArrayPtr(reinterpret_cast<const ezUInt8*>(sMsg.GetStartPointer()), sMsg.GetElementCount())));

    auto res2 = pClientTester->WaitForEvents(ezTime::MakeFromSeconds(1));
    EZ_TEST_BOOL(res2.has_value() && res2->m_Type == ezIpcChannelEvent::NewMessages);

    auto res3 = pClientTester->WaitForMessage(ezTime::MakeFromSeconds(1));
    EZ_TEST_BOOL(res3.has_value() && MessageMatches(sMsg, res3.value()));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ClientDisconnect")
  {
    pClient->Disconnect();
    pClient->Disconnect();

    auto res = pServerTester->WaitForEvents(ezTime::MakeFromSeconds(1));
    EZ_TEST_BOOL(res.has_value() && res->m_Type == ezIpcChannelEvent::Disconnected);
    auto res2 = pClientTester->WaitForEvents(ezTime::MakeFromSeconds(1));
    EZ_TEST_BOOL(res2.has_value() && res2->m_Type == ezIpcChannelEvent::Disconnected);

    EZ_TEST_BOOL(pServer->GetConnectionState() == ezIpcChannel::ConnectionState::Disconnected);
    EZ_TEST_BOOL(pClient->GetConnectionState() == ezIpcChannel::ConnectionState::Disconnected);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Reconnect")
  {
    {
      pServer->Connect();
      auto res = pServerTester->WaitForEvents(ezTime::MakeFromMilliseconds(100));
      EZ_TEST_BOOL(res.has_value() && res->m_Type == ezIpcChannelEvent::Connecting);
      EZ_TEST_BOOL(pServer->GetConnectionState() == ezIpcChannel::ConnectionState::Connecting);
    }
    {
      pClient->Connect();
      auto res = pClientTester->WaitForEvents(ezTime::MakeFromMilliseconds(100));
      EZ_TEST_BOOL(res.has_value() && res->m_Type == ezIpcChannelEvent::Connecting);
    }

    auto res = pServerTester->WaitForEvents(ezTime::MakeFromSeconds(1));
    EZ_TEST_BOOL(res.has_value() && res->m_Type == ezIpcChannelEvent::Connected);
    auto res2 = pClientTester->WaitForEvents(ezTime::MakeFromSeconds(1));
    EZ_TEST_BOOL(res2.has_value() && res2->m_Type == ezIpcChannelEvent::Connected);

    EZ_TEST_BOOL(pServer->GetConnectionState() == ezIpcChannel::ConnectionState::Connected);
    EZ_TEST_BOOL(pClient->GetConnectionState() == ezIpcChannel::ConnectionState::Connected);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ClientSend after reconnect")
  {
    ezStringView sMsg = "TestMessage"_ezsv;

    EZ_TEST_BOOL(pClient->Send(ezConstByteArrayPtr(reinterpret_cast<const ezUInt8*>(sMsg.GetStartPointer()), sMsg.GetElementCount())));

    auto res = pServerTester->WaitForEvents(ezTime::MakeFromSeconds(1));
    EZ_TEST_BOOL(res.has_value() && res->m_Type == ezIpcChannelEvent::NewMessages);
    auto res2 = pClientTester->WaitForEvents(ezTime::MakeFromSeconds(1));
    EZ_TEST_BOOL(res2.has_value() && res2->m_Type == ezIpcChannelEvent::NewMessages);

    auto res3 = pClientTester->WaitForMessage(ezTime::MakeFromSeconds(1));
    EZ_TEST_BOOL(res3.has_value() && MessageMatches(sMsg, res3.value()));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ServerDisconnect")
  {
    pServer->Disconnect();
    pServer->Disconnect();

    auto res = pServerTester->WaitForEvents(ezTime::MakeFromSeconds(1));
    EZ_TEST_BOOL(res.has_value() && res->m_Type == ezIpcChannelEvent::Disconnected);
    auto res2 = pClientTester->WaitForEvents(ezTime::MakeFromSeconds(1));
    EZ_TEST_BOOL(res2.has_value() && res2->m_Type == ezIpcChannelEvent::Disconnected);

    EZ_TEST_BOOL(pServer->GetConnectionState() == ezIpcChannel::ConnectionState::Disconnected);
    EZ_TEST_BOOL(pClient->GetConnectionState() == ezIpcChannel::ConnectionState::Disconnected);
  }
}

/* TODO: Enet does not connect in process.
EZ_CREATE_SIMPLE_TEST(Communication, IpcChannel_Network)
{
  ezUniquePtr<ezIpcChannel> pServer = ezIpcChannel::CreateNetworkChannel("127.0.0.1:1050"_ezsv, ezIpcChannel::Mode::Server);
  ezUniquePtr<ChannelTester> pServerTester = EZ_DEFAULT_NEW(ChannelTester, pServer.Borrow(), true);

  ezUniquePtr<ezIpcChannel> pClient = ezIpcChannel::CreateNetworkChannel("127.0.0.1:1050"_ezsv, ezIpcChannel::Mode::Client);
  ezUniquePtr<ChannelTester> pClientTester = EZ_DEFAULT_NEW(ChannelTester, pClient.Borrow(), false);

  TestIPCChannel(pServer.Borrow(), pServerTester.Borrow(), pClient.Borrow(), pClientTester.Borrow());

  pClientTester.Clear();
  pClient.Clear();

  pServerTester.Clear();
  pServer.Clear();
}
*/

EZ_CREATE_SIMPLE_TEST(Communication, IpcChannel_Pipe)
{
  ezUniquePtr<ezIpcChannel> pServer = ezIpcChannel::CreatePipeChannel("ezEngine_unit_test_channel", ezIpcChannel::Mode::Server);
  ezUniquePtr<ChannelTester> pServerTester = EZ_DEFAULT_NEW(ChannelTester, pServer.Borrow(), true);

  ezUniquePtr<ezIpcChannel> pClient = ezIpcChannel::CreatePipeChannel("ezEngine_unit_test_channel", ezIpcChannel::Mode::Client);
  ezUniquePtr<ChannelTester> pClientTester = EZ_DEFAULT_NEW(ChannelTester, pClient.Borrow(), false);

  TestIPCChannel(pServer.Borrow(), pServerTester.Borrow(), pClient.Borrow(), pClientTester.Borrow());

  pClientTester.Clear();
  pClient.Clear();

  pServerTester.Clear();
  pServer.Clear();
}

#endif
