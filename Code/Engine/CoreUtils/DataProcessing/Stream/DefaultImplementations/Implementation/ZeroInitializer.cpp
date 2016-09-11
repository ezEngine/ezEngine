
#include <CoreUtils/PCH.h>
#include <CoreUtils/Basics.h>
#include <CoreUtils/DataProcessing/Stream/DefaultImplementations/ZeroInitializer.h>
#include <CoreUtils/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <CoreUtils/DataProcessing/Stream/ProcessingStream.h>
#include <Foundation/Memory/MemoryUtils.h>

ezProcessingStreamSpawnerZeroInitialized::ezProcessingStreamSpawnerZeroInitialized( const char* szStreamName )
  : m_pStream(nullptr)
{
  m_StreamName.Assign( szStreamName );
}

ezProcessingStreamSpawnerZeroInitialized::~ezProcessingStreamSpawnerZeroInitialized()
{
  int i = 0;
  (void)i;
}


ezResult ezProcessingStreamSpawnerZeroInitialized::UpdateStreamBindings()
{
  m_pStream = m_pStreamGroup->GetStreamByName( m_StreamName );
  return m_pStream ? EZ_SUCCESS : EZ_FAILURE;
}


void ezProcessingStreamSpawnerZeroInitialized::SpawnElements( ezUInt64 uiStartIndex, ezUInt64 uiNumElements )
{
  const ezUInt64 uiElementSize = m_pStream->GetElementSize();
  const ezUInt64 uiElementStride = m_pStream->GetElementStride();

  for ( ezUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i )
  {
    ezMemoryUtils::ZeroFill<ezUInt8>( static_cast<ezUInt8*>(ezMemoryUtils::AddByteOffset( m_pStream->GetWritableData(), static_cast<ptrdiff_t>(i * uiElementStride) ) ), static_cast<size_t>(uiElementSize));
  }

}
