#pragma once

#include <Foundation/Strings/HashedString.h>
#include <GameEngine/GameEngineDLL.h>

class ezGameObject;
class ezLogInterface;

enum class [[nodiscard]] ezAiActionResult
{
  Succeded, ///< Finished for this frame, but needs to be executed again.
  Finished, ///< Completely finished (or canceled), does not need to be executed again.
  Failed,   ///< Failed and should not be executed again.
};

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

class EZ_AIPLUGIN_DLL ezAiAction
{
public:
  ezAiAction() = default;
  virtual ~ezAiAction() = default;

  virtual void Reset() = 0;
  virtual void GetDebugDesc(ezStringBuilder& inout_sText) = 0;
  virtual ezAiActionResult Execute(ezGameObject& owner, ezTime tDiff, ezLogInterface* pLog) = 0;
  virtual void Cancel(ezGameObject& owner) = 0;

private:
  friend class ezAiActionQueue;
  virtual void Destroy() = 0;
};
