#pragma once

#include <ProcGenPlugin/Tasks/VertexColorTask.h>
#include <TestFramework/Utilities/TestLogInterface.h>

#include "../TestClass/TestClass.h"

class ezGameEngineTestProcGen : public ezGameEngineTest
{
  using SUPER = ezGameEngineTest;

public:
  virtual const char* GetTestName() const override;
  virtual ezGameEngineTestApplication* CreateApplication() override;

protected:
  enum SubTests
  {
    VertexColors,
    CurveNode,
  };

  virtual void SetupSubTests() override;
  virtual ezResult InitializeSubTest(ezInt32 iIdentifier) override;
  virtual ezTestAppRun RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount) override;
  virtual ezResult DeInitializeTest() override;

  using InputVertex = ezProcGenInternal::VertexColorTask::InputVertex;
  ezResult TestOutput(const ezHashedString& sOutputName, ezArrayPtr<InputVertex> inputVertices, ezArrayPtr<const ezVec4> expectedOutputs);

  ezInt32 m_iFrame = 0;
  ezGameEngineTestApplication* m_pOwnApplication = nullptr;

  ezUInt32 m_uiImgCompIdx = 0;
  ezHybridArray<ezUInt32, 8> m_ImgCompFrames;

  ezExpression::GlobalData m_GlobalData;
  ezUniquePtr<ezExpressionVM> m_pVM;
};
