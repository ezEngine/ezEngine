#pragma once

#include <ParticlePlugin/ParticlePluginDLL.h>
#include <ParticlePlugin/Declarations.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamProcessor.h>
#include <Foundation/DataProcessing/Stream/ProcessingStream.h>

class ezParticleStream;
class ezParticleSystemInstance;

/// \brief Base class for all particle stream factories
class EZ_PARTICLEPLUGIN_DLL ezParticleStreamFactory : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleStreamFactory, ezReflectedClass);

public:
  ezParticleStreamFactory(const char* szStreamName, ezProcessingStream::DataType dataType, const ezRTTI* pStreamTypeToCreate);

  const ezRTTI* GetParticleStreamType() const;
  ezProcessingStream::DataType GetStreamDataType() const;
  const char* GetStreamName() const;

  static void GetFullStreamName(const char* szName, ezProcessingStream::DataType type, ezStringBuilder& out_Result);

  ezParticleStream* CreateParticleStream(ezParticleSystemInstance* pOwner) const;

private:
  const char* m_szStreamName = nullptr;
  ezProcessingStream::DataType m_DataType = ezProcessingStream::DataType::Float;
  const ezRTTI* m_pStreamTypeToCreate = nullptr;
};

/// \brief Base class for all particle streams
class EZ_PARTICLEPLUGIN_DLL ezParticleStream : public ezProcessingStreamProcessor
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleStream, ezProcessingStreamProcessor);

  friend class ezParticleSystemInstance;
  friend class ezParticleStreamFactory;

protected:
  ezParticleStream();
  virtual void Initialize(ezParticleSystemInstance* pOwner) {}
  virtual ezResult UpdateStreamBindings() final override;
  virtual void Process(ezUInt64 uiNumElements) final override {}

  /// \brief The default implementation initializes all data with zero.
  virtual void InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements) override;

  ezProcessingStream* m_pStream;

private:
  ezParticleStreamBinding m_StreamBinding;
};
