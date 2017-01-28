
#include <Foundation/PCH.h>
#include <Foundation/Basics.h>
#include <Foundation/DataProcessing/Stream/DefaultImplementations/ZeroInitializer.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/DataProcessing/Stream/ProcessingStream.h>
#include <Foundation/Memory/MemoryUtils.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezProcessingStreamSpawnerZeroInitialized, 1, ezRTTIDefaultAllocator<ezProcessingStreamSpawnerZeroInitialized>)
EZ_END_DYNAMIC_REFLECTED_TYPE

ezProcessingStreamSpawnerZeroInitialized::ezProcessingStreamSpawnerZeroInitialized()
  : m_pStream(nullptr)
{
}

void ezProcessingStreamSpawnerZeroInitialized::SetStreamName(const char* szStreamName)
{
  m_StreamName.Assign(szStreamName);
}

ezResult ezProcessingStreamSpawnerZeroInitialized::UpdateStreamBindings()
{
  EZ_ASSERT_DEBUG(!m_StreamName.IsEmpty(), "ezProcessingStreamSpawnerZeroInitialized: Stream name has not been configured");

  m_pStream = m_pStreamGroup->GetStreamByName(m_StreamName);
  return m_pStream ? EZ_SUCCESS : EZ_FAILURE;
}


void ezProcessingStreamSpawnerZeroInitialized::InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements)
{
  const ezUInt64 uiElementSize = m_pStream->GetElementSize();
  const ezUInt64 uiElementStride = m_pStream->GetElementStride();

  for (ezUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
  {
    ezMemoryUtils::ZeroFill<ezUInt8>(static_cast<ezUInt8*>(ezMemoryUtils::AddByteOffset(m_pStream->GetWritableData(), static_cast<ptrdiff_t>(i * uiElementStride))), static_cast<size_t>(uiElementSize));
  }
}



EZ_STATICLINK_FILE(Foundation, Foundation_DataProcessing_Stream_DefaultImplementations_Implementation_ZeroInitializer);

