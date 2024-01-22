#pragma once

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Types/Delegate.h>

/// \brief Identifies an event subscription. Zero is always an invalid subscription ID.
using ezEventSubscriptionID = ezUInt32;

/// \brief Specifies the type of ezEvent implementation to use
enum class ezEventType
{
  Default,        /// Default implementation. Does not support modifying the event while broadcasting.
  CopyOnBroadcast /// CopyOnBroadcast implementation. Supports modifying the event while broadcasting.
};

/// \brief This class propagates event information to registered event handlers.
///
/// An event can be anything that "happens" that might be of interest to other code, such
/// that it can react on it in some way.
/// Just create an instance of ezEvent and call Broadcast() on it. Other interested code needs const access to
/// the event variable to be able to call AddEventHandler() and RemoveEventHandler().
/// To pass information to the handlers, create a custom struct with event information
/// and then pass a (const) reference to that data through Broadcast().
///
/// If you need to modify the event while broadcasting, for example inside one of the registered event handlers,
/// set EventType = ezEventType::CopyOnBroadcast. Each broadcast will then copy the event handler array before signaling them, allowing
/// modifications during broadcasting.
///
/// \note A class holding an ezEvent member needs to provide public access to the member for external code to
/// be able to register as an event handler. To make it possible to prevent external code from also raising events,
/// all functions that are needed for listening are const, and all others are non-const.
/// Therefore, simply make event members private and provide const reference access through a public getter.
template <typename EventData, typename MutexType, ezEventType EventType>
class ezEventBase
{
protected:
  /// \brief Constructor.
  ezEventBase(ezAllocator* pAllocator);
  ~ezEventBase();

public:
  /// \brief Notification callback type for events.
  using Handler = ezDelegate<void(EventData)>;

  /// \brief An object that can be passed to ezEvent::AddEventHandler to store the subscription information
  /// and automatically remove the event handler upon destruction.
  class Unsubscriber
  {
    EZ_DISALLOW_COPY_AND_ASSIGN(Unsubscriber);

  public:
    Unsubscriber() = default;
    Unsubscriber(Unsubscriber&& other)
    {
      m_pEvent = other.m_pEvent;
      m_SubscriptionID = other.m_SubscriptionID;
      other.Clear();
    }
    ~Unsubscriber() { Unsubscribe(); }

    void operator=(Unsubscriber&& other)
    {
      Unsubscribe();

      m_pEvent = other.m_pEvent;
      m_SubscriptionID = other.m_SubscriptionID;
      other.Clear();
    }

    /// \brief If the unsubscriber holds a valid subscription, it will be removed from the target ezEvent.
    void Unsubscribe()
    {
      if (m_SubscriptionID == 0)
        return;

      m_pEvent->RemoveEventHandler(m_SubscriptionID);
      Clear();
    }

    /// \brief Checks whether this unsubscriber has a valid subscription.
    bool IsSubscribed() const { return m_SubscriptionID != 0; }

    /// \brief Resets the unsubscriber. Use when the target ezEvent may have been destroyed and automatic unsubscription cannot be executed
    /// anymore.
    void Clear()
    {
      m_pEvent = nullptr;
      m_SubscriptionID = 0;
    }

  private:
    friend class ezEventBase<EventData, MutexType, EventType>;

    const ezEventBase<EventData, MutexType, EventType>* m_pEvent = nullptr;
    ezEventSubscriptionID m_SubscriptionID = 0;
  };

  /// \brief Implementation specific constants.
  enum
  {
    /// Whether the uiMaxRecursionDepth parameter to Broadcast() is supported in this implementation or not.
    RecursionDepthSupported = (EventType == ezEventType::Default || ezConversionTest<MutexType, ezNoMutex>::sameType == 1) ? 1 : 0,

    /// Default value for the maximum recursion depth of Broadcast.
    /// As limiting the recursion depth is not supported when EventType == ezEventType::CopyAndBroadcast and MutexType != ezNoMutex
    /// the default value for that case is the maximum.
    MaxRecursionDepthDefault = RecursionDepthSupported ? 0 : 255
  };

  /// \brief This function will broadcast to all registered users, that this event has just happened.
  ///  Setting uiMaxRecursionDepth will allow you to permit recursions. When broadcasting consider up to what depth
  ///  you want recursions to be permitted. By default no recursion is allowed.
  void Broadcast(EventData pEventData, ezUInt8 uiMaxRecursionDepth = MaxRecursionDepthDefault); // [tested]

  /// \brief Adds a function as an event handler. All handlers will be notified in the order that they were registered.
  ///
  /// The return value can be stored and used to remove the event handler later again.
  ezEventSubscriptionID AddEventHandler(Handler handler) const; // [tested]

  /// \brief An overload that adds an event handler and initializes the given \a Unsubscriber object.
  ///
  /// When the Unsubscriber is destroyed, it will automatically remove the event handler.
  void AddEventHandler(Handler handler, Unsubscriber& inout_unsubscriber) const; // [tested]

  /// \brief Removes a previously registered handler. It is an error to remove a handler that was not registered.
  void RemoveEventHandler(const Handler& handler) const; // [tested]

  /// \brief Removes a previously registered handler via the returned subscription ID.
  ///
  /// The ID will be reset to zero.
  /// If this is called with a zero ID, nothing happens.
  void RemoveEventHandler(ezEventSubscriptionID& inout_id) const;

  /// \brief Checks whether an event handler has already been registered.
  bool HasEventHandler(const Handler& handler) const;

  /// \brief Removes all registered event handlers.
  void Clear();

  /// \brief Returns true, if no event handlers are registered.
  bool IsEmpty() const;

  // it would be a problem if the ezEvent moves in memory, for instance the Unsubscriber's would point to invalid memory
  EZ_DISALLOW_COPY_AND_ASSIGN(ezEventBase);

private:
  // Used to detect recursive broadcasts and then throw asserts at you.
  ezUInt8 m_uiRecursionDepth = 0;
  mutable ezEventSubscriptionID m_NextSubscriptionID = 0;

  mutable MutexType m_Mutex;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  const void* m_pSelf = nullptr;
#endif

  struct HandlerData
  {
    Handler m_Handler;
    ezEventSubscriptionID m_SubscriptionID;
  };

  /// \brief A dynamic array allows to have zero overhead as long as no event handlers are registered.
  mutable ezDynamicArray<HandlerData> m_EventHandlers;
};

/// \brief Can be used when ezEvent is used without any additional data
struct ezNoEventData
{
};

/// \brief \see ezEventBase
template <typename EventData, typename MutexType = ezNoMutex, typename AllocatorWrapper = ezDefaultAllocatorWrapper, ezEventType EventType = ezEventType::Default>
class ezEvent : public ezEventBase<EventData, MutexType, EventType>
{
public:
  ezEvent();
  ezEvent(ezAllocator* pAllocator);
};

template <typename EventData, typename MutexType = ezNoMutex, typename AllocatorWrapper = ezDefaultAllocatorWrapper>
using ezCopyOnBroadcastEvent = ezEvent<EventData, MutexType, AllocatorWrapper, ezEventType::CopyOnBroadcast>;

#include <Foundation/Communication/Implementation/Event_inl.h>
