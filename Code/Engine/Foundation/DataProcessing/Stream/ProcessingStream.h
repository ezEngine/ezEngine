#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Strings/HashedString.h>

/// \brief A single stream in a stream group holding contiguous data of a given type.
class EZ_FOUNDATION_DLL ezProcessingStream
{
public:
  /// \brief Destructor.
  ~ezProcessingStream();

  /// \brief The data types which can be stored in the stream.
  /// When adding new data types the GetDataTypeSize() of ezProcessingStream needs to be updated.
  enum class DataType
  {
    Half,  // ezFloat16
    Half2, // 2x ezFloat16
    Half3, // 3x ezFloat16
    Half4, // 4x ezFloat16

    Float,  // float
    Float2, // 2x float, e.g. ezVec2
    Float3, // 3x float, e.g. ezVec3
    Float4, // 4x float, e.g. ezVec4

    Matrix4x4,

    //Byte, -> memory corruptions?
    //Byte2,
    /*Byte3 - bad alignment*/
    //Byte4,

    //Short,
    Short2,
    //Short3,
    Short4,

    Int,
    Int2,
    Int3,
    Int4
  };

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

  /// \brief Returns the name of the stream
  const ezHashedString& GetName() const { return m_Name; }

  /// \brief Returns the alignment which was used to allocate the stream.
  const ezUInt64 GetAlignment() const { return m_uiAlignment; }

  /// \brief Returns the data type of the stream.
  DataType GetDataType() const { return m_Type; }

  /// \brief Returns the size of one stream element.
  const ezUInt64 GetElementSize() const { return m_uiTypeSize; }

  /// \brief Returns the stride between two elements of the stream.
  const ezUInt64 GetElementStride() const
  {
    // For now this is the type size, but this method will help with introduction of interleaved streams etc.
    return m_uiTypeSize;
  }

  static size_t GetDataTypeSize(DataType Type);

protected:
  friend class ezProcessingStreamGroup;

  ezProcessingStream(const char* szName, DataType Type, ezUInt64 uiAlignment = 64);

  void SetSize(ezUInt64 uiNumElements);

  void FreeData();

  void* m_pData;

  ezUInt64 m_uiAlignment;

  ezUInt64 m_uiNumElements;

  ezUInt64 m_uiTypeSize;

  DataType m_Type;

  ezHashedString m_Name;
};

