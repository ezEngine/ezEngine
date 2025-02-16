#pragma once

#include "../TestClass/TestClass.h"

class ezRendererTestDynamicBuffer : public ezGraphicsTest
{
  using SUPER = ezGraphicsTest;

public:
  virtual const char* GetTestName() const override { return "Dynamic Buffer"; }

private:
  enum SubTests
  {
    ST_Allocations,
    ST_Deallocations,
    ST_Compaction,
  };

  virtual void SetupSubTests() override;

  virtual ezResult InitializeSubTest(ezInt32 iIdentifier) override;
  virtual ezResult DeInitializeSubTest(ezInt32 iIdentifier) override;
  virtual ezTestAppRun RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount) override;

private:
  ezGALDynamicBufferHandle m_hDynamicBuffer;
};
