#pragma once

#include <Foundation/Containers/HybridArray.h>

/// \brief A very simple string class that should only be used to temporarily convert text to the OSes native wchar_t convention (16 or 32 Bit).
///
/// This should be used when one needs to output text via some function that only accepts wchar_t strings.
/// DO NOT use this for storage or anything else that is not temporary.
/// wchar_t is 16 Bit on Windows and 32 Bit on most other platforms. This class will always automatically convert to the correct format.
class EZ_FOUNDATION_DLL ezStringWChar
{
public:
  ezStringWChar(ezAllocatorBase* pAllocator = ezFoundation::GetDefaultAllocator());
  ezStringWChar(const char* szUtf8, ezAllocatorBase* pAllocator = ezFoundation::GetDefaultAllocator());
  ezStringWChar(const ezUInt16* szUtf16, ezAllocatorBase* pAllocator = ezFoundation::GetDefaultAllocator());
  ezStringWChar(const ezUInt32* szUtf32, ezAllocatorBase* pAllocator = ezFoundation::GetDefaultAllocator());
  ezStringWChar(const wchar_t* szUtf32, ezAllocatorBase* pAllocator = ezFoundation::GetDefaultAllocator());

  void operator=(const char* szUtf8);
  void operator=(const ezUInt16* szUtf16);
  void operator=(const ezUInt32* szUtf32);
  void operator=(const wchar_t* szUtf32);

  const wchar_t* GetData() const { return &m_Data[0]; }
  ezUInt32 GetElementCount() const { return m_Data.GetCount() - 1; /* exclude the '\0' terminator */ }

private:
  // It is not intended to copy these things around.
  EZ_DISALLOW_COPY_AND_ASSIGN(ezStringWChar);

  static const ezUInt32 BufferSize = 1024;
  ezHybridArray<wchar_t, BufferSize> m_Data;
};


/// \brief A small string class that converts any other encoding to Utf8.
///
/// Use this class only temporarily. Do not use it for storage.
class EZ_FOUNDATION_DLL ezStringUtf8
{
public:
  ezStringUtf8(ezAllocatorBase* pAllocator = ezFoundation::GetDefaultAllocator());
  ezStringUtf8(const char* szUtf8, ezAllocatorBase* pAllocator = ezFoundation::GetDefaultAllocator());
  ezStringUtf8(const ezUInt16* szUtf16, ezAllocatorBase* pAllocator = ezFoundation::GetDefaultAllocator());
  ezStringUtf8(const ezUInt32* szUtf32, ezAllocatorBase* pAllocator = ezFoundation::GetDefaultAllocator());
  ezStringUtf8(const wchar_t* szUtf32, ezAllocatorBase* pAllocator = ezFoundation::GetDefaultAllocator());

  void operator=(const char* szUtf8);
  void operator=(const ezUInt16* szUtf16);
  void operator=(const ezUInt32* szUtf32);
  void operator=(const wchar_t* szUtf32);

  const char* GetData() const { return &m_Data[0]; }
  ezUInt32 GetElementCount() const { return m_Data.GetCount() - 1; /* exclude the '\0' terminator */ }

private:
  // It is not intended to copy these things around.
  EZ_DISALLOW_COPY_AND_ASSIGN(ezStringUtf8);

  static const ezUInt32 BufferSize = 1024;
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
  ezStringUtf16(ezAllocatorBase* pAllocator = ezFoundation::GetDefaultAllocator());
  ezStringUtf16(const char* szUtf8, ezAllocatorBase* pAllocator = ezFoundation::GetDefaultAllocator());
  ezStringUtf16(const ezUInt16* szUtf16, ezAllocatorBase* pAllocator = ezFoundation::GetDefaultAllocator());
  ezStringUtf16(const ezUInt32* szUtf32, ezAllocatorBase* pAllocator = ezFoundation::GetDefaultAllocator());
  ezStringUtf16(const wchar_t* szUtf32, ezAllocatorBase* pAllocator = ezFoundation::GetDefaultAllocator());

  void operator=(const char* szUtf8);
  void operator=(const ezUInt16* szUtf16);
  void operator=(const ezUInt32* szUtf32);
  void operator=(const wchar_t* szUtf32);

  const ezUInt16* GetData() const { return &m_Data[0]; }
  ezUInt32 GetElementCount() const { return m_Data.GetCount() - 1; /* exclude the '\0' terminator */ }

private:
  // It is not intended to copy these things around.
  EZ_DISALLOW_COPY_AND_ASSIGN(ezStringUtf16);

  static const ezUInt32 BufferSize = 1024;
  ezHybridArray<ezUInt16, BufferSize> m_Data;
};



/// \brief This class only exists for completeness.
///
/// There should be no case where it is preferred over other classes.
class EZ_FOUNDATION_DLL ezStringUtf32
{
public:
  ezStringUtf32(ezAllocatorBase* pAllocator = ezFoundation::GetDefaultAllocator());
  ezStringUtf32(const char* szUtf8, ezAllocatorBase* pAllocator = ezFoundation::GetDefaultAllocator());
  ezStringUtf32(const ezUInt16* szUtf16, ezAllocatorBase* pAllocator = ezFoundation::GetDefaultAllocator());
  ezStringUtf32(const ezUInt32* szUtf32, ezAllocatorBase* pAllocator = ezFoundation::GetDefaultAllocator());
  ezStringUtf32(const wchar_t* szUtf32, ezAllocatorBase* pAllocator = ezFoundation::GetDefaultAllocator());

  void operator=(const char* szUtf8);
  void operator=(const ezUInt16* szUtf16);
  void operator=(const ezUInt32* szUtf32);
  void operator=(const wchar_t* szUtf32);

  const ezUInt32* GetData() const { return &m_Data[0]; }
  ezUInt32 GetElementCount() const { return m_Data.GetCount() - 1; /* exclude the '\0' terminator */ }

private:
  // It is not intended to copy these things around.
  EZ_DISALLOW_COPY_AND_ASSIGN(ezStringUtf32);

  static const ezUInt32 BufferSize = 1024;
  ezHybridArray<ezUInt32, BufferSize> m_Data;
};

#include <Foundation/Strings/Implementation/StringConversion_inl.h>

