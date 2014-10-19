#pragma once

#include <Foundation/Types/Delegate.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/Mutex.h>

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

public:
  /// \brief Notification callback type for events.
  typedef ezDelegate<void (EventData)> Handler;

  /// \brief This function will broadcast to all registered users, that this event has just happened.
  void Broadcast(EventData pEventData); // [tested]

  /// \brief Adds a function as an event handler. All handlers will be notified in the order that they were registered.
  void AddEventHandler(Handler handler) const; // [tested]

  /// \brief Removes a previously registered handler. It is an error to remove a handler that was not registered.
  void RemoveEventHandler(Handler handler) const; // [tested]

  EZ_DISALLOW_COPY_AND_ASSIGN(ezEventBase);

private:
  /// \brief Used to detect recursive broadcasts and then throw asserts at you.
  bool m_bBroadcasting;

  mutable MutexType m_Mutex;

  /// \brief A dynamic array allows to have zero overhead as long as no event receivers are registered.
  mutable ezDynamicArray<Handler> m_EventHandlers;  
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

