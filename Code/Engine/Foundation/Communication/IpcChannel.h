#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Communication/RemoteInterface.h>
#include <Foundation/Threading/ThreadSignal.h>
#include <Foundation/Types/UniquePtr.h>

class ezIpcChannel;
class ezMessageLoop;

/// \brief Event data for ezIpcChannel::m_Events
struct EZ_FOUNDATION_DLL ezIpcChannelEvent
{
  enum Type
  {
    Disconnected, ///< Server or client are in a dorment state.
    Connecting,   ///< The server is listening for clients or the client is trying to find the server.
    Connected,    ///< Client and server are connected to each other.
    NewMessages,  ///< Sent when a new messages have been received or when disconnected to wake up any thread waiting for messages.
  };

  ezIpcChannelEvent() = default;

  ezIpcChannelEvent(Type type, ezIpcChannel* pChannel)
    : m_Type(type)
    , m_pChannel(pChannel)
  {
  }

  Type m_Type = NewMessages;
  ezIpcChannel* m_pChannel = nullptr;
};



/// \brief Base class for a communication channel between processes.
///
///  The channel allows for byte blobs to be send back and forth between two processes.
///  A client should only try to connect to a server once the server has changed to ConnectionState::Connecting as this indicates the server is ready to be conneccted to.
///
///  Use ezIpcChannel:::CreatePipeChannel to create an IPC pipe instance.
///  To send more complex messages accross, you can create a ezIpcProcessMessageProtocol on top of the channel.
class EZ_FOUNDATION_DLL ezIpcChannel
{
public:
  struct Mode
  {
    using StorageType = ezUInt8;
    enum Enum
    {
      Server,
      Client,
      Default = Server
    };
  };

  struct ConnectionState
  {
    using StorageType = ezUInt8;
    enum Enum
    {
      Disconnected,
      Connecting, ///< In case of the server, this state indicates that the server is ready to be connected to.
      Connected,
      Default = Disconnected
    };
  };

  virtual ~ezIpcChannel();

  /// \brief Creates an IPC communication channel using pipes.
  /// \param szAddress Name of the pipe, must be unique on a system and less than 200 characters.
  /// \param mode Whether to run in client or server mode.
  static ezInternal::NewInstance<ezIpcChannel> CreatePipeChannel(ezStringView sAddress, Mode::Enum mode);

  static ezInternal::NewInstance<ezIpcChannel> CreateNetworkChannel(ezStringView sAddress, Mode::Enum mode);


  /// \brief Connects async. On success, m_Events will be broadcasted.
  void Connect();
  /// \brief Disconnect async. On completion, m_Events will be broadcasted.
  void Disconnect();
  /// \brief Returns whether we have a connection.
  bool IsConnected() const { return m_ConnectionState == ConnectionState::Connected; }
  /// \brief Returns the current state of the connection.
  ezEnum<ConnectionState> GetConnectionState() const { return ezEnum<ConnectionState>(m_ConnectionState); }

  /// \brief Sends a message. pMsg can be destroyed after the call.
  bool Send(ezArrayPtr<const ezUInt8> data);

  using ReceiveCallback = ezDelegate<void(ezArrayPtr<const ezUInt8> message)>;
  void SetReceiveCallback(ReceiveCallback callback);

  /// \brief Block and wait for new messages and call ProcessMessages.
  ezResult WaitForMessages(ezTime timeout);

public:
  ezEvent<const ezIpcChannelEvent&, ezMutex> m_Events; ///< Will be sent from any thread.

protected:
  ezIpcChannel(ezStringView sAddress, Mode::Enum mode);

  /// \brief Override this and return true, if the surrounding infrastructure should call the 'Tick()' function.
  virtual bool RequiresRegularTick() { return false; }
  /// \brief Can implement regular updates, e.g. for polling network state.
  virtual void Tick() {}

  /// \brief Called on worker thread after Connect was called.
  virtual void InternalConnect() = 0;
  /// \brief Called on worker thread after Disconnect was called.
  virtual void InternalDisconnect() = 0;
  /// \brief Called on worker thread to sent pending messages.
  virtual void InternalSend() = 0;
  /// \brief Called by Send to determine whether the message loop need to be woken up.
  virtual bool NeedWakeup() const = 0;

  void SetConnectionState(ezEnum<ConnectionState> state);
  /// \brief Implementation needs to call this when new data has been received.
  ///  data can be invalidated after the function.
  void ReceiveData(ezArrayPtr<const ezUInt8> data);
  void FlushPendingOperations();

private:
protected:
  enum Constants : ezUInt32
  {
    HEADER_SIZE = 8,                     ///< Magic value and size ezUint32
    MAGIC_VALUE = 'USED',                ///< Magic value
    MAX_MESSAGE_SIZE = 1024 * 1024 * 16, ///< Arbitrary message size limit
  };

  friend class ezMessageLoop;
  ezThreadID m_ThreadId = 0;

  ezAtomicInteger<ConnectionState::Enum> m_ConnectionState = ConnectionState::Disconnected;

  // Setup in ctor
  ezString m_sAddress;
  const ezEnum<Mode> m_Mode;
  ezMessageLoop* m_pOwner = nullptr;

  // Mutex locked
  ezMutex m_OutputQueueMutex;
  ezDeque<ezContiguousMemoryStreamStorage> m_OutputQueue;

  // Only accessed from worker thread
  ezDynamicArray<ezUInt8> m_MessageAccumulator; ///< Message is assembled in here

  // Mutex locked
  ezMutex m_ReceiveCallbackMutex;
  ReceiveCallback m_ReceiveCallback;
  ezThreadSignal m_IncomingMessages;
};
