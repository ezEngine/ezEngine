#pragma once

#include <Foundation/Basics.h>

/// \brief Collection of helper methods when working with endianess "problems"
struct EZ_FOUNDATION_DLL ezEndianHelper
{

  /// \brief Returns true if called on a big endian system, false otherwise.
  ///
  /// \note Note that usually the compile time decisions with the defines EZ_PLATFORM_LITTLE_ENDIAN, EZ_PLATFORM_BIG_ENDIAN is preferred.
  static inline bool IsBigEndian()
  {
    const int i = 1;
    return (*(char*)&i) == 0;
  }

  /// \brief Returns true if called on a little endian system, false otherwise.
  ///
  /// \note Note that usually the compile time decisions with the defines EZ_PLATFORM_LITTLE_ENDIAN, EZ_PLATFORM_BIG_ENDIAN is preferred.
  static inline bool IsLittleEndian()
  {
    return !IsBigEndian();
  }

  /// \brief Switches endianess of the given array of words (16 bit values).
  static inline void SwitchWords(ezUInt16* pWords, ezUInt32 uiCount) // [tested]
  {
    for (ezUInt32 i = 0; i < uiCount; i++)
      pWords[i] = Switch(pWords[i]);
  }

  /// \brief Switches endianess of the given array of double words (32 bit values).
  static inline void SwitchDWords(ezUInt32* pDWords, ezUInt32 uiCount) // [tested]
  {
    for (ezUInt32 i = 0; i < uiCount; i++)
      pDWords[i] = Switch(pDWords[i]);
  }

  /// \brief Switches endianess of the given array of quad words (64 bit values).
  static inline void SwitchQWords(ezUInt64* pQWords, ezUInt32 uiCount) // [tested]
  {
    for (ezUInt32 i = 0; i < uiCount; i++)
      pQWords[i] = Switch(pQWords[i]);
  }

  /// \brief Returns a single switched word (16 bit value).
  static EZ_FORCE_INLINE ezUInt16 Switch(ezUInt16 uiWord) // [tested]
  {
    return (((uiWord & 0xFF) << 8) | ((uiWord >> 8) & 0xFF));
  }

  /// \brief Returns a single switched double word (32 bit value).
  static EZ_FORCE_INLINE ezUInt32 Switch(ezUInt32 uiDWord) // [tested]
  {
    return (((uiDWord & 0xFF) << 24) | (((uiDWord >> 8) & 0xFF) << 16) | (((uiDWord >> 16) & 0xFF) << 8) | ((uiDWord >> 24) & 0xFF));
  }

  /// \brief Returns a single switched quad word (64 bit value).
  static EZ_FORCE_INLINE ezUInt64 Switch(ezUInt64 uiQWord) // [tested]
  {
    return (((uiQWord & 0xFF) << 56) | ((uiQWord & 0xFF00) << 40) | ((uiQWord & 0xFF0000) << 24) | ((uiQWord & 0xFF000000) << 8) | ((uiQWord & 0xFF00000000) >> 8) | ((uiQWord & 0xFF0000000000) >> 24) | ((uiQWord & 0xFF000000000000) >> 40) | ((uiQWord & 0xFF00000000000000) >> 56));
  }

  /// \brief Switches a value in place (template accepts pointers for 2, 4 & 8 byte data types)
  template <typename T> static void SwitchInPlace(T* pValue) // [tested]
  {
    EZ_CHECK_AT_COMPILETIME_MSG((sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8), "Switch in place only works for type equivalents of ezUInt16, ezUInt32, ezUInt64!");

    if (sizeof(T) == 2)
    {
      struct TAnd16BitUnion
      {
        union { ezUInt16 BitValue; T TValue; };
      };

      TAnd16BitUnion Temp;
      Temp.TValue = *pValue;
      Temp.BitValue = Switch(Temp.BitValue);

      *pValue = Temp.TValue;
    }
    else if (sizeof(T) == 4)
    {
      struct TAnd32BitUnion
      {
        union { ezUInt32 BitValue; T TValue; };
      };

      TAnd32BitUnion Temp;
      Temp.TValue = *pValue;
      Temp.BitValue = Switch(Temp.BitValue);

      *pValue = Temp.TValue;
    }
    else if (sizeof(T) == 8)
    {
      struct TAnd64BitUnion
      {
        union { ezUInt64 BitValue; T TValue; };
      };

      TAnd64BitUnion Temp;
      Temp.TValue = *pValue;
      Temp.BitValue = Switch(Temp.BitValue);

      *pValue = Temp.TValue;
    }
  }

  #if EZ_ENABLED(EZ_PLATFORM_LITTLE_ENDIAN)

  static EZ_FORCE_INLINE void LittleEndianToNative(ezUInt16* pWords, ezUInt32 uiCount)
  {
  }

  static EZ_FORCE_INLINE void NativeToLittleEndian(ezUInt16* pWords, ezUInt32 uiCount)
  {
  }

  static EZ_FORCE_INLINE void LittleEndianToNative(ezUInt32* pDWords, ezUInt32 uiCount)
  {
  }

  static EZ_FORCE_INLINE void NativeToLittleEndian(ezUInt32* pDWords, ezUInt32 uiCount)
  {
  }

  static EZ_FORCE_INLINE void LittleEndianToNative(ezUInt64* pQWords, ezUInt32 uiCount)
  {
  }

  static EZ_FORCE_INLINE void NativeToLittleEndian(ezUInt64* pQWords, ezUInt32 uiCount)
  {
  }

  static EZ_FORCE_INLINE void BigEndianToNative(ezUInt16* pWords, ezUInt32 uiCount)
  {
    SwitchWords(pWords, uiCount);
  }

  static EZ_FORCE_INLINE void NativeToBigEndian(ezUInt16* pWords, ezUInt32 uiCount)
  {
    SwitchWords(pWords, uiCount);
  }

  static EZ_FORCE_INLINE void BigEndianToNative(ezUInt32* pDWords, ezUInt32 uiCount)
  {
    SwitchDWords(pDWords, uiCount);
  }

  static EZ_FORCE_INLINE void NativeToBigEndian(ezUInt32* pDWords, ezUInt32 uiCount)
  {
    SwitchDWords(pDWords, uiCount);
  }

  static EZ_FORCE_INLINE void BigEndianToNative(ezUInt64* pQWords, ezUInt32 uiCount)
  {
    SwitchQWords(pQWords, uiCount);
  }

  static EZ_FORCE_INLINE void NativeToBigEndian(ezUInt64* pQWords, ezUInt32 uiCount)
  {
    SwitchQWords(pQWords, uiCount);
  }

  #elif EZ_ENABLED(EZ_PLATFORM_BIG_ENDIAN)

  static EZ_FORCE_INLINE void LittleEndianToNative(ezUInt16* pWords, ezUInt32 uiCount)
  {
    SwitchWords(pWords, uiCount);
  }

  static EZ_FORCE_INLINE void NativeToLittleEndian(ezUInt16* pWords, ezUInt32 uiCount)
  {
    SwitchWords(pWords, uiCount);
  }

  static EZ_FORCE_INLINE void LittleEndianToNative(ezUInt32* pDWords, ezUInt32 uiCount)
  {
    SwitchDWords(pDWords, uiCount);
  }

  static EZ_FORCE_INLINE void NativeToLittleEndian(ezUInt32* pDWords, ezUInt32 uiCount)
  {
    SwitchDWords(pDWords, uiCount);
  }

  static EZ_FORCE_INLINE void LittleEndianToNative(ezUInt64* pQWords, ezUInt32 uiCount)
  {
    SwitchQWords(pQWords, uiCount);
  }

  static EZ_FORCE_INLINE void NativeToLittleEndian(ezUInt64* pQWords, ezUInt32 uiCount)
  {
    SwitchQWords(pQWords, uiCount);
  }

  static EZ_FORCE_INLINE void BigEndianToNative(ezUInt16* pWords, ezUInt32 uiCount)
  {
  }

  static EZ_FORCE_INLINE void NativeToBigEndian(ezUInt16* pWords, ezUInt32 uiCount)
  {
  }

  static EZ_FORCE_INLINE void BigEndianToNative(ezUInt32* pDWords, ezUInt32 uiCount)
  {
  }

  static EZ_FORCE_INLINE void NativeToBigEndian(ezUInt32* pDWords, ezUInt32 uiCount)
  {
  }

  static EZ_FORCE_INLINE void BigEndianToNative(ezUInt64* pQWords, ezUInt32 uiCount)
  {
  }

  static EZ_FORCE_INLINE void NativeToBigEndian(ezUInt64* pQWords, ezUInt32 uiCount)
  {
  }

  #endif


  /// \brief Switches a given struct according to the layout described in the szFormat parameter
  ///
  /// The format string may contain the characters:
  ///  - c, b for a member of 1 byte
  ///  - w, s for a member of 2 bytes (word, ezUInt16)
  ///  - d for a member of 4 bytes (dword, ezUInt32)
  ///  - q for a member of 8 bytes (qword, ezUInt64)
  static void SwitchStruct(void* pDataPointer, const char* szFormat);

  /// \brief Templated helper method for SwitchStruct
  template <typename T> static void SwitchStruct(T* pDataPointer, const char* szFormat) // [tested]
  {
    SwitchStruct(static_cast<void*>(pDataPointer), szFormat);
  }

  /// \brief Switches a given set of struct according to the layout described in the szFormat parameter
  ///
  /// The format string may contain the characters:
  ///  - c, b for a member of 1 byte
  ///  - w, s for a member of 2 bytes (word, ezUInt16)
  ///  - d for a member of 4 bytes (dword, ezUInt32)
  ///  - q for a member of 8 bytes (qword, ezUInt64)
  static void SwitchStructs(void* pDataPointer, const char* szFormat, ezUInt32 uiStride, ezUInt32 uiCount); // [tested]

  /// \brief Templated helper method for SwitchStructs
  template <typename T> static void SwitchStructs(T* pDataPointer, const char* szFormat, ezUInt32 uiCount) // [tested]
  {
    SwitchStructs(static_cast<void*>(pDataPointer), szFormat, sizeof(T), uiCount);
  }


};

