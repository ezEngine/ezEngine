#include <Foundation/FoundationPCH.h>

#include <Foundation/Basics.h>
#include <Foundation/DataProcessing/Stream/ProcessingStream.h>

static_assert(sizeof(ezProcessingStream) == 32);

ezProcessingStream::ezProcessingStream() = default;

ezProcessingStream::ezProcessingStream(const ezHashedString& sName, DataType Type, ezUInt16 uiStride, ezUInt16 uiAlignment)
  : m_uiAlignment(uiAlignment)
  , m_uiTypeSize(GetDataTypeSize(Type))
  , m_uiStride(uiStride)
  , m_Type(Type)
  , m_sName(sName)
{
}

ezProcessingStream::ezProcessingStream(const ezHashedString& sName, ezArrayPtr<ezUInt8> data, DataType Type, ezUInt16 uiStride)
  : m_pData(data.GetPtr())
  , m_uiDataSize(data.GetCount())
  , m_uiTypeSize(GetDataTypeSize(Type))
  , m_uiStride(uiStride)
  , m_Type(Type)
  , m_bExternalMemory(true)
  , m_sName(sName)
{
}

ezProcessingStream::ezProcessingStream(const ezHashedString& sName, ezArrayPtr<ezUInt8> data, DataType Type)
  : m_pData(data.GetPtr())
  , m_uiDataSize(data.GetCount())
  , m_uiTypeSize(GetDataTypeSize(Type))
  , m_uiStride(m_uiTypeSize)
  , m_Type(Type)
  , m_bExternalMemory(true)
  , m_sName(sName)
{
}

ezProcessingStream::~ezProcessingStream()
{
  FreeData();
}

void ezProcessingStream::SetSize(ezUInt64 uiNumElements)
{
  ezUInt64 uiNewDataSize = uiNumElements * m_uiTypeSize;
  if (m_uiDataSize == uiNewDataSize)
    return;

  FreeData();

  if (uiNewDataSize == 0)
  {
    return;
  }

  /// \todo Allow to reuse memory from a pool ?
  if (m_uiAlignment > 0)
  {
    m_pData = ezFoundation::GetAlignedAllocator()->Allocate(static_cast<size_t>(uiNewDataSize), static_cast<size_t>(m_uiAlignment));
  }
  else
  {
    m_pData = ezFoundation::GetDefaultAllocator()->Allocate(static_cast<size_t>(uiNewDataSize), 0);
  }

  EZ_ASSERT_DEV(m_pData != nullptr, "Allocating {0} elements of {1} bytes each, with {2} bytes alignment, failed", uiNumElements, ((ezUInt32)GetDataTypeSize(m_Type)), m_uiAlignment);
  m_uiDataSize = uiNewDataSize;
}

void ezProcessingStream::FreeData()
{
  if (m_pData != nullptr && m_bExternalMemory == false)
  {
    if (m_uiAlignment > 0)
    {
      ezFoundation::GetAlignedAllocator()->Deallocate(m_pData);
    }
    else
    {
      ezFoundation::GetDefaultAllocator()->Deallocate(m_pData);
    }
  }

  m_pData = nullptr;
  m_uiDataSize = 0;
}

static ezUInt16 s_TypeSize[] = {
  2, // Half,
  4, //Half2,
  6, //Half3,
  8, //Half4,

  4,  //Float,
  8,  //Float2,
  12, //Float3,
  16, //Float4,

  1, //Byte,
  2, //Byte2,
  3, //Byte3,
  4, //Byte4,

  2, //Short,
  4, //Short2,
  6, //Short3,
  8, //Short4,

  4,  //Int,
  8,  //Int2,
  12, //Int3,
  16, //Int4,
};
static_assert(EZ_ARRAY_SIZE(s_TypeSize) == (size_t)ezProcessingStream::DataType::Count);

// static
ezUInt16 ezProcessingStream::GetDataTypeSize(DataType Type)
{
  return s_TypeSize[(ezUInt32)Type];
}

EZ_STATICLINK_FILE(Foundation, Foundation_DataProcessing_Stream_Implementation_ProcessingStream);
