#pragma once

#include <Foundation/Basics.h>

namespace ezMemoryPolicies
{
  /// \brief Implements a bounds checking policy that does no bounds checking.
  ///
  /// Used to configure allocators to do no bounds checking.
  class ezNoBoundsChecking
  {
  public:
    enum 
    {
      GuardSizeFront = 0,
      GuardSizeBack = 0
    };

    /// \brief Does nothing.
    void GuardFront(void*, size_t) const { }

    /// \brief Does nothing.
    void GuardBack(void*) const { }

    /// \brief Does nothing.
    void CheckFront(const void*, size_t) const { }

    /// \brief Does nothing.
    void CheckBack(const void*) const { }

    /// \brief Does nothing, returns zero.
    size_t GetGuardOffset(const void*) const { return 0; }
  };

  class ezGuardedBoundsChecking
  {
  public:
    enum
    {
      GuardSizeFront = 4,
      GuardSizeBack = 4
    };

    void GuardFront(void* ptr, size_t uiGuardSize) const 
    {
      EZ_ASSERT(uiGuardSize < 256, "Invalid guard size");
      EZ_CHECK_ALIGNMENT(uiGuardSize, sizeof(ezUInt32));

      ezUInt32* pGuards = static_cast<ezUInt32*>(ptr);
      ezUInt32* pGuardsEnd = ezMemoryUtils::AddByteOffset(pGuards, uiGuardSize);

      while (pGuards < pGuardsEnd)
      {
        *pGuards = 0x12349876;
        pGuards++;
      }

      // write the size of the front guard to the first byte
      ezUInt8* pGuardSize = ezMemoryUtils::AddByteOffset(static_cast<ezUInt8*>(ptr), uiGuardSize - GuardSizeFront);
      *pGuardSize = static_cast<ezUInt8>(uiGuardSize);
    }

    inline void GuardBack(void* ptr) const 
    {
      ezUInt32* pGuards = ezMemoryUtils::AddByteOffset(static_cast<ezUInt32*>(ptr), -GuardSizeBack);
      *pGuards = 0x98761234;
    }

    void CheckFront(const void* ptr, size_t uiGuardSize) const 
    {
      const ezUInt32* pGuards = static_cast<const ezUInt32*>(ptr);
      const ezUInt32* pGuardsEnd = ezMemoryUtils::AddByteOffsetConst(pGuards, uiGuardSize);

      while (pGuards < pGuardsEnd)
      {
        EZ_ASSERT_ALWAYS(((*pGuards) & 0xffffff00) == 0x12349800, "Front guard was overwritten");
        pGuards++;
      }
    }

    inline void CheckBack(const void* ptr) const 
    {
      const ezUInt32* pGuards = ezMemoryUtils::AddByteOffsetConst(static_cast<const ezUInt32*>(ptr), -GuardSizeBack);
      EZ_ASSERT_ALWAYS(*pGuards == 0x98761234, "Back guard was overwritten");
    }

    inline size_t GetGuardOffset(const void* ptr) const 
    { 
      return *ezMemoryUtils::AddByteOffsetConst(static_cast<const ezUInt8*>(ptr), -GuardSizeFront);
    }
  };
}
