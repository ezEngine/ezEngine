#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Types/Delegate.h>

/// \brief Identifies an event subscription. Zero is always an invalid subscription ID.
typedef ezUInt16 ezEventSubscriptionID;

/// \brief This class allows to propagate events to code that might be interested in them.
///
/// An event can be anything that "happens" that might be of interest for other code, such
/// that it can react on it in some way.
/// Just create an instance of ezEvent and call "Broadcast" on it. Other interested code needs access to
/// the variable (or at least to AddEventHandler / RemoveEventHandler) such that it can
/// register itself as an interested party. To pass information to the handlers, create your own
/// custom struct to package that information and then pass a pointer to that data through Broadcast.
/// The handlers just need to cast the void-pointer to the proper struct and thus can get all the detailed
/// information about the event.
template <typename EventData, typename MutexType>
class ezEventBase
{
protected:
  /// \brief Constructor.
  ezEventBase(ezAllocatorBase* pAllocator);
  ~ezEventBase();

public:
  /// \brief Notification callback type for events.
  typedef ezDelegate<void(EventData)> Handler;

  /// \brief An object that can be passed to ezEvent::AddEventHandler to store the subscription information
  /// and automatically remove the event handler upon destruction.
  class Unsubscriber
  {
  public:
    Unsubscriber() = default;
    ~Unsubscriber() { Unsubscribe(); }

    /// \brief If the unsubscriber holds a valid subscription, it will be removed from the target ezEvent.
    void Unsubscribe()
    {
      if (m_SubscriptionID == 0)
        return;

      m_pEvent->RemoveEventHandler(m_SubscriptionID);
      Clear();
    }

    /// \brief Resets the unsubscriber. Use when the target ezEvent may have been destroyed and automatic unsubscription cannot be executed
    /// anymore.
    void Clear()
    {
      m_pEvent = nullptr;
      m_SubscriptionID = 0;
    }

  private:
    friend class ezEventBase<EventData, MutexType>;

    const ezEventBase<EventData, MutexType>* m_pEvent = nullptr;
    ezEventSubscriptionID m_SubscriptionID = 0;
  };

  /// \brief This function will broadcast to all registered users, that this event has just happened.
  ///  Setting uiMaxRecursionDepth will allow you to permit recursions. When broadcasting consider up to what depth
  ///  you want recursions to be permitted. By default no recursion is allowed.
  void Broadcast(EventData pEventData, ezUInt8 uiMaxRecursionDepth = 0) const; // [tested]

  /// \brief Adds a function as an event handler. All handlers will be notified in the order that they were registered.
  ///
  /// The return value can be stored and used to remove the event handler later again.
  ezEventSubscriptionID AddEventHandler(Handler handler) const; // [tested]

  /// \brief An overload that adds an event handler and initializes the given \a Unsubscriber object.
  ///
  /// When the Unsubscriber is destroyed, it will automatically remove the event handler.
  void AddEventHandler(Handler handler, Unsubscriber& unsubscriber) const; // [tested]

  /// \brief Removes a previously registered handler. It is an error to remove a handler that was not registered.
  void RemoveEventHandler(Handler handler) const; // [tested]

  /// \brief Removes a previously registered handler via the returned subscription ID.
  ///
  /// The ID will be reset to zero.
  /// If this is called with a zero ID, nothing happens.
  void RemoveEventHandler(ezEventSubscriptionID& id) const;

  /// \brief Checks whether an event handler has already been registered.
  bool HasEventHandler(Handler handler) const;

  // it would be a problem if the ezEvent moves in memory, for instance the Unsubscriber's would point to invalid memory
  EZ_DISALLOW_COPY_AND_ASSIGN(ezEventBase);

private:
  /// \brief Used to detect recursive broadcasts and then throw asserts at you.
  mutable ezUInt8 m_uiRecursionDepth;
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

  /// \brief A dynamic array allows to have zero overhead as long as no event receivers are registered.
  mutable ezDynamicArray<HandlerData> m_EventHandlers;
};

/// \brief \see ezEventBase
template <typename EventData, typename MutexType = ezNoMutex, typename AllocatorWrapper = ezDefaultAllocatorWrapper>
class ezEvent : public ezEventBase<EventData, MutexType>
{
public:
  ezEvent();
  ezEvent(ezAllocatorBase* pAllocator);
};

#include <Foundation/Communication/Implementation/Event_inl.h>
