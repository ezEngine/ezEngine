
#include <CoreUtils/PCH.h>
#include <CoreUtils/Basics.h>
#include <CoreUtils/DataProcessing/Stream/ProcessingStream.h>

ezProcessingStream::ezProcessingStream( const char* szName, ezProcessingStream::DataType Type, ezUInt64 uiAlignment /*= 64*/ )
  : m_pData(nullptr)
  , m_uiAlignment(uiAlignment)
  , m_uiNumElements(0)
  , m_uiTypeSize(GetDataTypeSize(Type))
  , m_Type(Type)
  , m_Name()
{
  m_Name.Assign( szName );
}

ezProcessingStream::~ezProcessingStream()
{
  FreeData();
}

void ezProcessingStream::SetSize( ezUInt64 uiNumElements )
{
  if ( m_uiNumElements == uiNumElements )
    return;

  FreeData();

  if ( uiNumElements == 0 )
  {
    return;
  }

  /// \todo Allow to reuse memory from a pool ?
  if ( m_uiAlignment > 0 )
  {
    m_pData = ezFoundation::GetAlignedAllocator()->Allocate( static_cast<size_t>(uiNumElements * GetDataTypeSize( m_Type )), static_cast<size_t>(m_uiAlignment) );
  }
  else
  {
    m_pData = ezFoundation::GetDefaultAllocator()->Allocate( static_cast<size_t>(uiNumElements * GetDataTypeSize( m_Type )), 0 );
  }

  EZ_ASSERT_DEV(m_pData != nullptr, "Allocating %u elements of %u bytes each, with %u bytes alignment, failed", uiNumElements, ((ezUInt32)GetDataTypeSize(m_Type)), m_uiAlignment);
  m_uiNumElements = uiNumElements;
}

void ezProcessingStream::FreeData()
{
  if ( m_pData )
  {
    if ( m_uiAlignment > 0 )
    {
      ezFoundation::GetAlignedAllocator()->Deallocate( m_pData );
    }
    else
    {
      ezFoundation::GetDefaultAllocator()->Deallocate( m_pData );
    }
  }

  m_uiNumElements = 0;
}

size_t ezProcessingStream::GetDataTypeSize( DataType Type )
{
  switch ( Type )
  {
    case DataType::Float:
    case DataType::Int:
      return 4;
    case DataType::Float2:
    case DataType::Int2:
      return 8;
    case DataType::Float3:
    case DataType::Int3:
      return 12;
    case DataType::Float4:
    case DataType::Int4:
      return 16;

    case DataType::Matrix4x4:
      return 64;
  }

  EZ_ASSERT_NOT_IMPLEMENTED;

  return 0;
}
