#pragma once

#include "../TestClass/TestClass.h"
#include <Foundation/Types/SharedPtr.h>

class ezRenderGraphTest : public ezGraphicsTest
{
public:
  virtual const char* GetTestName() const override { return "RenderGraph"; }

private:
  enum SubTests
  {
    ST_DeadPassCulling,
    ST_ResourceAliasing,
    ST_ImportReplace,
    ST_ExecuteCallbacks,
    ST_EmptyGraph,
    ST_StressTest,
    ST_MsaaResolve,
  };

  virtual void SetupSubTests() override;

  virtual ezResult InitializeSubTest(ezInt32 iIdentifier) override;
  virtual ezResult DeInitializeSubTest(ezInt32 iIdentifier) override;
  virtual ezTestAppRun RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount) override;

  void DeadPassCulling();
  void ResourceAliasing();
  void ImportReplace();
  void ExecuteCallbacks();
  void EmptyGraph();
  void MsaaResolve();
  ezTestAppRun StressTestRenderGraph(ezUInt32 uiNumPasses);

private:
  ezSharedPtr<ezRenderGraph> m_pRenderGraph;
};
