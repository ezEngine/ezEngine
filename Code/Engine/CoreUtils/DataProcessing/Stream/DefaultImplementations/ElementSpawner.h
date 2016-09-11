
#pragma once

#include <CoreUtils/Basics.h>
#include <CoreUtils/DataProcessing/Stream/StreamElementSpawner.h>
#include <Foundation/Strings/HashedString.h>

class ezProcessingStream;

/// \brief This element spawner initializes new elements with 0 (by writing 0 bytes into the whole element)
class EZ_COREUTILS_DLL ezProcessingStreamSpawnerZeroInitialized : public ezProcessingStreamSpawner
{
  public:

    ezProcessingStreamSpawnerZeroInitialized(const char* szStreamName);

    virtual ~ezProcessingStreamSpawnerZeroInitialized();

  protected:

    virtual ezResult UpdateStreamBindings() override;

    virtual void SpawnElements( ezUInt64 uiStartIndex, ezUInt64 uiNumElements ) override;

    ezHashedString m_StreamName;

    ezProcessingStream* m_pStream;
};
