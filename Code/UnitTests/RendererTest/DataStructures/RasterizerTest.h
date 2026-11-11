#pragma once

#include "../TestClass/TestClass.h"
#include <RendererCore/Rasterizer/RasterizerView.h>

class ezRasterizerTest : public ezGraphicsTest
{
public:
  ezRasterizerTest() = default;

  virtual const char* GetTestName() const override { return "Rasterizer"; }

private:
  enum SubTests
  {
    ST_ThirdPartyCityPerformance,
    ST_MOCSSE2CityPerformance,
    ST_MOCSSE41CityPerformance,
    ST_MOCAVX2CityPerformance,
    ST_MOCAVX512CityPerformance,
    ST_GenericMOCCityPerformance,
  };

  virtual void SetupSubTests() override;
  virtual ezTestAppRun RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount) override;
  static ezRasterizerImplementation GetImplementation(ezInt32 iIdentifier);
};
