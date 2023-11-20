#pragma once

#include <AiPlugin/AiPluginDLL.h>
#include <Foundation/Strings/HashedString.h>
#include <GameEngine/GameEngineDLL.h>

class ezGameObject;
class ezLogInterface;

/// Return value of ezAiAction::Execute().
enum class [[nodiscard]] ezAiActionResult
{
  Succeded, ///< Finished for this frame, but needs to be executed again.
  Finished, ///< Completely finished (or canceled), does not need to be executed again.
  Failed,   ///< Failed and should not be executed again. The ezAiActionQueue then cancels all remaining actions.
};

/// Pool allocator for actions, used through EZ_DECLARE_AICMD / EZ_IMPLEMENT_AICMD.
///
/// Released actions are kept in a free list and reused by the next Acquire(), memory is never freed.
template <typename TYPE>
class ezAiActionAlloc
{
public:
  TYPE* Acquire()
  {
    EZ_LOCK(m_Mutex);

    TYPE* pType = nullptr;

    if (m_FreeList.IsEmpty())
    {
      pType = &m_Allocated.ExpandAndGetRef();
    }
    else
    {
      pType = m_FreeList.PeekBack();
      m_FreeList.PopBack();
    }

    pType->Reset();
    return pType;
  }

  void Release(TYPE* pType)
  {
    EZ_LOCK(m_Mutex);
    EZ_ASSERT_DEBUG(!m_FreeList.Contains(pType), "");
    m_FreeList.PushBack(pType);
  }

private:
  ezMutex m_Mutex;
  ezHybridArray<TYPE*, 16> m_FreeList;
  ezDeque<TYPE> m_Allocated;
};

/// Adds a static Create() function to an action type, which returns a pooled instance (see ezAiActionAlloc).
///
/// Instances from Create() are owned by the ezAiActionQueue they are queued in, which returns them to the pool.
/// An action can also be used as a regular member (e.g. one action embedding another), Destroy() then only resets it.
#define EZ_DECLARE_AICMD(OwnType)           \
public:                                     \
  static OwnType* Create()                  \
  {                                         \
    OwnType* pType = s_Allocator.Acquire(); \
    pType->m_bFromAllocator = true;         \
    return pType;                           \
  }                                         \
                                            \
private:                                    \
  virtual void Destroy() override           \
  {                                         \
    Reset();                                \
    if (m_bFromAllocator)                   \
      s_Allocator.Release(this);            \
  }                                         \
  bool m_bFromAllocator = false;            \
  static ezAiActionAlloc<OwnType> s_Allocator;

#define EZ_IMPLEMENT_AICMD(OwnType) \
  ezAiActionAlloc<OwnType> OwnType::s_Allocator;

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/// A small step that an NPC executes, such as "navigate to X", "wait 2 seconds" or "set blackboard entry".
///
/// Behaviors put actions into an ezAiActionQueue, which executes them one after another, every frame.
/// Actions are configured through public members right after Create() and are not changed from the outside afterwards.
///
/// Typical implementations treat their 'unconfigured' state (e.g. speed == 0, empty name) as 'canceled':
/// Cancel() resets that state, so the next Execute() returns Finished right away.
class EZ_AIPLUGIN_DLL ezAiAction
{
public:
  ezAiAction() = default;
  virtual ~ezAiAction() = default;

  /// Sets all members to their default values. Called when an action is taken from the pool and when it is destroyed.
  virtual void Reset() = 0;

  /// Describes the action and its parameters for debug output.
  virtual void GetDebugDesc(ezStringBuilder& inout_sText) = 0;

  /// Called every frame while this is the first action in the queue.
  ///
  /// Actions further back in the queue are not executed until all previous ones returned Finished.
  virtual ezAiActionResult Execute(ezGameObject& owner, ezTime tDiff, ezLogInterface* pLog) = 0;

  /// Requests the action to stop as soon as possible. Also called on actions that haven't started yet.
  ///
  /// The action stays in the queue and still gets executed, so it can clean up (e.g. stop navigation).
  /// Some actions intentionally ignore this (e.g. ezAiActionBlackboardSetEntry::m_bNoCancel), to restore state that a previous action changed.
  virtual void Cancel(ezGameObject& owner) = 0;

private:
  friend class ezAiActionQueue;
  virtual void Destroy() = 0;
};
