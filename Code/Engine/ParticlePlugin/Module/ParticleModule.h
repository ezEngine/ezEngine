#pragma once

#include <ParticlePlugin/Basics.h>
#include <Foundation/DataProcessing/Stream/ProcessingStream.h>
#include <ParticlePlugin/Declarations.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamProcessor.h>

class ezProcessingStream;

class EZ_PARTICLEPLUGIN_DLL ezParticleModule : public ezProcessingStreamProcessor
{
  friend class ezParticleSystemInstance;

public:

  /// \brief Called after construction when m_pOwnerSystem is set and when properties have been set
  virtual void AfterPropertiesConfigured(bool bFirstTime) {}
  virtual void CreateRequiredStreams() = 0;
  virtual void QueryOptionalStreams() {}

  void Reset(ezParticleSystemInstance* pOwner)
  {
    m_pOwnerSystem = pOwner;
    m_StreamBinding.Clear();

    OnReset();
  }

  ezParticleSystemInstance* GetOwnerSystem()
  {
    return m_pOwnerSystem;
  }

  const ezParticleSystemInstance* GetOwnerSystem() const
  {
    return m_pOwnerSystem;
  }

  ezParticleEffectInstance* GetOwnerEffect() const
  {
    return m_pOwnerSystem->GetOwnerEffect();
  }

protected:
  /// \brief Called by Reset()
  virtual void OnReset() {}

  void CreateStream(const char* szName, ezProcessingStream::DataType Type, ezProcessingStream** ppStream, bool bWillInitializeStream)
  {
    m_pOwnerSystem->CreateStream(szName, Type, ppStream, m_StreamBinding, bWillInitializeStream);
  }

  virtual ezResult UpdateStreamBindings() final override
  {
    m_StreamBinding.UpdateBindings(m_pStreamGroup);
    return EZ_SUCCESS;
  }

  ezRandom& GetRNG() const
  {
    return m_pOwnerSystem->GetRNG();
  }

private:
  ezParticleSystemInstance* m_pOwnerSystem;
  ezParticleStreamBinding m_StreamBinding;
};
