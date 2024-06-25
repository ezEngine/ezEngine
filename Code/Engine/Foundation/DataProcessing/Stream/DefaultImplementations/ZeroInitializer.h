
#pragma once

#include <Foundation/Basics.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamProcessor.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>

class ezProcessingStream;

/// \brief This element spawner initializes new elements with 0 (by writing 0 bytes into the whole element)
class EZ_FOUNDATION_DLL ezProcessingStreamSpawnerZeroInitialized : public ezProcessingStreamProcessor
{
  EZ_ADD_DYNAMIC_REFLECTION(ezProcessingStreamSpawnerZeroInitialized, ezProcessingStreamProcessor);

public:
  ezProcessingStreamSpawnerZeroInitialized();

  /// \brief Which stream to zero initialize
  void SetStreamName(ezStringView sStreamName);

protected:
  virtual ezResult UpdateStreamBindings() override;

  virtual void InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements) override;
  virtual void Process(ezUInt64 uiNumElements) override { EZ_IGNORE_UNUSED(uiNumElements); }

  ezHashedString m_sStreamName;

  ezProcessingStream* m_pStream = nullptr;
};
