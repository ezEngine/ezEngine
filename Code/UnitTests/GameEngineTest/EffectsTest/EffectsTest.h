#pragma once

#include <GameEngineTest/GameEngineTestPCH.h>

#include "../TestClass/TestClass.h"

class ezGameEngineTestEffects : public ezGameEngineTest
{
  using SUPER = ezGameEngineTest;

public:
  virtual const char* GetTestName() const override;
  virtual ezGameEngineTestApplication* CreateApplication() override;

protected:
  enum SubTests
  {
    Decals,
    Heightfield,
    WindClothRopes,
    Reflections,
    StressTest,
    AdvancedMeshes,
  };

  virtual void SetupSubTests() override;
  virtual ezResult InitializeSubTest(ezInt32 iIdentifier) override;
  virtual ezTestAppRun RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount) override;

  ezInt32 m_iFrame = 0;
  ezGameEngineTestApplication* m_pOwnApplication = nullptr;

  ezUInt32 m_uiImgCompIdx = 0;

  struct ImgCompare
  {
    ImgCompare(ezUInt32 uiFrame, ezUInt32 uiThreshold = 450)
    {
      m_uiFrame = uiFrame;
      m_uiThreshold = uiThreshold;
    }

    ezUInt32 m_uiFrame;
    ezUInt32 m_uiThreshold = 450;
  };

  ezHybridArray<ImgCompare, 8> m_ImgCompFrames;
};
