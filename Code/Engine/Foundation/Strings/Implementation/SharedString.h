#pragma once

#include <Foundation/Threading/AtomicInteger.h>
#include <Foundation/Strings/StringConversion.h>

class ezSharedString;

namespace ezInternal
{
  /// For internal use by the other string classes.
  class ezSharedStringBase
  {
  public:
    ezSharedStringBase() {}

  private:
    EZ_DISALLOW_COPY_AND_ASSIGN(ezSharedStringBase);

    // Only a few classes shall use the ezSharedStringBase internally.
    // Add all these classes as friends here, if you need to add another string type that uses shared data.
    friend class ezSharedString;

    static ezSharedStringBase* CreateSharedString(const char* szString, const char* szStringEnd = ezMaxStringEnd)
    {
      EZ_ASSERT(ezUnicodeUtils::IsValidUtf8(szString), "Input Data is not a valid Utf8 string. Did you intend to use a Wide-String and forget the 'L' prefix?");

      ezUInt32 uiCharacterCount, uiElementCount;
      ezStringUtils::GetCharacterAndElementCount(szString, uiCharacterCount, uiElementCount, szStringEnd);

      ezUInt32 uiSize = ezMath::Ceil((ezInt32) (sizeof(ezSharedStringBase) + uiElementCount + 1), 4);
      ezUInt8* pData = EZ_DEFAULT_NEW_RAW_BUFFER(ezUInt8, uiSize);

      ezSharedStringBase* pString = reinterpret_cast<ezSharedStringBase*>(pData);
      ezMemoryUtils::Construct<ezSharedStringBase>(pString, 1);

      pString->m_szString = reinterpret_cast<char*> (pData + sizeof(ezSharedStringBase));
      ezMemoryUtils::Copy(pString->m_szString, szString, uiElementCount + 1);
      pString->m_szString[uiElementCount] = '\0';

      pString->m_uiCapacity = uiSize - (ezUInt32) sizeof(ezSharedStringBase);
      pString->m_uiCharacterCount = uiCharacterCount;
      pString->m_uiElementCount = uiElementCount;
      pString->Acquire();

      return pString;
    }

    EZ_FORCE_INLINE void Acquire()
    {
      m_RefCount.Increment();
    }

    inline void Release()
    {
      if (m_RefCount.Decrement() == 0)
      {
        ezSharedStringBase* pPtr = this;

        // this will not only delete this shared string, but also the string buffer which is located right after it
        EZ_DEFAULT_DELETE_RAW_BUFFER(pPtr);
      }
    }

  private:
    char* m_szString;
    ezAtomicInteger32 m_RefCount;
    ezUInt32 m_uiCharacterCount;
    ezUInt32 m_uiElementCount;
    ezUInt32 m_uiCapacity; // how many bytes were allocated (including for the \0 terminator)
  };

}



