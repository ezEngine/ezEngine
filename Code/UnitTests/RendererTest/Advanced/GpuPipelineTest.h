#pragma once

#include <Foundation/Types/SharedPtr.h>
#include <RendererTest/TestClass/TestClass.h>

class ezGpuPipelineTest : public ezGraphicsTest
{
public:
  virtual const char* GetTestName() const override { return "GpuPipeline"; }

private:
  enum SubTests
  {
    ST_DeadPassCulling,
    ST_DependencySorting,
    ST_PassThroughSorting,
    ST_CycleDetection,
    ST_TextureSwitch,
    ST_SubGraphInlining,
    ST_SubGraphInliningErrors,
    ST_ViewBlackboard,
    ST_BufferPassThroughSorting,
    ST_MixedPassThrough,
    ST_MixedPinIndexing,
    ST_BufferSwitch,
    ST_SubGraphBufferInlining,
    ST_IncompatiblePinConnection,
    ST_SharedSourceSwitch,
  };

  virtual void SetupSubTests() override;
  virtual ezResult InitializeSubTest(ezInt32 iIdentifier) override;
  virtual ezResult DeInitializeSubTest(ezInt32 iIdentifier) override;
  virtual ezTestAppRun RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount) override;

  void DeadPassCulling();
  void DependencySorting();
  void PassThroughSorting();
  void CycleDetection();
  void TextureSwitch();
  void SubGraphInlining();
  void SubGraphInliningErrors();
  void ViewBlackboard();
  void BufferPassThroughSorting();
  void MixedPassThrough();
  void MixedPinIndexing();
  void BufferSwitch();
  void SubGraphBufferInlining();
  void IncompatiblePinConnection();
  void SharedSourceSwitch();

  ezSharedPtr<ezRenderGraph> m_pRenderGraph;
};