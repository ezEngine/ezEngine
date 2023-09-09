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
    Connected,
    Disconnected,
    NewMessages, ///< Sent when a new message has been received.
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
///  Use ezIpcChannel:::CreatePipeChannel to create an IPC pipe instance.
class EZ_FOUNDATION_DLL ezIpcChannel
{
public:
  struct Mode
  {
    enum Enum
    {
      Server,
      Client
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
  bool IsConnected() const { return m_bConnected; }


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
  ezAtomicBool m_bConnected = false;

  // Setup in ctor
  ezString m_sAddress;
  const Mode::Enum m_Mode;
  ezMessageLoop* m_pOwner = nullptr;

  // Mutex locked
  ezMutex m_OutputQueueMutex;
  ezDeque<ezContiguousMemoryStreamStorage> m_OutputQueue;

  // Only accessed from worker thread
  ezDynamicArray<ezUInt8> m_MessageAccumulator; ///< Message is assembled in here

  // Mutex locked
  ezMutex m_IncomingQueueMutex;
  ReceiveCallback m_ReceiveCallback;
  ezThreadSignal m_IncomingMessages;
};
