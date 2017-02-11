
#pragma once

#include <Foundation/Basics.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamProcessor.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Reflection/Reflection.h>

class ezProcessingStream;

/// \brief This element spawner initializes new elements with 0 (by writing 0 bytes into the whole element)
class EZ_FOUNDATION_DLL ezProcessingStreamSpawnerZeroInitialized : public ezProcessingStreamProcessor
{
  EZ_ADD_DYNAMIC_REFLECTION(ezProcessingStreamSpawnerZeroInitialized, ezProcessingStreamProcessor);

public:

  ezProcessingStreamSpawnerZeroInitialized();

  /// \brief Which stream to zero initialize
  void SetStreamName(const char* szStreamName);

protected:

  virtual ezResult UpdateStreamBindings() override;

  virtual void InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements) override;
  virtual void Process(ezUInt64 uiNumElements) override {}

  ezHashedString m_StreamName;

  ezProcessingStream* m_pStream;
};

