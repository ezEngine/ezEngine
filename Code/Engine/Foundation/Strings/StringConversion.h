#pragma once

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/StringView.h>

/// \brief A very simple string class that should only be used to temporarily convert text to the OSes native wchar_t convention (16 or 32
/// Bit).
///
/// This should be used when one needs to output text via some function that only accepts wchar_t strings.
/// DO NOT use this for storage or anything else that is not temporary.
/// wchar_t is 16 Bit on Windows and 32 Bit on most other platforms. This class will always automatically convert to the correct format.
class EZ_FOUNDATION_DLL ezStringWChar
{
public:
  ezStringWChar(ezAllocator* pAllocator = ezFoundation::GetDefaultAllocator());
  ezStringWChar(const ezUInt16* pUtf16, ezAllocator* pAllocator = ezFoundation::GetDefaultAllocator());
  ezStringWChar(const ezUInt32* pUtf32, ezAllocator* pAllocator = ezFoundation::GetDefaultAllocator());
  ezStringWChar(const wchar_t* pUtf32, ezAllocator* pAllocator = ezFoundation::GetDefaultAllocator());
  ezStringWChar(ezStringView sUtf8, ezAllocator* pAllocator = ezFoundation::GetDefaultAllocator());

  void operator=(const ezUInt16* pUtf16);
  void operator=(const ezUInt32* pUtf32);
  void operator=(const wchar_t* pUtf32);
  void operator=(ezStringView sUtf8);

  EZ_ALWAYS_INLINE operator const wchar_t*() const { return &m_Data[0]; }
  EZ_ALWAYS_INLINE const wchar_t* GetData() const { return &m_Data[0]; }
  EZ_ALWAYS_INLINE ezUInt32 GetElementCount() const { return m_Data.GetCount() - 1; /* exclude the '\0' terminator */ }

private:
  static constexpr ezUInt32 BufferSize = 1024;
  ezHybridArray<wchar_t, BufferSize> m_Data;
};


/// \brief A small string class that converts any other encoding to Utf8.
///
/// Use this class only temporarily. Do not use it for storage.
class EZ_FOUNDATION_DLL ezStringUtf8
{
public:
  ezStringUtf8(ezAllocator* pAllocator = ezFoundation::GetDefaultAllocator());
  ezStringUtf8(const char* szUtf8, ezAllocator* pAllocator = ezFoundation::GetDefaultAllocator());
  ezStringUtf8(const ezUInt16* pUtf16, ezAllocator* pAllocator = ezFoundation::GetDefaultAllocator());
  ezStringUtf8(const ezUInt32* pUtf32, ezAllocator* pAllocator = ezFoundation::GetDefaultAllocator());
  ezStringUtf8(const wchar_t* pWChar, ezAllocator* pAllocator = ezFoundation::GetDefaultAllocator());

  void operator=(const char* szUtf8);
  void operator=(const ezUInt16* pUtf16);
  void operator=(const ezUInt32* pUtf32);
  void operator=(const wchar_t* pWChar);

  EZ_ALWAYS_INLINE operator const char*() const
  {
    return &m_Data[0];
  }
  EZ_ALWAYS_INLINE const char* GetData() const
  {
    return &m_Data[0];
  }
  EZ_ALWAYS_INLINE ezUInt32 GetElementCount() const
  {
    return m_Data.GetCount() - 1; /* exclude the '\0' terminator */
  }
  EZ_ALWAYS_INLINE operator ezStringView() const
  {
    return GetView();
  }
  EZ_ALWAYS_INLINE ezStringView GetView() const
  {
    return ezStringView(&m_Data[0], GetElementCount());
  }

private:
  static constexpr ezUInt32 BufferSize = 1024;
  ezHybridArray<char, BufferSize> m_Data;
};



/// \brief A very simple class to convert text to Utf16 encoding.
///
/// Use this class only temporarily, if you need to output something in Utf16 format, e.g. for writing it to a file.
/// Never use this for storage.
/// When working with OS functions that expect '16 Bit strings', use ezStringWChar instead.
class EZ_FOUNDATION_DLL ezStringUtf16
{
public:
  ezStringUtf16(ezAllocator* pAllocator = ezFoundation::GetDefaultAllocator());
  ezStringUtf16(const char* szUtf8, ezAllocator* pAllocator = ezFoundation::GetDefaultAllocator());
  ezStringUtf16(const ezUInt16* pUtf16, ezAllocator* pAllocator = ezFoundation::GetDefaultAllocator());
  ezStringUtf16(const ezUInt32* pUtf32, ezAllocator* pAllocator = ezFoundation::GetDefaultAllocator());
  ezStringUtf16(const wchar_t* pUtf32, ezAllocator* pAllocator = ezFoundation::GetDefaultAllocator());

  void operator=(const char* szUtf8);
  void operator=(const ezUInt16* pUtf16);
  void operator=(const ezUInt32* pUtf32);
  void operator=(const wchar_t* pUtf32);

  EZ_ALWAYS_INLINE const ezUInt16* GetData() const { return &m_Data[0]; }
  EZ_ALWAYS_INLINE ezUInt32 GetElementCount() const { return m_Data.GetCount() - 1; /* exclude the '\0' terminator */ }

private:
  static constexpr ezUInt32 BufferSize = 1024;
  ezHybridArray<ezUInt16, BufferSize> m_Data;
};



/// \brief This class only exists for completeness.
///
/// There should be no case where it is preferred over other classes.
class EZ_FOUNDATION_DLL ezStringUtf32
{
public:
  ezStringUtf32(ezAllocator* pAllocator = ezFoundation::GetDefaultAllocator());
  ezStringUtf32(const char* szUtf8, ezAllocator* pAllocator = ezFoundation::GetDefaultAllocator());
  ezStringUtf32(const ezUInt16* pUtf16, ezAllocator* pAllocator = ezFoundation::GetDefaultAllocator());
  ezStringUtf32(const ezUInt32* pUtf32, ezAllocator* pAllocator = ezFoundation::GetDefaultAllocator());
  ezStringUtf32(const wchar_t* pWChar, ezAllocator* pAllocator = ezFoundation::GetDefaultAllocator());

  void operator=(const char* szUtf8);
  void operator=(const ezUInt16* pUtf16);
  void operator=(const ezUInt32* pUtf32);
  void operator=(const wchar_t* pWChar);

  EZ_ALWAYS_INLINE const ezUInt32* GetData() const { return &m_Data[0]; }
  EZ_ALWAYS_INLINE ezUInt32 GetElementCount() const { return m_Data.GetCount() - 1; /* exclude the '\0' terminator */ }

private:
  static constexpr ezUInt32 BufferSize = 1024;
  ezHybridArray<ezUInt32, BufferSize> m_Data;
};

#include <Foundation/Strings/Implementation/StringConversion_inl.h>
