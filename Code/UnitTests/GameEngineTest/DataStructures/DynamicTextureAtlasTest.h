#pragma once

#include <GameEngineTest/GameEngineTestPCH.h>

#include <RendererCore/Textures/DynamicTextureAtlas.h>

#include "../TestClass/TestClass.h"

class ezGameEngineTestDynamicTextureAtlas : public ezGameEngineTest
{
  using SUPER = ezGameEngineTest;

public:
  virtual const char* GetTestName() const override;
  virtual ezGameEngineTestApplication* CreateApplication() override;

private:
  enum SubTests
  {
    ST_AllocationsSmall,
    ST_AllocationsLarge,
    ST_AllocationsMixed,
    ST_Deallocations,
    ST_Deallocations2,
  };

  virtual void SetupSubTests() override;

  virtual ezResult InitializeSubTest(ezInt32 iIdentifier) override;
  virtual ezResult DeInitializeSubTest(ezInt32 iIdentifier) override;
  virtual ezTestAppRun RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount) override;

private:
  ezInt32 m_iFrame = 0;
  ezGameEngineTestApplication* m_pOwnApplication = nullptr;

  ezDynamicTextureAtlas m_TextureAtlas;
};
