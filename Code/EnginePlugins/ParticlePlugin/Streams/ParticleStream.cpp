#include <ParticlePluginPCH.h>

#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Memory/MemoryUtils.h>
#include <ParticlePlugin/Streams/ParticleStream.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleStreamFactory, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleStream, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezParticleStreamFactory::ezParticleStreamFactory(const char* szStreamName, ezProcessingStream::DataType dataType,
                                                 const ezRTTI* pStreamTypeToCreate)
{
  m_szStreamName = szStreamName;
  m_DataType = dataType;
  m_pStreamTypeToCreate = pStreamTypeToCreate;
}

const ezRTTI* ezParticleStreamFactory::GetParticleStreamType() const
{
  return m_pStreamTypeToCreate;
}

ezProcessingStream::DataType ezParticleStreamFactory::GetStreamDataType() const
{
  return m_DataType;
}

const char* ezParticleStreamFactory::GetStreamName() const
{
  return m_szStreamName;
}

void ezParticleStreamFactory::GetFullStreamName(const char* szName, ezProcessingStream::DataType type, ezStringBuilder& out_Result)
{
  out_Result = szName;
  out_Result.AppendFormat("({0})", (int)type);
}

ezParticleStream* ezParticleStreamFactory::CreateParticleStream(ezParticleSystemInstance* pOwner) const
{
  const ezRTTI* pRtti = GetParticleStreamType();
  EZ_ASSERT_DEBUG(pRtti->IsDerivedFrom<ezParticleStream>(), "Particle stream factory does not create a valid stream type");

  ezParticleStream* pStream = pRtti->GetAllocator()->Allocate<ezParticleStream>();

  pOwner->CreateStream(GetStreamName(), GetStreamDataType(), &pStream->m_pStream, pStream->m_StreamBinding, true);
  pStream->Initialize(pOwner);

  return pStream;
}

//////////////////////////////////////////////////////////////////////////

ezParticleStream::ezParticleStream()
{
  // make sure default stream initializers are run very first
  m_fPriority = -1000.0f;
}

ezResult ezParticleStream::UpdateStreamBindings()
{
  m_StreamBinding.UpdateBindings(m_pStreamGroup);
  return EZ_SUCCESS;
}

void ezParticleStream::InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements)
{
  const ezUInt64 uiElementSize = m_pStream->GetElementSize();
  const ezUInt64 uiElementStride = m_pStream->GetElementStride();

  for (ezUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
  {
    ezMemoryUtils::ZeroFill<ezUInt8>(
        static_cast<ezUInt8*>(ezMemoryUtils::AddByteOffset(m_pStream->GetWritableData(), static_cast<ptrdiff_t>(i * uiElementStride))),
        static_cast<size_t>(uiElementSize));
  }
}



EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Streams_ParticleStream);
