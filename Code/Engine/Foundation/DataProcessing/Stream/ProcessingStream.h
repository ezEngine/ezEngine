#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Strings/HashedString.h>

/// \brief A single stream in a stream group holding contiguous data of a given type.
class EZ_FOUNDATION_DLL ezProcessingStream
{
public:
  /// \brief The data types which can be stored in the stream.
  /// When adding new data types the GetDataTypeSize() of ezProcessingStream needs to be updated.
  enum class DataType : ezUInt8
  {
    Half,   // ezFloat16
    Half2,  // 2x ezFloat16
    Half3,  // 3x ezFloat16
    Half4,  // 4x ezFloat16

    Float,  // float
    Float2, // 2x float, e.g. ezVec2
    Float3, // 3x float, e.g. ezVec3
    Float4, // 4x float, e.g. ezVec4

    Byte,
    Byte2,
    Byte3,
    Byte4,

    Short,
    Short2,
    Short3,
    Short4,

    Int,
    Int2,
    Int3,
    Int4,

    Count
  };

  ezProcessingStream();
  ezProcessingStream(const ezHashedString& sName, DataType type, ezUInt16 uiStride, ezUInt16 uiAlignment);
  ezProcessingStream(const ezHashedString& sName, ezArrayPtr<ezUInt8> data, DataType type, ezUInt16 uiStride);
  ezProcessingStream(const ezHashedString& sName, ezArrayPtr<ezUInt8> data, DataType type);
  ~ezProcessingStream();

  /// \brief Returns a const pointer to the data casted to the type T, note that no type check is done!
  template <typename T>
  const T* GetData() const
  {
    return static_cast<const T*>(GetData());
  }

  /// \brief Returns a const pointer to the start of the data block.
  const void* GetData() const { return m_pData; }

  /// \brief Returns a non-const pointer to the data casted to the type T, note that no type check is done!
  template <typename T>
  T* GetWritableData() const
  {
    return static_cast<T*>(GetWritableData());
  }

  /// \brief Returns a non-const pointer to the start of the data block.
  void* GetWritableData() const { return m_pData; }

  ezUInt64 GetDataSize() const { return m_uiDataSize; }

  /// \brief Returns the name of the stream
  const ezHashedString& GetName() const { return m_sName; }

  /// \brief Returns the alignment which was used to allocate the stream.
  ezUInt16 GetAlignment() const { return m_uiAlignment; }

  /// \brief Returns the data type of the stream.
  DataType GetDataType() const { return m_Type; }

  /// \brief Returns the size of one stream element in bytes.
  ezUInt16 GetElementSize() const { return m_uiTypeSize; }

  /// \brief Returns the stride between two elements of the stream in bytes.
  ezUInt16 GetElementStride() const { return m_uiStride; }

  static ezUInt16 GetDataTypeSize(DataType type);
  static ezStringView GetDataTypeName(DataType type);

protected:
  friend class ezProcessingStreamGroup;

  void SetSize(ezUInt64 uiNumElements);
  void FreeData();

  void* m_pData = nullptr;
  ezUInt64 m_uiDataSize = 0; // in bytes

  ezUInt16 m_uiAlignment = 0;
  ezUInt16 m_uiTypeSize = 0;
  ezUInt16 m_uiStride = 0;
  DataType m_Type;
  bool m_bExternalMemory = false;

  ezHashedString m_sName;
};
