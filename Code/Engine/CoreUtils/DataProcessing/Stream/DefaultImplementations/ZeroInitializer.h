
#pragma once

#include <CoreUtils/Basics.h>
#include <CoreUtils/DataProcessing/Stream/ProcessingStreamSpawner.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Reflection/Reflection.h>

class ezProcessingStream;

/// \brief This element spawner initializes new elements with 0 (by writing 0 bytes into the whole element)
class EZ_COREUTILS_DLL ezProcessingStreamSpawnerZeroInitialized : public ezProcessingStreamSpawner
{
  EZ_ADD_DYNAMIC_REFLECTION(ezProcessingStreamSpawnerZeroInitialized, ezProcessingStreamSpawner);

public:

  ezProcessingStreamSpawnerZeroInitialized();

  /// \brief Which stream to zero initialize
  void SetStreamName(const char* szStreamName);

protected:

  virtual ezResult UpdateStreamBindings() override;

  virtual void SpawnElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements) override;

  ezHashedString m_StreamName;

  ezProcessingStream* m_pStream;
};
