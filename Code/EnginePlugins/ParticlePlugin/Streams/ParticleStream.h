#pragma once

#include <Foundation/DataProcessing/Stream/ProcessingStream.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamProcessor.h>
#include <Foundation/Reflection/Reflection.h>
#include <ParticlePlugin/Declarations.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

class ezParticleStream;
class ezParticleSystemInstance;

/// \brief Base class for all particle stream factories
///
/// Stream factories are responsible for creating and configuring particle streams.
/// Each factory specifies the stream name, data type, and the actual stream class to instantiate.
class EZ_PARTICLEPLUGIN_DLL ezParticleStreamFactory : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleStreamFactory, ezReflectedClass);

public:
  ezParticleStreamFactory(const char* szStreamName, ezProcessingStream::DataType dataType, const ezRTTI* pStreamTypeToCreate);

  const ezRTTI* GetParticleStreamType() const;
  ezProcessingStream::DataType GetStreamDataType() const;
  const char* GetStreamName() const;

  /// Creates and initializes a new particle stream instance for the given particle system.
  ezParticleStream* CreateParticleStream(ezParticleSystemInstance* pOwner) const;

private:
  const char* m_szStreamName = nullptr;
  ezProcessingStream::DataType m_DataType = ezProcessingStream::DataType::Float;
  const ezRTTI* m_pStreamTypeToCreate = nullptr;
};

/// \brief Base class for all particle streams
///
/// Particle streams store per-particle data like position, velocity, color, or size.
/// Each stream type provides initialization logic for new particles.
/// Streams run with high priority (-1000) to ensure they initialize data before other processors.
class EZ_PARTICLEPLUGIN_DLL ezParticleStream : public ezProcessingStreamProcessor
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleStream, ezProcessingStreamProcessor);

  friend class ezParticleSystemInstance;
  friend class ezParticleStreamFactory;

protected:
  ezParticleStream();

  /// Called once during stream creation to set up any necessary references or state.
  virtual void Initialize(ezParticleSystemInstance* pOwner) {}

  virtual ezResult UpdateStreamBindings() final override;

  /// Particle streams do not process existing elements, they only initialize new ones.
  virtual void Process(ezUInt64 uiNumElements) final override {}

  /// \brief The default implementation initializes all data with zero.
  ///
  /// Override this to provide custom initialization for new particles.
  /// The implementation should initialize elements in the range [uiStartIndex, uiStartIndex + uiNumElements).
  virtual void InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements) override;

  ezProcessingStream* m_pStream; ///< The underlying data stream managed by this particle stream

private:
  ezParticleStreamBinding m_StreamBinding;
};
