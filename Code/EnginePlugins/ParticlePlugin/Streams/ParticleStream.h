#pragma once

#include <Foundation/DataProcessing/Stream/ProcessingStream.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>
#include <ParticlePlugin/Declarations.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

class ezParticleSystemInstance;

/// Base class for the default initializers of particle streams.
///
/// Each one specifies the stream name and data type, and how to initialize the stream for new particles,
/// in case no module takes care of that. It is used for all particle systems of a world, so it has no state per system.
class EZ_PARTICLEPLUGIN_DLL ezParticleStream : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleStream, ezReflectedClass);

public:
  ezParticleStream(ezTempHashedString sStreamName, ezProcessingStream::DataType dataType);

  ezProcessingStream::DataType GetStreamDataType() const;
  ezTempHashedString GetStreamName() const;

  /// Initializes the elements in the range [uiStartIndex, uiStartIndex + uiNumElements) for new particles.
  ///
  /// The default implementation initializes all data with zero.
  virtual void InitializeElements(ezParticleSystemInstance* pSystem, ezProcessingStream* pStream, ezUInt64 uiStartIndex, ezUInt64 uiNumElements) const;

  /// Writes zero to all bytes of the given elements.
  static void ZeroFillElements(ezProcessingStream* pStream, ezUInt64 uiStartIndex, ezUInt64 uiNumElements);

private:
  ezTempHashedString m_sStreamName;
  ezProcessingStream::DataType m_DataType = ezProcessingStream::DataType::Float;
};
