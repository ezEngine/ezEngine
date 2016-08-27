
#include <CoreUtils/PCH.h>
#include <CoreUtils/Basics.h>
#include <CoreUtils/DataProcessing/Stream/DefaultImplementations/ElementSpawner.h>
#include <CoreUtils/DataProcessing/Stream/StreamGroup.h>
#include <CoreUtils/DataProcessing/Stream/Stream.h>
#include <Foundation/Memory/MemoryUtils.h>

ezStreamElementSpawnerZeroInitialized::ezStreamElementSpawnerZeroInitialized( const char* szStreamName )
  : m_pStream(nullptr)
{
  m_StreamName.Assign( szStreamName );
}

ezStreamElementSpawnerZeroInitialized::~ezStreamElementSpawnerZeroInitialized()
{
  int i = 0;
  (void)i;
}


ezResult ezStreamElementSpawnerZeroInitialized::UpdateStreamBindings()
{
  m_pStream = m_pStreamGroup->GetStreamByName( m_StreamName );
  return m_pStream ? EZ_SUCCESS : EZ_FAILURE;
}


void ezStreamElementSpawnerZeroInitialized::SpawnElements( ezUInt64 uiStartIndex, ezUInt64 uiNumElements )
{
  const ezUInt64 uiElementSize = m_pStream->GetElementSize();
  const ezUInt64 uiElementStride = m_pStream->GetElementStride();

  for ( ezUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i )
  {
    ezMemoryUtils::ZeroFill<ezUInt8>( static_cast<ezUInt8*>(ezMemoryUtils::AddByteOffset( m_pStream->GetWritableData(), static_cast<ptrdiff_t>(i * uiElementStride) ) ), static_cast<size_t>(uiElementSize));
  }

}
