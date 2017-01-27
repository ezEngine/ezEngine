#pragma once

#include <ParticlePlugin/Basics.h>
#include <ParticlePlugin/Declarations.h>
#include <Foundation/Reflection/Implementation/DynamicRTTI.h>
#include <CoreUtils/DataProcessing/Stream/ProcessingStreamProcessor.h>
#include <CoreUtils/DataProcessing/Stream/ProcessingStream.h>

class ezParticleStream;
class ezParticleSystemInstance;

/// \brief Base class for all particle stream factories
class EZ_PARTICLEPLUGIN_DLL ezParticleStreamFactory : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleStreamFactory, ezReflectedClass);

public:
  virtual const ezRTTI* GetParticleStreamType() const = 0;
  virtual ezProcessingStream::DataType GetStreamDataType() const = 0;
  virtual const char* GetStreamName() const = 0;

  static void GetFullStreamName(const char* szName, ezProcessingStream::DataType type, ezStringBuilder& out_Result);

  ezParticleStream* CreateParticleStream(ezParticleSystemInstance* pOwner) const;
};

/// \brief Base class for all particle streams
class EZ_PARTICLEPLUGIN_DLL ezParticleStream : public ezProcessingStreamProcessor
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleStream, ezProcessingStreamProcessor);

  friend class ezParticleSystemInstance;
  friend class ezParticleStreamFactory;

protected:
  virtual ezResult UpdateStreamBindings() final override;
  virtual void Process(ezUInt64 uiNumElements) final override {}

  /// \brief The default implementation initializes all data with zero.
  virtual void InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements) override;

  ezProcessingStream* m_pStream;

private:
  ezParticleStreamBinding m_StreamBinding;
};
