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
  };

  virtual void SetupSubTests() override;
  virtual ezResult InitializeSubTest(ezInt32 iIdentifier) override;
  virtual ezTestAppRun RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount) override;

  ezInt32 m_iFrame = 0;
  ezGameEngineTestApplication* m_pOwnApplication = nullptr;

  ezUInt32 m_iImgCompIdx = 0;
  ezHybridArray<ezUInt32, 8> m_ImgCompFrames;
};
