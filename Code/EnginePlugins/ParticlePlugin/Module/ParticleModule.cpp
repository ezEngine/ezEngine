#include <ParticlePlugin/ParticlePluginPCH.h>

#include <ParticlePlugin/Module/ParticleModule.h>
#include <ParticlePlugin/Streams/ParticleStream.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleModule, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleModuleProcessor, 1, ezRTTIDefaultAllocator<ezParticleModuleProcessor>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void ezParticleModuleProcessor::SetModules(ezParticleSystemInstance* pSystem, ezArrayPtr<const ezParticleModuleInstance*> instances)
{
  m_pSystem = pSystem;
  m_Instances = instances;

  m_Instances.Sort([](const ezParticleModuleInstance* a, const ezParticleModuleInstance* b)
    { return a->m_pModule->GetPriority() < b->m_pModule->GetPriority(); });
}

void ezParticleModuleProcessor::AddStreamInitializer(const ezParticleStream* pParticleStream, ezProcessingStream* pStream)
{
  StreamInitializer& init = m_StreamInitializers.ExpandAndGetRef();
  init.m_pParticleStream = pParticleStream;
  init.m_pStream = pStream;
}

void ezParticleModuleProcessor::RemoveStreamInitializer(const ezProcessingStream* pStream)
{
  for (ezUInt32 i = 0; i < m_StreamInitializers.GetCount(); ++i)
  {
    if (m_StreamInitializers[i].m_pStream == pStream)
    {
      m_StreamInitializers.RemoveAtAndCopy(i);
      return;
    }
  }
}

void ezParticleModuleProcessor::InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements)
{
  for (const StreamInitializer& init : m_StreamInitializers)
  {
    if (init.m_pParticleStream != nullptr)
    {
      init.m_pParticleStream->InitializeElements(m_pSystem, init.m_pStream, uiStartIndex, uiNumElements);
    }
    else
    {
      ezParticleStream::ZeroFillElements(init.m_pStream, uiStartIndex, uiNumElements);
    }
  }

  for (const ezParticleModuleInstance* pInstance : m_Instances)
  {
    pInstance->m_pModule->InitializeElements(pInstance->m_Context, uiStartIndex, uiNumElements);
  }
}

void ezParticleModuleProcessor::Process(ezUInt64 uiNumElements)
{
  for (const ezParticleModuleInstance* pInstance : m_Instances)
  {
    pInstance->m_pModule->Process(pInstance->m_Context, uiNumElements);
  }
}


EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Module_ParticleModule);
